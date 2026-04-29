#version 460 core

in vec2 in_Pos;
in vec2 in_UV;

uniform mat3 u_Model;

layout (location = 0) out vec2 out_UV;

void main()
{
    gl_Position = vec4(u_Model * vec3(in_Pos, 0.0), 1.0);
    out_UV = in_UV;
}
