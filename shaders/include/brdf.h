// Specifies minimal reflectance for dielectrics (when metalness is zero)
// Nothing has lower reflectance than 2%, but we use 4% to have consistent results with UE4, Frostbite, et al.
#define MIN_DIELECTRICS_F0 0.04f

const uint DIFFUSE = 1;
const uint SPECULAR = 2;

// Samples a microfacet normal for the GGX distribution using VNDF method.
// Source: "Sampling the GGX Distribution of Visible Normals" by Heitz
// See also https://hal.inria.fr/hal-00996995v1/document and http://jcgt.org/published/0007/04/01/
// Random variables 'u' must be in <0;1) interval
// PDF is 'G1(NdotV) * D'
vec3 sample_GGXVNDF(vec3 Ve, vec2 alpha2D, uint seed) {
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha2D.x * Ve.x, alpha2D.y * Ve.y, Ve.z));

    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0.0f ? vec3(-Vh.y, Vh.x, 0.0f) * inversesqrt(lensq) : vec3(1.0f, 0.0f, 0.0f);
    vec3 T2 = cross(Vh, T1);

    // Section 4.2: parameterization of the projected area
    float r = sqrt(rand(seed));
    float phi = TWO_PI * rand(seed);
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = mix(sqrt(1.0f - t1 * t1), t2, s);

    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;

    // Section 3.4: transforming the normal back to the ellipsoid configuration
    return normalize(vec3(alpha2D.x * Nh.x, alpha2D.y * Nh.y, max(0.0f, Nh.z)));
}

// Smith G1 term (masking function) further optimized for GGX distribution (by substituting G_a into G1_GGX)
float Smith_G1_GGX(float alpha, float NdotS, float alpha_squared, float NdotS_squared) {
    return 2.0f / (sqrt(((alpha_squared * (1.0f - NdotS_squared)) + NdotS_squared) / NdotS_squared) + 1.0f);
}

// Weight for the reflection ray sampled from GGX distribution using VNDF method
float specular_sample_weight_GGXVNDF(float alpha, float alpha_squared, float NdotL, float NdotV, float HdotL, float NdotH) {
#if USE_HEIGHT_CORRELATED_G2
    return Smith_G2_Over_G1_Height_Correlated(alpha, alpha_squared, NdotL, NdotV);
#else 
    return Smith_G1_GGX(alpha, NdotL, alpha_squared, NdotL * NdotL);
#endif
}

vec3 base_color_to_specular_F0(vec3 base_color, float metalness) {
    return mix(vec3(MIN_DIELECTRICS_F0), base_color, metalness);
}

vec3 base_color_to_diffuse_reflectance(vec3 base_color, float metalness) {
    return base_color * (1.0f - metalness);
}

// Schlick's approximation to Fresnel term
// f90 should be 1.0, except for the trick used by Schuler (see 'shadowed_F90' function)
vec3 eval_fresnel_schlick(vec3 f0, float f90, float NdotS) {
    return f0 + (f90 - f0) * pow(1.0f - NdotS, 5.0f);
}

vec3 eval_fresnel(vec3 f0, float f90, float NdotS) {
    // Default is Schlick's approximation
    return eval_fresnel_schlick(f0, f90, NdotS);
}

// Attenuates F90 for very low F0 values
// Source: "An efficient and Physically Plausible Real-Time Shading Model" in ShaderX7 by Schuler
// Also see section "Overbright highlights" in Hoffman's 2010 "Crafting Physically Motivated Shading Models for Game Development" for discussion
// IMPORTANT: Note that when F0 is calculated using metalness, it's value is never less than MIN_DIELECTRICS_F0, and therefore,
// this adjustment has no effect. To be effective, F0 must be authored separately, or calculated in different way. See main text for discussion.
float shadowed_F90(vec3 F0) {
    // This scaler value is somewhat arbitrary, Schuler used 60 in his article. In here, we derive it from MIN_DIELECTRICS_F0 so
    // that it takes effect for any reflectance lower than least reflective dielectrics
    //const float t = 60.0f;
    const float t = (1.f / MIN_DIELECTRICS_F0);
    return min(1.f, t * luminance(F0));
}

float get_brdf_probability(vec3 color, float metalness, vec3 view, vec3 shading_normal) {
    // Evaluate Fresnel term using the shading normal
    // Note: we use the shading normal instead of the microfacet normal (half-vector) for Fresnel term here. That's suboptimal for rough surfaces at grazing angles, but half-vector is yet unknown at this point
    vec3 specular_F0 = vec3(luminance(base_color_to_specular_F0(color, metalness)));
    float diffuse_reflectance = luminance(base_color_to_diffuse_reflectance(color, metalness));
    float fresnel = clamp(luminance(eval_fresnel(specular_F0, shadowed_F90(specular_F0), max(0.0f, dot(view, shading_normal)))), 0.0, 1.0);

    // Approximate relative contribution of BRDFs using the Fresnel term
    float specular = fresnel;
    float diffuse = diffuse_reflectance * (1.0f - fresnel); //< If diffuse term is weighted by Fresnel, apply it here as well

    // Return probability of selecting specular BRDF over diffuse BRDF
    float p = (specular / max(0.0001f, (specular + diffuse)));

    // Clamp probability to avoid undersampling of less prominent BRDF
    return clamp(p, 0.1f, 0.9f);
}
