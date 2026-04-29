#version 460 core

layout(location = 0) in vec4 in_Color;

out vec4 out_Color;

void main()
{
    out_Color = in_Color;
}
