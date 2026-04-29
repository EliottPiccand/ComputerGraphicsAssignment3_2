#version 460 core

in vec3 in_Pos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    vec4 posWorld = u_Model * vec4(in_Pos, 1.0);

    gl_Position = u_Projection * u_View * posWorld;
}
