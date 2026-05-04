#version 460 core

in vec3 in_Pos;

uniform mat4 u_View;
uniform mat4 u_Projection;

uniform vec3 u_WorldEast;
uniform vec3 u_WorldNorth;
uniform vec3 u_WorldUp;

layout(location = 0) out vec3 out_WorldDir;

void main()
{
    vec3 worldPos = in_Pos.x * u_WorldEast + in_Pos.y * u_WorldNorth + in_Pos.z * u_WorldUp;
    out_WorldDir = normalize(worldPos);

    mat4 viewRotOnly = mat4(mat3(u_View));
    
    vec4 screenPos = u_Projection * viewRotOnly * vec4(worldPos, 1.0);
    gl_Position = screenPos.xyww;
}

