#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "DrawCommand.hpp"

class Camera;
class Shader;
class Mesh;
class Material;
struct DirectionalLight;

// Describes which kind of render pass we are running.
enum class RenderPassType
{
    Opaque = 0,
    Transparent,
    // Shadow
};

struct RenderPassDesc
{
    RenderPassType Type = RenderPassType::Opaque;
};

class RenderPass {
public:
    // Runs one main forward-rendering pass (opaque or transparent).
    static void ExecutePass(
        const RenderPassDesc& pass,
        std::vector<DrawCommand>& queue,
        const Camera& camera,
        const DirectionalLight& directionalLight,
        bool hasDirectionalLight,
        unsigned int shadowDepthTexture,
        const glm::mat4& lightSpaceMatrix
    );

    // Runs the directional-light shadow pass.
    static void ExecuteShadowPass(
        const std::vector<DrawCommand>& queue,
        Shader* shadowShader,
        bool hasDirectionalLight,
        unsigned int shadowFBO,
        int shadowMapWidth,
        int shadowMapHeight,
        const glm::mat4& lightSpaceMatrix
    );

private:
    static void SetupPass(const RenderPassDesc& pass);
    static void TeardownPass(const RenderPassDesc& pass);

    static void SetupShaderForPass(
        Shader& shader,
        const Camera& camera,
        const DirectionalLight& directionalLight,
        bool hasDirectionalLight,
        unsigned int shadowDepthTexture,
        const glm::mat4& lightSpaceMatrix
    );

    static void UploadFrameUniforms(Shader& shader, const Camera& camera);

    static void UploadLightingUniforms(
        Shader& shader,
        const DirectionalLight& directionalLight,
        bool hasDirectionalLight,
        unsigned int shadowDepthTexture,
        const glm::mat4& lightSpaceMatrix
    );

    static void UploadObjectUniforms(Shader& shader, const glm::mat4& modelMatrix);
    static void ApplyMaterialRenderState(const Material& material);
    static void IssueDraw(const Mesh& mesh);
};