#version 460 core

layout(triangles, equal_spacing, ccw) in;
in TC_OUT
{
    vec3 position;
    vec4 texCoord0;
} te_in[];

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_DampeningFactor;
uniform float u_Time;

out TE_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec4 positionView;
    vec4 texCoord0;
    vec4 screenPosition;
    vec4 positionWorld;
    vec4 worldNormalAndHeight;
} output_;

/******************************************************************************/
/*                                   Waves                                    */
/******************************************************************************/

struct WaveResult
{
    vec3 position;
    vec3 normal;
    vec3 binormal;
    vec3 tangent;
};

struct Wave
{
    vec3  direction;
    float steepness;
    float waveLength;
    float amplitude;
    float speed;
};

const uint numWaves = 2;

WaveResult CalculateWave(Wave wave, vec3 wavePosition, float edgeDampen)
{
    WaveResult result;

    vec3 direction = normalize(wave.direction);
 
    float frequency = 2.0 / wave.waveLength;
    float phaseConstant = wave.speed * frequency;
    float qi = wave.steepness / (wave.amplitude * frequency * numWaves);
    float rad = frequency * dot(direction.xy, wavePosition.xy) + u_Time * phaseConstant;
    float sinR = sin(rad);
    float cosR = cos(rad);
 
    result.position.x = wavePosition.x + qi * wave.amplitude * direction.x * cosR * edgeDampen;
    result.position.y = wavePosition.y + qi * wave.amplitude * direction.y * cosR * edgeDampen;
    result.position.z = wave.amplitude * sinR * edgeDampen;
 
    float waFactor = frequency * wave.amplitude;
    float radN = frequency * dot(direction.xy, result.position.xy) + u_Time * phaseConstant;
    float sinN = sin(radN);
    float cosN = cos(radN);
 
    result.binormal.x = -1 * (qi * direction.x * direction.y * waFactor * sinN);
    result.binormal.y = 1 - (qi * direction.y * direction.y * waFactor * sinN);
    result.binormal.z = direction.y * waFactor * cosN;
 
    result.tangent.x = 1 - (qi * direction.x * direction.x * waFactor * sinN);
    result.tangent.y = -1 * (qi * direction.x * direction.y * waFactor * sinN);
    result.tangent.z = direction.x * waFactor * cosN;
 
    result.normal.x = -1 * (direction.x * waFactor * cosN);
    result.normal.y = -1 * (direction.y * waFactor * cosN);
    result.normal.z = 1 - (qi * waFactor * sinN);
 
    result.binormal = normalize(result.binormal);
    result.tangent = normalize(result.tangent);
    result.normal = normalize(result.normal);
 
    return result;
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

void main()
{
    vec3 uvwCoord = gl_TessCoord;

    vec4 output_position = vec4(uvwCoord.x * te_in[0].position + uvwCoord.y * te_in[1].position + uvwCoord.z * te_in[2].position, 1.0);
    output_.texCoord0 = uvwCoord.x * te_in[0].texCoord0 + uvwCoord.y * te_in[1].texCoord0 + uvwCoord.z * te_in[2].texCoord0;
 
    Wave waves[numWaves];
    waves[0].direction = vec3(0.3, 0, -0.7);
    waves[0].steepness = 1.79;
    waves[0].waveLength = 3.75;
    waves[0].amplitude = 0.85;
    waves[0].speed = 1.21;

    waves[1].direction = vec3(0.5, 0, -0.2);
    waves[1].steepness = 1.79;
    waves[1].waveLength = 4.1;
    waves[1].amplitude = 0.52;
    waves[1].speed = 1.03;

    float dampening = 1.0 - pow(saturate(abs(output_.texCoord0.z - 0.5) / 0.5), u_DampeningFactor);
    dampening *= 1.0 - pow(saturate(abs(output_.texCoord0.w - 0.5) / 0.5), u_DampeningFactor);
 
    WaveResult finalWaveResult;
    finalWaveResult.position = vec3(0,0,0);
    finalWaveResult.normal = vec3(0,0,0);
    finalWaveResult.tangent = vec3(0,0,0);
    finalWaveResult.binormal = vec3(0,0,0);

    for(uint waveId = 0; waveId < numWaves; waveId++)
    {
        WaveResult waveResult = CalculateWave(waves[waveId], output_position.xyz, dampening);
        finalWaveResult.position += waveResult.position;
        finalWaveResult.normal += waveResult.normal;
        finalWaveResult.tangent += waveResult.tangent;
        finalWaveResult.binormal += waveResult.binormal;
    }

    finalWaveResult.position -= output_position.xyz * (numWaves - 1);
    finalWaveResult.normal = normalize(finalWaveResult.normal);
    finalWaveResult.tangent = normalize(finalWaveResult.tangent);
    finalWaveResult.binormal = normalize(finalWaveResult.binormal);

    output_.worldNormalAndHeight.w = finalWaveResult.position.z - output_position.z;
    output_position = vec4(finalWaveResult.position, 1.0);
    output_.positionWorld = u_Model * output_position;
    output_.positionView = u_View * output_.positionWorld;
    output_position = u_Projection * output_.positionView;
    output_.screenPosition = output_position;
    output_.normal = normalize(mat3(u_Model) * finalWaveResult.normal);
    output_.worldNormalAndHeight.xyz = output_.normal;
    output_.normal = normalize((u_View * vec4(output_.normal, 0.0)).xyz);
    output_.tangent = normalize(mat3(u_Model) * finalWaveResult.tangent);
    output_.tangent = normalize((u_View * vec4(output_.tangent, 0.0)).xyz);
    output_.binormal = normalize(mat3(u_Model) * finalWaveResult.binormal);
    output_.binormal = normalize((u_View * vec4(output_.binormal, 0.0)).xyz);

    gl_Position = output_position;
}
