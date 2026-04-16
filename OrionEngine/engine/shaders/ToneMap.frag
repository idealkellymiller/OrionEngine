#version 460 core

in vec2 v_UV;

out vec4 FragColor;

uniform sampler2D u_HDRBuffer;
uniform float u_Exposure;

// ACES filmic tone mapping curve.
// Attempt to approximate the look of film stock — rich contrast,
// gentle highlight rolloff, and slightly desaturated bright areas.
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 hdrColor = texture(u_HDRBuffer, v_UV).rgb;

    // Exposure adjustment — lets the artist control overall brightness.
    hdrColor *= u_Exposure;

    // Tone map HDR to [0,1] range.
    vec3 mapped = ACESFilm(hdrColor);

    // Gamma correction (linear → sRGB).
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
