#version 460 core

in vec3 position;
in vec4 texCoord0;

out VS_OUT
{
    vec3 position;
    vec4 texCoord0;
} vs_out;

void main()
{
    vs_out.position  = position;
    vs_out.texCoord0 = texCoord0;
}
