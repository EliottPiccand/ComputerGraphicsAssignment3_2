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

uniform float u_SpecularFactor;
uniform vec3 u_SpecularColor;

uniform bool u_UseIBL;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BrdfLut;
uniform float u_MaxReflectionLod;

out vec4 out_Color;

const float MIN_ROUGHNESS = 0.04;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // region Albedo
    vec4 albedoAlpha = texture(u_AlbedoTexture, in_UV) * u_AlbedoColor;
    vec3 albedo = albedoAlpha.rgb;
    float alpha = albedoAlpha.a;

    if (alpha < 0.1)
        discard;

    // region Metallic / Roughness
    vec2 mrData = texture(u_MetallicRoughnessTexture, in_UV).gb;
    float metallic = mrData.y * u_MetallicFactor;
    float roughness = clamp(mrData.x * u_RoughnessFactor, MIN_ROUGHNESS, 1.0);

    // region Normal Mapping
    vec3 dp1 = dFdx(in_Pos);
    vec3 dp2 = dFdy(in_Pos);
    vec2 duv1 = dFdx(in_UV);
    vec2 duv2 = dFdy(in_UV);

    float det = duv1.x * duv2.y - duv2.x * duv1.y;

    vec3 N;
    if (abs(det) < 0.0001)
    {
        N = in_Normal;
    }
    else
    {
        float invDet = 1.0 / det;

        vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * invDet;
        vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * invDet;

        vec3 N_world = in_Normal;
        T = normalize(T - dot(T, N_world) * N_world);
    B = normalize(B - dot(B, N_world) * N_world - dot(B, T) * T);

        mat3 TBN = mat3(T, B, N_world);

        vec3 N_map = texture(u_NormalMap, in_UV).rgb;
        N_map = normalize(N_map * 2.0 - 1.0);
        N = normalize(TBN * N_map);
    }

    // region Material Response
    vec3 dielectricF0 = min(vec3(MIN_ROUGHNESS) * u_SpecularColor * u_SpecularFactor, vec3(1.0));

    vec3 F0 = mix(dielectricF0, albedo, metallic);
    vec3 V = normalize(u_CameraPosition - in_Pos);

    // region Directional Lighting
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_DirectionalLightCount && i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        DirectionalLight l = u_DirectionalLights[i];

        vec3 L = -l.direction;
        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        vec3 radiance = l.color * NdotL;

        vec3 diffuse = kD * albedo / PI;
        Lo += (diffuse + specular) * radiance;
    }

    vec3 emissive = texture(u_EmissiveTexture, in_UV).rgb * u_EmissiveColor;

    // region Ambient Lighting
    vec3 ambient;
    if (u_UseIBL)
    {
        vec3 R = reflect(-V, N);
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo / PI;

        // region Reflections
        float mip_level = roughness * u_MaxReflectionLod;
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, mip_level).rgb;

        vec2 brdf = texture(u_BrdfLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F * brdf.x + vec3(brdf.y));

        ambient = kD * diffuseIBL + specularIBL;
    }
    else
    {
        ambient = u_AmbientColor * albedo;
    }

    vec3 color = ambient + Lo + emissive;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_Color = vec4(color, alpha);
}
