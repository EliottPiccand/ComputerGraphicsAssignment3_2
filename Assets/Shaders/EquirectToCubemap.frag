#version 460 core

layout(location = 0) in vec3 out_LocalPos;

uniform sampler2D u_EquirectangularMap;

out vec4 out_Color;

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
    vec3 dir = normalize(out_LocalPos);
    vec3 equirectDir = vec3(dir.x, dir.z, -dir.y);
    vec3 hdr = texture(u_EquirectangularMap, sampleEquirect(equirectDir)).rgb;
    out_Color = vec4(hdr, 1.0);
}
