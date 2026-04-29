#version 460 core

in vec3 in_Pos;
in vec2 in_UV;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

layout(location = 0) out vec2 out_UV;

void main()
{
    gl_Position = u_Projection * u_View * u_Model * vec4(in_Pos, 1.0);
    out_UV = in_UV;
}
