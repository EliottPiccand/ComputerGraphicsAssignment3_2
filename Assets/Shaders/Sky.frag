#version 460 core

layout(location = 0) in vec3 in_WorldDir;

uniform sampler2D u_EnvironmentMap;

out vec4 out_Color;
out vec4 out_Normal;

const float PI = 3.14159265359;

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
    vec4 color = texture(u_EnvironmentMap, sampleEquirect(equirectDir));
    out_Color = vec4(toneMapping(color.rgb), color.a);
    out_Normal = vec4(0.0);
}
