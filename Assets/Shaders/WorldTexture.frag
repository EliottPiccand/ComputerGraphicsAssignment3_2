#version 460 core

layout(location = 0) in vec2 in_UV;

uniform sampler2D u_Texture;

out vec4 out_Color;
out vec4 out_Normal;

void main()
{
    out_Color = texture(u_Texture, in_UV);
    out_Normal = vec4(0.0);
}
