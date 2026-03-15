#version 460 core

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
    float intensity;
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

float ComputeShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir)
{
    // Convert clip-space position to normalized device coordinates.
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert from [-1, 1] to [0, 1].
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map depth range = not shadowed.
    if (projCoords.z > 1.0)
    {
        return 0.0;
    }

    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias reduces self-shadowing acne.
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.00005);

    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}

void main()
{
    // Start with material base color.
    vec4 baseColor = u_Material.Color;

    // Multiply by texture if one is used.
    if (u_UseTexture == 1)
    {
        baseColor *= texture(u_DiffuseTexture, v_UV);
    }

    vec3 normal = normalize(v_WorldNormal);

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
        vec3 viewDir = normalize(u_CameraPos - v_WorldPos);
        vec3 halfDir = normalize(lightDir + viewDir);

        float spec = pow(max(dot(normal, halfDir), 0.0), u_Material.Shininess);
        vec3 specular =
            spec *
            u_Material.SpecularColor *
            u_DirectionalLight.color *
            u_DirectionalLight.intensity;

        float shadow = ComputeShadow(v_LightSpacePos, normal, lightDir);

        // Ambient remains; direct light is shadowed.
        lighting += (1.0 - shadow) * (diffuse + specular);
    }

    FragColor = vec4(lighting, baseColor.a);
}