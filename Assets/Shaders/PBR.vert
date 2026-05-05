#version 460 core

in vec3 in_Pos;
in vec3 in_Normal;
in vec2 in_UV;

uniform mat4 u_Model;
uniform mat3 u_ModelNormal;
uniform mat4 u_View;
uniform mat4 u_Projection;
#ifdef FLAP
uniform float u_Time;
uniform float u_Width;
#endif

layout(location = 0) out vec3 out_Pos;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec2 out_UV;

#ifdef FLAP

const float PI = 3.14159265359;

const float WAVE_BASE_AMPLITUDE = 0.25;
const float WAVE_SECONDARY_AMPLITUDE = 0.08;
const float WAVE_LENGTH_FACTOR = 0.8;
const float WAVE_SPEED = 2.75;

float evaluateWave(float x)
{
    const float WAVE_LENGTH = u_Width * WAVE_LENGTH_FACTOR;

    const float u = clamp(x / u_Width, 0.0, 1.0);
    const float envelope = u * u;
    const float k = 2.0 * PI / WAVE_LENGTH;

    const float primary = sin(k * x - WAVE_SPEED * u_Time);
    const float secondary = sin(2.0 * k * x - 1.5 * WAVE_SPEED * u_Time);

    return envelope * (WAVE_BASE_AMPLITUDE * primary + WAVE_SECONDARY_AMPLITUDE * secondary);
}

float evaluateWaveDerivative(float x)
{
    const float WAVE_LENGTH = u_Width * WAVE_LENGTH_FACTOR;

    const float u = clamp(x / u_Width, 0.0, 1.0);
    const float envelope = u * u;
    const float envelope_derivative = 2.0 * u / u_Width;
    const float k = 2.0 * PI / WAVE_LENGTH;

    const float phase_1 = k * x - WAVE_SPEED * u_Time;
    const float phase_2 = 2.0 * k * x - 1.5 * WAVE_SPEED * u_Time;

    const float oscillation = WAVE_BASE_AMPLITUDE * sin(phase_1) + WAVE_SECONDARY_AMPLITUDE * sin(phase_2);
    const float oscillation_derivative =
        WAVE_BASE_AMPLITUDE * k * cos(phase_1) + WAVE_SECONDARY_AMPLITUDE * 2.0 * k * cos(phase_2);

    return envelope_derivative * oscillation + envelope * oscillation_derivative;
}

#endif

void main()
{
    #ifdef FLAP
        float wave = evaluateWave(in_Pos.x);
        float waveDerivative = evaluateWaveDerivative(in_Pos.x);
        vec3 frontNormal = normalize(vec3(-waveDerivative, 0.0, 1.0));

        vec3 pos = in_Pos + vec3(0.0, 0.0, wave);
        vec3 normal = frontNormal * sign(dot(in_Normal, frontNormal));
    #else
        vec3 pos = in_Pos;
        vec3 normal = in_Normal;
    #endif

    vec4 posWorld = u_Model * vec4(pos, 1.0);

    gl_Position = u_Projection * u_View * posWorld;
    out_Pos = posWorld.xyz;
    out_Normal = normalize(u_ModelNormal * normal);
    out_UV = in_UV;
}
