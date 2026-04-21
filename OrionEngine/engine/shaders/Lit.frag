#version 460 core

#define MAX_POINT_LIGHTS 16

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLightData
{
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

struct MaterialData
{
    vec4 Color;
    vec3 SpecularColor;
    float Shininess;
};

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_UV;
in vec4 v_LightSpacePos;

out vec4 FragColor;

uniform sampler2D u_DiffuseTexture;
uniform sampler2D u_ShadowMap;
uniform int u_UseTexture;

uniform vec3 u_CameraPos;
uniform int u_HasDirectionalLight;
uniform DirectionalLight u_DirectionalLight;
uniform MaterialData u_Material;

uniform int u_NumPointLights;
uniform PointLightData u_PointLights[MAX_POINT_LIGHTS];

// Convert sRGB color to linear space for correct lighting math.
vec3 SRGBToLinear(vec3 srgb)
{
    return pow(srgb, vec3(2.2));
}

// PCF (Percentage-Closer Filtering) soft shadows.
// Samples a 5x5 grid around the projected position and averages the results.
float ComputeShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map depth range = not shadowed.
    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // Slope-scaled bias to reduce shadow acne.
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.00005);

    // PCF: sample a 5x5 neighborhood and average.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    return shadow;
}

void main()
{
    // Decode material base color from sRGB to linear space first.
    // This must happen before any multiplication so each input is decoded exactly once.
    vec4 baseColor = u_Material.Color;
    baseColor.rgb = SRGBToLinear(baseColor.rgb);
    vec3 specColor = SRGBToLinear(u_Material.SpecularColor);

    // Multiply by texture if one is used.
    // Texture is also sRGB — decode it once before blending with the base color.
    if (u_UseTexture == 1)
    {
        vec4 texColor = texture(u_DiffuseTexture, v_UV);
        texColor.rgb = SRGBToLinear(texColor.rgb);
        baseColor *= texColor;
    }

    vec3 normal = normalize(v_WorldNormal);
    vec3 viewDir = normalize(u_CameraPos - v_WorldPos);

    // Small ambient term.
    vec3 ambient = 0.15 * baseColor.rgb;
    vec3 lighting = ambient;

    if (u_HasDirectionalLight == 1)
    {
        vec3 lightDir = normalize(-u_DirectionalLight.direction);

        // Diffuse lighting.
        float NdotL = max(dot(normal, lightDir), 0.0);
        vec3 diffuse =
            NdotL *
            u_DirectionalLight.color *
            u_DirectionalLight.intensity *
            baseColor.rgb;

        // Blinn-Phong specular.
        vec3 halfDir = normalize(lightDir + viewDir);

        float spec = pow(max(dot(normal, halfDir), 0.0), u_Material.Shininess);
        vec3 specular =
            spec *
            specColor *
            u_DirectionalLight.color *
            u_DirectionalLight.intensity;

        float shadow = ComputeShadow(v_LightSpacePos, normal, lightDir);

        // Ambient remains; direct light is shadowed.
        lighting += (1.0 - shadow) * (diffuse + specular);
    }

    // Point lights
    for (int i = 0; i < u_NumPointLights; i++)
    {
        vec3 lightVec = u_PointLights[i].position - v_WorldPos;
        float distance = length(lightVec);
        vec3 lightDir = normalize(lightVec);

        // Attenuation
        float attenuation = 1.0 / (
            u_PointLights[i].constant +
            u_PointLights[i].linear * distance +
            u_PointLights[i].quadratic * distance * distance
        );

        // Diffuse
        float NdotL = max(dot(normal, lightDir), 0.0);
        vec3 diffuse =
            NdotL *
            u_PointLights[i].color *
            u_PointLights[i].intensity *
            baseColor.rgb;

        // Blinn-Phong specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), u_Material.Shininess);
        vec3 specular =
            spec *
            specColor *
            u_PointLights[i].color *
            u_PointLights[i].intensity;

        lighting += attenuation * (diffuse + specular);
    }

    // Output linear HDR color — tone mapping pass handles gamma + exposure.
    FragColor = vec4(lighting, baseColor.a);
}
