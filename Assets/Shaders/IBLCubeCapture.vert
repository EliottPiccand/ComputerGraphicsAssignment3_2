#version 460 core

layout(location = 0) in vec3 in_Pos;

uniform mat4 u_View;
uniform mat4 u_Projection;

layout(location = 0) out vec3 out_LocalPos;

void main()
{
    out_LocalPos = in_Pos;
    gl_Position = u_Projection * u_View * vec4(in_Pos, 1.0);
}
