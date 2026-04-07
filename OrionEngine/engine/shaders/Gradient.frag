#version 460 core

in vec2 v_UV;

out vec4 FragColor;

uniform vec4 u_TopColor;
uniform vec4 u_BottomColor;

void main()
{
    // v_UV.y = 0 at bottom, 1 at top.
    // Lerp from bottom color to top color along the vertical axis.
    FragColor = mix(u_BottomColor, u_TopColor, v_UV.y);
}
