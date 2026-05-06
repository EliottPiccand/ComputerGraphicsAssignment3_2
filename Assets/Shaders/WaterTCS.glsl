#version 460 core

layout(vertices = 3) out;

in VS_OUT
{
    vec3 position;
    vec4 texCoord0;
} tc_in[];

out TC_OUT
{
    vec3 position;
    vec4 texCoord0;
} tc_out[];

uniform float u_TessellationFactor;

void main()
{
    tc_out[gl_InvocationID].position = tc_in[gl_InvocationID].position;
    tc_out[gl_InvocationID].texCoord0 = tc_in[gl_InvocationID].texCoord0;

    gl_TessLevelInner[0] = u_TessellationFactor;
    gl_TessLevelOuter[0] = u_TessellationFactor;
    gl_TessLevelOuter[1] = u_TessellationFactor;
    gl_TessLevelOuter[2] = u_TessellationFactor;
}
