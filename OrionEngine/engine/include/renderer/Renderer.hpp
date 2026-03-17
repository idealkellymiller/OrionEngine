#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Vertex.hpp"
#include "OBJLoader.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "RenderScene.hpp"
#include "Frustum.hpp"
#include "DirectionalLight.hpp"
#include "RenderPass.hpp"
#include "Framebuffer.hpp"



class IRenderBackend;
class Shader;
class Mesh;
class Material;
class Camera;


class Renderer {
public:
	static bool Init();
	static void Shutdown();

	static void SetViewport(int x, int y, int width, int height);
	static void SetClearColor(float r, float g, float b, float a);

	static void BeginFrame();
	static void EndFrame();

	static void Render(const RenderScene& scene);

	static void DrawMesh(Mesh& mesh, Material& material, const glm::mat4& modelMatrix);

	static void SetShadowShader(Shader* shader) { s_ShadowShader = shader; }

private:

	struct DrawCommand {
		Mesh* MeshPtr = nullptr;
		Material* MaterialPtr = nullptr;
		glm::mat4 ModelMatrix = glm::mat4(1.0f);
		float CameraDistance = 0.0f;
	};

	struct RenderPassDesc {
		RenderPassType Type = RenderPassType::Opaque;
	};

	static void BuildRenderQueue(const RenderScene& scene);
	static void ClearQueues();

	// Returns true if the renderable is visible to the camera frustum.
	static bool IsRenderableVisible(const Renderable& renderable, const Frustum& frustum);
	// Apply material state before drawing
	static void ApplyMaterialRenderState(const Material& material);

	// New uniform upload stages.
	static void UploadFrameUniforms(Shader& shader, const Camera& camera);
	static void UploadLightingUniforms(Shader& shader);
	static void UploadObjectUniforms(Shader& shader, const glm::mat4& modelMatrix);

	// Actual OpenGL drawsubmission.
	static void IssueDraw(const Mesh& mesh);
	// Help for "Shader changed" pass setup
	static void SetupShaderForPass(Shader& shader, const Camera& camera);

	// Pass abstraction helpers.
	static void ExecutePass(const RenderPassDesc& pass, std::vector<DrawCommand>& queue, const Camera& camera);
	static void SetupPass(const RenderPassDesc& pass);
	static void TeardownPass(const RenderPassDesc& pass);
	static void SortDrawQueue(const RenderPassDesc& pass, std::vector<DrawCommand>& queue);

	// Shadow system
	static void InitShadowResources();
	static void ShutdownShadowResources();
	static void ExecuteShadowPass();
	static glm::mat4 BuildLightSpaceMatrix();

private:
	// static IRenderBackend* s_Backend;
	
	// Store the clear color so the backend has a consistent state.
	static float m_ClearColor[4];

	static int s_WindowWidth;
	static int s_WindowHeight;

	// internal queues built fresh every frame
	static std::vector<DrawCommand> s_OpaqueQueue;
	static std::vector<DrawCommand> s_TransparentQueue;

	// Cached scene lighting for the current frame.
	static DirectionalLight s_DirectionalLight;
	static bool s_HasDirectionalLight;

	// Shadow map GPU resources
	static unsigned int s_ShadowFBO;
	static unsigned int s_ShadowDepthTexture;
	// Cached light-space transform used by both shadow pass and main pass.
	static glm::mat4 s_LightSpaceMatrix;
	// Shadow map resolution
	static int s_ShadowMapWidth;
	static int s_ShadowMapHeight;
	// Shadow-only shader
	static Shader* s_ShadowShader;

};