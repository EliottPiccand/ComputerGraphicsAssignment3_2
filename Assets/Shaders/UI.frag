#version 460 core

layout(location = 0) in vec2 in_UV;

uniform float u_Alpha;
uniform sampler2D u_AlbedoTexture;

out vec4 out_Color;
out vec4 out_Normal;

void main()
{
    out_Color = texture(u_AlbedoTexture, in_UV) * u_Alpha;
    out_Normal = vec4(0.0, 0.0, 1.0, 0.0);
}
