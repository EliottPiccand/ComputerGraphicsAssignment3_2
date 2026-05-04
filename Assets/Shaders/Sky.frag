#version 460 core

layout(location = 0) in vec3 in_WorldDir;

uniform sampler2D u_EnvironmentMap;

const float PI = 3.14159265359;

out vec4 out_Color;

vec2 sampleEquirect(vec3 dir)
{
    vec3 n = normalize(dir);
    float theta = atan(n.z, n.x);
    float phi = asin(clamp(n.y, -1.0, 1.0));

    float u = 0.5 + theta / (2.0 * PI);
    float v = 0.5 - phi / PI;
    return vec2(u, v);
}

void main()
{
    vec3 dir = in_WorldDir;
    vec3 equirectDir = vec3(dir.x, dir.z, -dir.y);

    vec3 hdr = texture(u_EnvironmentMap, sampleEquirect(equirectDir)).rgb;
    vec3 color = hdr / (hdr + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_Color = vec4(color, 1.0);
}
