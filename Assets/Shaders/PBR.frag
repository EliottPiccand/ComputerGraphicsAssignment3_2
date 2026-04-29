#version 460 core

layout(location = 0) in vec3 in_PosVS;
layout(location = 1) in vec3 in_NormalVS;
layout(location = 2) in vec2 in_UV;

uniform vec4 u_TextureColor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform sampler2D u_Texture;
uniform sampler2D u_MetallicRoughnessTex; // Packed: R=Unused, G=Metallic, B=Roughness (Linear)

out vec4 out_Color;

void main()
{
    out_Color = texture(u_Texture, in_UV) * u_TextureColor;
}
