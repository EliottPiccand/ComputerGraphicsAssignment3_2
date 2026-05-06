float CalculateNormalDistributionGGX(float roughness, float nDotH)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (nDotH * nDotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

vec3 CalculateSchlickFresnelReflectance(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 CalculateSchlickFresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float CalculateSmithGGXGeometryTerm(float roughness, float nDotL, float nDotV)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float g1L = nDotL / (nDotL * (1.0 - k) + k);
    float g1V = nDotV / (nDotV * (1.0 - k) + k);

    return g1L * g1V;
}
