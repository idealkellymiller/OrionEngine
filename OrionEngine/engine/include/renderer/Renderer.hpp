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
#include "DrawCommand.hpp"



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

	static void SetShadowShader(Shader* shader) { s_ShadowShader = shader; }

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

private:
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