#version 460 core

// Fullscreen triangle trick — no vertex buffer needed.
// Draw with glDrawArrays(GL_TRIANGLES, 0, 3) and an empty VAO.

out vec2 v_UV;

void main()
{
    float x = float((gl_VertexID & 1) << 2) - 1.0;
    float y = float((gl_VertexID & 2) << 1) - 1.0;

    v_UV = vec2(x * 0.5 + 0.5, y * 0.5 + 0.5);

    gl_Position = vec4(x, y, 0.0, 1.0);
}
