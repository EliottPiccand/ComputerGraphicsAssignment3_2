#version 460 core

layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_UV;

uniform vec3 u_CameraPosition;

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
};
uniform int u_DirectionalLightCount;
uniform DirectionalLight u_DirectionalLights[MAX_DIRECTIONAL_LIGHTS];

uniform vec3 u_AmbientColor;

uniform vec4 u_AlbedoColor;
uniform sampler2D u_AlbedoTexture;

/// 0 → 1 = Dielectric → Metal
uniform float u_MetallicFactor;
/// 0 → 1 = Mirror → Matte
uniform float u_RoughnessFactor;
/// R: unused | G: roughness | B: metallic
uniform sampler2D u_MetallicRoughnessTexture;

uniform sampler2D u_NormalMap;

uniform vec3 u_EmissiveColor;
uniform sampler2D u_EmissiveTexture;

uniform sampler2D u_EnvironmentMap;

out vec4 out_Color;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec2 sampleEquirect(vec3 dir)
{
    vec3 n = normalize(dir);
    float theta = atan(n.z, n.x);
    float phi = asin(clamp(n.y, -1.0, 1.0));

    float u = 0.5 + theta / (2.0 * PI);
    float v = 0.5 - phi / PI;
    return vec2(u, v);
}

vec3 computeNormal()
{
    vec3 N = normalize(in_Normal);

    vec3 dp1  = dFdx(in_Pos);
    vec3 dp2  = dFdy(in_Pos);
    vec2 duv1 = dFdx(in_UV);
    vec2 duv2 = dFdy(in_UV);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (abs(det) < 1e-5)
        return N;

    float invDet = 1.0 / det;

    vec3 T   = normalize((dp1 * duv2.y - dp2 * duv1.y) * invDet);
    vec3 B = normalize((dp2 * duv1.x - dp1 * duv2.x) * invDet);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    if (dot(cross(T, B), N) < 0.0)
        T = -T;

    mat3 TBN = mat3(T, B, N);

    vec3 normalMap = texture(u_NormalMap, in_UV).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);

    return normalize(TBN * normalMap);
}

void main()
{
    // Albedo
    vec4 albedoWithAlpha = texture(u_AlbedoTexture, in_UV) * u_AlbedoColor;
    vec3 albedo = albedoWithAlpha.rgb;

    // Metallic / Roughness
    vec2 mr = texture(u_MetallicRoughnessTexture, in_UV).gb;
    float roughness = max(mr.r * u_RoughnessFactor, 0.04);
    float metallic  =     mr.g * u_MetallicFactor;

    // Emissive
    vec3 emissive = texture(u_EmissiveTexture, in_UV).rgb * u_EmissiveColor;

    // Lights
    vec3 N = computeNormal();
    vec3 V = normalize(u_CameraPosition - in_Pos);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    // - Directionals
    for (int i = 0; i < u_DirectionalLightCount; i++)
    {
        DirectionalLight light = u_DirectionalLights[i];

        vec3 L = normalize(-light.direction);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);

        float D = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
        vec3  numerator = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3  specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        vec3 diffuse = kD * albedo / PI;

        vec3 radiance = light.color;
        Lo += (diffuse + specular) * radiance * NdotL;
    }

    // Ambient
    vec3 R = reflect(-V, N);
    vec3 equiR = vec3(R.x, R.z, -R.y);
    float lod = roughness * 10.0;
    vec3 envColor = textureLod(u_EnvironmentMap, sampleEquirect(equiR), lod).rgb;

    vec3 F_amb = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kD_amb = (1.0 - F_amb) * (1.0 - metallic);

    vec3 ambient_diffuse = kD_amb * albedo * u_AmbientColor;
    vec3 ambient_specular = F_amb * envColor * (1.0 - roughness);
    vec3 ambient = ambient_diffuse + ambient_specular;

    // Composition
    vec3 color = ambient + Lo + emissive;

    out_Color = vec4(toneMapping(color), albedoWithAlpha.a);
}
