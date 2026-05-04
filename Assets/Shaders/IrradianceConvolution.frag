#version 460 core

layout(location = 0) in vec3 out_LocalPos;

uniform samplerCube u_EnvironmentMap;

out vec4 out_Color;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(out_LocalPos);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    const float sampleDelta = 0.025;
    vec3 irradiance = vec3(0.0);
    float sampleCount = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(u_EnvironmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            sampleCount += 1.0;
        }
    }

    irradiance = PI * irradiance / max(sampleCount, 1.0);
    out_Color = vec4(irradiance, 1.0);
}
