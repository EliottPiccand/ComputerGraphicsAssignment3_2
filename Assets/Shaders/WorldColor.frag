#version 460 core

uniform vec4 u_Color;

out vec4 out_Color;
out vec4 out_Normal;

void main()
{
    out_Color = u_Color;
    out_Normal = vec4(0.0);
}
