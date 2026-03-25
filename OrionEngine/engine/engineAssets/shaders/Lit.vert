#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_UV;
out vec4 v_LightSpacePos;

void main()
{
    // Transform vertex position into world space.
    vec4 worldPos = u_Model * vec4(a_Pos, 1.0);
    v_WorldPos = worldPos.xyz;

    // Transform normal into world space.
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    v_WorldNormal = normalize(normalMatrix * a_Normal);

    v_UV = a_UV;

    // Used later for shadow lookup.
    v_LightSpacePos = u_LightSpaceMatrix * worldPos;

    // Standard clip-space position.
    gl_Position = u_Projection * u_View * worldPos;
}