#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform vec3 u_ColorMultiplier;

out vec3 v_Color;

void main()
{
    // Pass vertex color to fragment shader.
    v_Color = a_Color * u_ColorMultiplier;

    // Transform local gizmo line vertex into clip space.
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}