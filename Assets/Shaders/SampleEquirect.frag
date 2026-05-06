vec2 sampleEquirect(vec3 dir)
{
    vec3 n = normalize(dir);
    float theta = atan(n.z, n.x);
    float phi = asin(clamp(n.y, -1.0, 1.0));

    float u = 0.5 + theta / (2.0 * PI);
    float v = 0.5 - phi / PI;
    return vec2(u, v);
}
