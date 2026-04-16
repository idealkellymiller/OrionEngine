#pragma once
#include "EngineCore.h"

#include <vector>
#include <glm/glm.hpp>
#include "DrawCommand.h"

namespace Orion {


class Camera;
class Shader;
class Mesh;
class Material;
struct DirectionalLight;
struct PointLight;




    // Describes which kind of render pass we are running.
    enum class ORION_API RenderPassType
    {
        Opaque = 0,
        Transparent,
        // Shadow
    };

    struct ORION_API RenderPassDesc
    {
        RenderPassType Type = RenderPassType::Opaque;
    };

    class ORION_API RenderPass {
    public:
        // Runs one main forward-rendering pass (opaque or transparent).
        static void ExecutePass(
            const RenderPassDesc& pass,
            std::vector<DrawCommand>& queue,
            const Camera& camera,
            const DirectionalLight& directionalLight,
            bool hasDirectionalLight,
            unsigned int shadowDepthTexture,
            const glm::mat4& lightSpaceMatrix,
            const std::vector<PointLight>& pointLights = {}
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

        // Runs picking pass (writes EntityID instead of shading)
        static void ExecutePickingPass(
            const std::vector<DrawCommand>& queue,
            Shader* pickingShader,
            const Camera& camera
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
            const glm::mat4& lightSpaceMatrix,
            const std::vector<PointLight>& pointLights
        );

        static void UploadFrameUniforms(Shader& shader, const Camera& camera);

        static void UploadLightingUniforms(
            Shader& shader,
            const DirectionalLight& directionalLight,
            bool hasDirectionalLight,
            unsigned int shadowDepthTexture,
            const glm::mat4& lightSpaceMatrix,
            const std::vector<PointLight>& pointLights
        );

        static void UploadObjectUniforms(Shader& shader, const glm::mat4& modelMatrix);
        static void ApplyMaterialRenderState(const Material& material);
        static void IssueDraw(const Mesh& mesh);


        static void UploadPickingUniforms(Shader& shader, const glm::mat4& modelMatrix, EntityID entityID);
    };
}