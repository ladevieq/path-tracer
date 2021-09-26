vec3 linear_to_srgb(vec3 color)
{
    color = clamp(color, 0.0f, 1.0f);
     
    return mix(
        pow(color, vec3(1.0f / 2.4f)) * 1.055f - 0.055f,
        color * 12.92f,
        less_than(color, 0.0031308f)
    );
}
 
vec3 srgb_to_linear(vec3 color)
{
    color = clamp(color, 0.0f, 1.0f);
     
    return mix(
        pow(((color + 0.055f) / 1.055f), vec3(2.4f)),
        color / 12.92f,
        less_than(color, 0.04045f)
    );
}

float luminance(vec3 rgb)
{
    return dot(rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}
