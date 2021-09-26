uint pcg_hash(inout uint seed)
{
    uint state = seed;
    seed *= 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand(inout uint seed)
{
    return float(pcg_hash(seed)) / 4294967296.0;
}
