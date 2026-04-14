#version 460 core

out vec4 FragColor;

uniform vec3 u_WireColor;

void main()
{
    FragColor = vec4(u_WireColor, 1.0);
}
