#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/Camera.h"
#include "Renderer/Mesh.h"
#include "Renderer/Vertex.h"
#include "Renderer/OBJLoader.h"
#include "Renderer/Texture.h"
#include "Renderer/Material.h"
#include "Renderer/RenderScene.h"
#include "Renderer/Frustum.h"
#include "Renderer/DirectionalLight.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/DrawCommand.h"

#include "ECS/Scene.h"



// class IRenderBackend;
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

	static void Render();

	static Shader* GetLitShader() { return &s_LitShader; }
	static Shader* GetShadowShader() { return &s_ShadowShader; }

	static Camera* GetActiveCamera() { return &s_ActiveCamera; } 
	static void SetActiveCamera(Camera camera) { s_ActiveCamera = camera; }

	static EntityID PickEntity(int mouseX, int mouseY);

private:
	static void BuildRenderQueue(const RenderScene& scene);
	static bool ShouldSubmitRenderable(const Renderable& renderable, const Frustum& frustum);
	static DrawCommand BuildDrawCommand(const Renderable& renderabl, const Camera& camera);
	static void ClassifyDrawCommand(const DrawCommand& cmd);
	static void ClearQueues();

	// Returns true if the renderable is visible to the camera frustum.
	static bool IsRenderableVisible(const Renderable& renderable, const Frustum& frustum);
	static void SortDrawQueues();

	// Shadow resource lifetime.
	static void InitShadowResources();
	static void ShutdownShadowResources();
	static glm::mat4 BuildLightSpaceMatrix();

	// Picking pass
	static void InitPickingResources();
	static void ShutdownPickingResources();
	static void ResizePickingResources(int width, int height);
	static void RenderPickingPass();

private:
	// Store the clear color so the backend has a consistent state.
	static float m_ClearColor[4];

	static int s_WindowWidth;
	static int s_WindowHeight;

	static Shader s_LitShader;
	static Shader s_ShadowShader;
	static Shader s_PickingShader;

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

	static Camera s_ActiveCamera;

	static RenderScene s_ActiveRenderScene;

	// Picking framebuffer and attachments
	static unsigned int s_PickingFBO;
	static unsigned int s_PickingColorTexture;	// stores EntityID (interger texture)
	static unsigned int s_PickingDepthRBO;
};