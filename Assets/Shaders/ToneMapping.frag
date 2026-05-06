vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 gammaCorrection(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

// TODO: remove toneMapping from main pass and add it as a post processing effect 
vec3 toneMapping(vec3 color)
{
    color = ACESFilm(color);
    color = gammaCorrection(color);
    return color;
}
