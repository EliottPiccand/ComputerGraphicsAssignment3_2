#version 460 core

layout(location = 0) in vec4 in_Color;

out vec4 out_Color;
out vec4 out_Normal;

void main()
{
    out_Color = in_Color;
    out_Normal = vec4(0.0);
}
