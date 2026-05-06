#version 460 core

in TE_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec4 positionView;
    vec4 texCoord0;
    vec4 screenPosition;
    vec4 positionWorld;
    vec4 worldNormalAndHeight;
}
input_;

uniform mat4 u_View;
uniform mat4 u_ViewInverse;
uniform mat4 u_Projection;
uniform mat4 u_ProjectionInverse;
uniform mat4 u_ViewProjectionInverse;
uniform vec4 u_SsrSettings;
uniform vec4 u_WaterSurfaceColor;
uniform vec3 u_LightDirection;
uniform vec4 u_NormalMapScroll;
uniform vec2 u_NormalMapScrollSpeed;
uniform float u_Roughness;
uniform float u_Reflectance;
uniform float u_SpecIntensity;
uniform float u_Time;
uniform float u_RefractionDistortionFactor;
uniform vec4 u_WaterRefractionColor;
uniform float u_DepthSofteningDistance;
uniform float u_RefractionDistanceFactor;
uniform float u_RefractionHeightFactor;
uniform float u_FoamTiling;
uniform float u_FoamHeightStart;
uniform float u_FoamFadeDistance;
uniform float u_FoamBrightness;
uniform float u_FoamAngleExponent;

uniform sampler2D u_WaterNormalMap1;
uniform sampler2D u_WaterNormalMap2;
uniform sampler2D u_EnvironmentMap;
uniform sampler2D u_WaterFoamMap;
uniform sampler2D u_WaterNoiseMap;
uniform sampler2D u_HDRMap;
uniform sampler2D u_DepthMap;
uniform sampler2D u_NormalMap;

out vec4 out_Color;

const float EPSILON = 1e-5;
const float PI = 3.14159265359;

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

float CalculateNormalDistributionGGX(float roughness, float nDotH)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (nDotH * nDotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

vec3 CalculateSchlickFresnelReflectance(float lDotH, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - lDotH, 5.0);
}

float CalculateSmithGGXGeometryTerm(float roughness, float nDotL, float nDotV)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float g1L = nDotL / (nDotL * (1.0 - k) + k);
    float g1V = nDotV / (nDotV * (1.0 - k) + k);

    return g1L * g1V;
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

vec3 GetWorldPositionFromDepth(vec2 uv, float depth, mat4 invViewProjection)
{
    // Convert from [0,1] depth to NDC [-1,1]
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);

    vec4 viewSpacePosition = invViewProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

vec3 GetViewPositionFromDepth(vec2 uv, float depth, mat4 invProjection)
{
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);

    vec4 viewSpacePosition = invProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

void main()
{
    vec3 input_normal = normalize(input_.normal);
    vec3 input_tangent = normalize(input_.tangent);
    vec3 input_binormal = normalize(input_.binormal);

    vec2 normalMapCoords1 = input_.texCoord0.xy + u_Time * u_NormalMapScroll.xy * u_NormalMapScrollSpeed.x;
    vec2 normalMapCoords2 = input_.texCoord0.xy + u_Time * u_NormalMapScroll.zw * u_NormalMapScrollSpeed.y;
    vec2 hdrCoords = ((vec2(input_.screenPosition.x, -input_.screenPosition.y) / input_.screenPosition.w) * 0.5) + 0.5;

    vec3 normalMap = (texture(u_WaterNormalMap1, normalMapCoords1).rgb * 2.0) - 1.0;
    vec3 normalMap2 = (texture(u_WaterNormalMap2, normalMapCoords2).rgb * 2.0) - 1.0;
    mat3 texSpace = mat3(input_tangent, input_binormal, input_normal);

    vec3 n1 = normalize(texSpace * normalMap);
    vec3 n2 = normalize(texSpace * normalMap2);
    vec3 finalNormal = normalize(n1 + n2);

    float linearRoughness = u_Roughness * u_Roughness;
    vec3 viewDir = -normalize(input_.positionView.xyz);
    vec3 lightDir = normalize((u_View * vec4(-u_LightDirection, 0.0)).xyz);
    vec3 half_ = normalize(viewDir + lightDir);
    float nDotL = saturate(dot(finalNormal, lightDir));
    float nDotV = abs(dot(finalNormal, viewDir)) + EPSILON;
    float nDotH = saturate(dot(finalNormal, half_));
    float lDotH = saturate(dot(lightDir, half_));

    vec3 f0 = 0.16 * u_Reflectance * u_Reflectance * vec3(1.0);
    float normalDistribution = CalculateNormalDistributionGGX(linearRoughness, nDotH);
    vec3 fresnelReflectance = CalculateSchlickFresnelReflectance(lDotH, f0);
    float geometryTerm = CalculateSmithGGXGeometryTerm(linearRoughness, nDotL, nDotV);

    float specularNoise = texture(u_WaterNoiseMap, normalMapCoords1 * 0.5).r;
    specularNoise *= texture(u_WaterNoiseMap, normalMapCoords2 * 0.5).r;
    specularNoise *= texture(u_WaterNoiseMap, input_.texCoord0.xy * 0.5).r;

    vec3 specularFactor =
        (geometryTerm * normalDistribution) * fresnelReflectance * u_SpecIntensity * nDotL * specularNoise;

    float sceneZ = 0;
    float stepCount = 0;
    float forwardStepCount = u_SsrSettings.y;
    vec3 rayMarchPosition = input_.positionView.xyz;
    vec4 rayMarchTexPosition = vec4(0, 0, 0, 0);
    vec3 reflectionVector = reflect(viewDir, finalNormal);

    while (stepCount < u_SsrSettings.y)
    {
        rayMarchPosition += reflectionVector.xyz * u_SsrSettings.x;
        rayMarchTexPosition = u_Projection * vec4(rayMarchPosition, 1.0);

        if (abs(rayMarchTexPosition.w) < EPSILON)
        {
            rayMarchTexPosition.w = EPSILON;
        }

        rayMarchTexPosition.xy /= rayMarchTexPosition.w;
        rayMarchTexPosition.xy = vec2(rayMarchTexPosition.x, -rayMarchTexPosition.y) * 0.5 + 0.5;

        sceneZ = textureLod(u_DepthMap, rayMarchTexPosition.xy, 0).r;
        sceneZ = GetViewPositionFromDepth(rayMarchTexPosition.xy, sceneZ, u_ProjectionInverse).z;

        if (sceneZ >= rayMarchPosition.z)
        {
            forwardStepCount = stepCount;
            stepCount = u_SsrSettings.y;
        }
        else
        {
            stepCount++;
        }
    }

    if (forwardStepCount < u_SsrSettings.y)
    {
        stepCount = 0;
        while (stepCount < u_SsrSettings.z)
        {
            rayMarchPosition -= reflectionVector.xyz * u_SsrSettings.x / u_SsrSettings.z;
            rayMarchTexPosition = u_Projection * vec4(-rayMarchPosition, 1);

            if (abs(rayMarchTexPosition.w) < EPSILON)
            {
                rayMarchTexPosition.w = EPSILON;
            }

            rayMarchTexPosition.xy /= rayMarchTexPosition.w;
            rayMarchTexPosition.xy = vec2(rayMarchTexPosition.x, -rayMarchTexPosition.y) * 0.5 + 0.5;

            sceneZ = texture(u_DepthMap, rayMarchTexPosition.xy, 0).r;
            sceneZ = GetViewPositionFromDepth(rayMarchTexPosition.xy, sceneZ, u_ProjectionInverse).z;

            if (sceneZ > rayMarchPosition.z)
            {
                stepCount = u_SsrSettings.z;
            }
            else
            {
                stepCount++;
            }
        }
    }

    vec3 ssrReflectionNormal = normalize(texture(u_NormalMap, rayMarchTexPosition.xy).rgb * 2.0 - 1.0);
    vec2 ssrDistanceFactor = vec2(distance(0.5, hdrCoords.x), distance(0.5, hdrCoords.y)) * 2;
    float ssrFactor = (1.0 - abs(nDotV)) * (1.0 - forwardStepCount / u_SsrSettings.y) *
                      saturate(1.0 - ssrDistanceFactor.x - ssrDistanceFactor.y) *
                      (1.0 / (1.0 + abs(sceneZ - rayMarchPosition.z) * u_SsrSettings.w)) *
                      (1.0 - saturate(dot(ssrReflectionNormal, finalNormal)));

    vec3 reflectionColor = texture(u_HDRMap, rayMarchTexPosition.xy).rgb;
    vec3 envReflection = mat3(u_ViewInverse) * reflectionVector;
    vec3 skyboxColor = texture(u_EnvironmentMap, sampleEquirect(envReflection)).rgb;
    reflectionColor = mix(skyboxColor, reflectionColor, ssrFactor);

    vec2 distortedTexCoord = hdrCoords + (finalNormal.xy * 0.5) * u_RefractionDistortionFactor;
    float  distortedDepth = texture(u_DepthMap, distortedTexCoord).r;
    vec3 distortedPosition = GetWorldPositionFromDepth(distortedTexCoord, distortedDepth, u_ViewProjectionInverse);
    vec2 refractionTexCoord = (distortedPosition.z < input_.positionWorld.z) ? distortedTexCoord : hdrCoords;
    vec3 waterColor = texture(u_HDRMap, refractionTexCoord).rgb * u_WaterRefractionColor.rgb;

    float sceneDepth = texture(u_DepthMap, hdrCoords).r;
    vec3 scenePosition = GetWorldPositionFromDepth(hdrCoords, sceneDepth, u_ViewProjectionInverse);
    float depthSoftenedAlpha = saturate(distance(scenePosition, input_.positionWorld.xyz) / u_DepthSofteningDistance);

    vec3 waterSurfacePosition = (distortedPosition.z < input_.positionWorld.z) ? distortedPosition : scenePosition;
    waterColor = mix(waterColor, u_WaterRefractionColor.rgb, saturate((input_.positionWorld.z - waterSurfacePosition.z) / u_RefractionHeightFactor));

    float waveTopReflectionFactor = pow(1.0 - saturate(dot(input_.normal, viewDir)), 3);
    vec3 waterBaseColor = mix(waterColor, reflectionColor, saturate(saturate(length(input_.positionView.xyz) / u_RefractionDistanceFactor) + waveTopReflectionFactor));

    vec3 finalWaterColor = (waterBaseColor + specularFactor) * u_WaterSurfaceColor.rgb;

    vec3 foamColor = texture(u_WaterFoamMap, (normalMapCoords1 + normalMapCoords2) * u_FoamTiling).rgb;
    float foamNoise = texture(u_WaterNoiseMap, input_.texCoord0.xy * u_FoamTiling).r;
    float foamAmount = saturate((input_.worldNormalAndHeight.w - u_FoamHeightStart) / u_FoamFadeDistance) * pow(saturate(dot(input_.worldNormalAndHeight.xyz, vec3(0, 0, 1))), u_FoamAngleExponent) * foamNoise;

    foamAmount += pow((1.0 - depthSoftenedAlpha), 3);

    finalWaterColor = mix(finalWaterColor, foamColor * u_FoamBrightness, saturate(foamAmount) * depthSoftenedAlpha);
    out_Color = vec4(finalWaterColor, depthSoftenedAlpha);
}
