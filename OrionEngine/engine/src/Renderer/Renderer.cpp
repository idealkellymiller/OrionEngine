#include "Renderer.hpp"

#include "Shader.hpp"
#include "VertexArray.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Camera.hpp"
#include "RenderScene.hpp"
#include "RenderPass.hpp"
#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>	// for std::sort
#include <glm/glm.hpp>
#include <glad/glad.h>


// Static declarations.
float Renderer::m_ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

int Renderer::s_WindowWidth;
int Renderer::s_WindowHeight;

Shader Renderer::s_LitShader;

std::vector<DrawCommand> Renderer::s_OpaqueQueue;
std::vector<DrawCommand> Renderer::s_TransparentQueue;

DirectionalLight Renderer::s_DirectionalLight;
bool Renderer::s_HasDirectionalLight;

unsigned int Renderer::s_ShadowFBO = 0;
unsigned int Renderer::s_ShadowDepthTexture = 0;
glm::mat4 Renderer::s_LightSpaceMatrix = glm::mat4(1.0f);
int Renderer::s_ShadowMapWidth = 2048;
int Renderer::s_ShadowMapHeight = 2048;
Shader Renderer::s_ShadowShader;

bool Renderer::Init() {

	InitShadowResources();

	// Enables depth testing.
	// This tells OpenGL to compate fragment depth values so that.
	// closer objects appear in front of farther ones.
	glEnable(GL_DEPTH_TEST);

	// Set the depth comparison rule.
	// GL_LESS means a fragment passes if its depth is less than
	// the value already stored in the depth buffer.
	glDepthFunc(GL_LESS);

	// Set the initial clear color.
	// glClearColor does NOT immediately change the screen.
	// It onlt tells OpenGL waht color to use next time the
	// color buffer is cleared with glClear(GL_COLOR_BUFFER_BIT).
	glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);



	// Create the Shaders.
	Shader litShader;
	if (!litShader.CreateFromFiles(
		"C:/dev/OrionRenderer/engine/engineAssets/shaders/Lit.vert",
		"C:/dev/OrionRenderer/engine/engineAssets/shaders/Lit.frag"))
	{
		std::cout << "Failed to create lit Shader\n";
	}

	Shader shadowShader;
	if (!shadowShader.CreateFromFiles(
		"C:/dev/OrionRenderer/engine/engineAssets/shaders/Shadow.vert",
		"C:/dev/OrionRenderer/engine/engineAssets/shaders/Shadow.frag"))
	{
			std::cout << "Failed to create shadow Shader\n";
	}

	

	s_LitShader = litShader;
	s_ShadowShader = shadowShader;
	

	return true; //s_Backend->Init();
}

void Renderer::Shutdown()
{

	ShutdownShadowResources();
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
	s_WindowWidth = width;
	s_WindowHeight = height;


	glViewport(x, y, width, height);
}

void Renderer::SetClearColor(float r, float g, float b, float a)
{
	// Update the stored clear color.
	m_ClearColor[0] = r;
	m_ClearColor[1] = g;
	m_ClearColor[2] = b;
	m_ClearColor[3] = a;

	// Stores the RGBA color OpenGL should use during a color-buffer clear.
	// Again, this does not paint the window by itself.
	glClearColor(r, g, b, a);
}

void Renderer::BeginFrame()
{
	glEnable(GL_DEPTH_TEST);

	// Clear the color buffer using the color previously set by glClearColor.
	// The color buffer is what ends up being displayed on screen.
	//
	// Clears the depth buffer as well.
	// The depth buffer stores per-pixel depth information used for
	// proper 3D visibilty testing.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Read the Json .scene
	// 
	// Load the assets used in it from their files (only once. Don't want to load erato every frame):
	// - Textures
	// - Meshes
	// 
	// Create the Meshes and Materials
	// Collect Rendables into a RenderScene from Scene
	RenderScene scene;
}

void Renderer::EndFrame()
{
	// Nothing here rn
	// TODO: add frame stats 
}

void Renderer::Render(const RenderScene& scene)
{
	Camera* camera = scene.GetActiveCamera();
	if (camera == nullptr)
		return;

	// Cache scene lighting for this frame.
	s_HasDirectionalLight = scene.HasDirectionalLight();
	if (s_HasDirectionalLight) {
		s_DirectionalLight = scene.GetDirectionalLight();
	}

	// Start from a clean frame state
	ClearQueues();

	// Build internal draw commands from the submitted scene.
	BuildRenderQueue(scene);

	// Sort once before running passes.
	SortDrawQueues();

	// Build the light-space matrix once for both shadow and main passes.
	if (s_HasDirectionalLight)
	{
		s_LightSpaceMatrix = BuildLightSpaceMatrix();
	}

	// Shadow pass first so the main pass can sample the shadow map.
	RenderPass::ExecuteShadowPass(
		reinterpret_cast<const std::vector<DrawCommand>&>(s_OpaqueQueue),
		&s_ShadowShader,
		s_HasDirectionalLight,
		s_ShadowFBO,
		s_ShadowMapWidth,
		s_ShadowMapHeight,
		s_LightSpaceMatrix
	);

	RenderPassDesc opaquePass;
	opaquePass.Type = RenderPassType::Opaque;
	RenderPassDesc transparentPass;
	transparentPass.Type = RenderPassType::Transparent;

	RenderPass::ExecutePass(
		opaquePass,
		reinterpret_cast<std::vector<DrawCommand>&>(s_OpaqueQueue),
		*camera,
		s_DirectionalLight,
		s_HasDirectionalLight,
		s_ShadowDepthTexture,
		s_LightSpaceMatrix
	);

	RenderPass::ExecutePass(
		transparentPass,
		reinterpret_cast<std::vector<DrawCommand>&>(s_TransparentQueue),
		*camera,
		s_DirectionalLight,
		s_HasDirectionalLight,
		s_ShadowDepthTexture,
		s_LightSpaceMatrix
	);

	ClearQueues();
}

void Renderer::BuildRenderQueue(const RenderScene& scene)
{
	Camera* camera = scene.GetActiveCamera();
	if (!camera)
		return;

	

	// Build frustum once for this frame.
	// We use it to reject objects outside of the visible region
	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 projection = camera->GetProjectionMatrix();
	glm::mat4 viewProjection = projection * view;

	Frustum frustum;
	frustum.Build(viewProjection);

	const std::vector<Renderable>& renderables = scene.GetRenderables();

	for (const Renderable& renderable : renderables) {

		// Skip invalid entries or culled objects before creating draw commands.
		if (!ShouldSubmitRenderable(renderable, frustum)) 
			continue;

		// Convert scene submission into renderer-owned frame data
		DrawCommand cmd = BuildDrawCommand(renderable, *camera);

		// Send the command into the correct queue
		ClassifyDrawCommand(cmd);
	}
}

bool Renderer::ShouldSubmitRenderable(const Renderable& renderable, const Frustum& frustum)
{
	// Skip bad scene entries
	if (!renderable.IsValid())
		return false;

	// Skip invisible objects outside the camera frustum.
	if (!IsRenderableVisible(renderable, frustum))
		return false;

	return true;
}

DrawCommand Renderer::BuildDrawCommand(const Renderable& renderable, const Camera& camera)
{
	DrawCommand cmd;
	cmd.MeshPtr = renderable.MeshPtr;
	cmd.MaterialPtr = renderable.MaterialPtr;
	cmd.ModelMatrix = renderable.ModelMatrix;

	// Extract world position from the matrix translation column
	glm::vec3 objectWorldPos = glm::vec3(renderable.ModelMatrix[3]);

	// Used for transparent back-to-front sorting
	cmd.CameraDistance = glm::length(camera.GetPosition() - objectWorldPos);

	// Placeholder for future shadow control
	cmd.CastsShadows = true;

	// Placeholder for future packed sort key
	cmd.SortKey = 0;

	return cmd;
}

void Renderer::ClassifyDrawCommand(const DrawCommand& cmd)
{
	if (cmd.MaterialPtr->IsTransparent()) {
		// Transparent objects are handled separately so they can be sorted back-to-front.
		s_TransparentQueue.push_back(cmd);
	}
	else {
		// Opaque objects are grouped for front-end state reduction.
		s_OpaqueQueue.push_back(cmd);
	}
}

bool Renderer::IsRenderableVisible(const Renderable& renderable, const Frustum& frustum)
{
	const BoundingSphere& localBounds = renderable.MeshPtr->GetBounds();

	// Transform local center into world space.
	glm::vec4 worldCenter4 = renderable.ModelMatrix * glm::vec4(localBounds.Center, 1.0f);
	glm::vec3 worldCenter = glm::vec3(worldCenter4);

	// Extract scale from model matrix basis vectors.
	// The length of each basis vector gives scale on that axis.
	float scaleX = glm::length(glm::vec3(renderable.ModelMatrix[0]));
	float scaleY = glm::length(glm::vec3(renderable.ModelMatrix[1]));
	float scaleZ = glm::length(glm::vec3(renderable.ModelMatrix[2]));

	// use the largest scale so the sphere still fully conatins the mesh.
	float maxScale = std::max(scaleX, std::max(scaleY, scaleZ));
	float worldRadius = localBounds.Radius * maxScale;

	return frustum.IntersectsSphere(worldCenter, worldRadius);
}

void Renderer::SortDrawQueues()
{
	std::sort(s_OpaqueQueue.begin(), s_OpaqueQueue.end(),
		[](const DrawCommand& a, const DrawCommand& b)
		{
			// First try to reduce material changes.
			if (a.MaterialPtr != b.MaterialPtr)
				return a.MaterialPtr < b.MaterialPtr;

			// Then reduce mesh/VAO changes.
			if (a.MeshPtr != b.MeshPtr)
				return a.MeshPtr < b.MeshPtr;

			return false;
		});

	std::sort(s_TransparentQueue.begin(), s_TransparentQueue.end(),
		[](const DrawCommand& a, const DrawCommand& b)
		{
			// Back-to-front for alpha blending.
			return a.CameraDistance > b.CameraDistance;
		});
}

void Renderer::ClearQueues()
{
	s_OpaqueQueue.clear();
	s_TransparentQueue.clear();
	// s_HasDirectionalLight = false;
}

// ----- Shadows and Shit ------
void Renderer::InitShadowResources()
{
	// Create framebuffer object that will hold shadow depth texture.
	glGenFramebuffers(1, &s_ShadowFBO);

	// Create the depth texture that stores light-space depth.
	glGenTextures(1, &s_ShadowDepthTexture);
	glBindTexture(GL_TEXTURE_2D, s_ShadowDepthTexture);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH_COMPONENT,
		s_ShadowMapWidth,
		s_ShadowMapHeight,
		0,
		GL_DEPTH_COMPONENT,
		GL_FLOAT,
		nullptr);

	// Depth texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp to border so samples outside of the map read a known value.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Attach the depth texture to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, s_ShadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_ShadowDepthTexture, 0);

	// Shadow pass does not render color
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ShutdownShadowResources()
{
	if (s_ShadowDepthTexture != 0) {
		glDeleteTextures(1, &s_ShadowDepthTexture);
		s_ShadowDepthTexture = 0;
	}

	if (s_ShadowFBO != 0) {
		glDeleteFramebuffers(1, &s_ShadowFBO);
		s_ShadowFBO = 0;
	}
}

glm::mat4 Renderer::BuildLightSpaceMatrix()
{
	if (!s_HasDirectionalLight) {
		return glm::mat4(1.0f);
	}

	// Normalize light direction so our light "camera" is stable
	glm::vec3 lightDir = glm::normalize(s_DirectionalLight.Direction);

	// Pick a point looking toward the origin for now.
	// Later this should track camera frustum better.
	glm::vec3 lightPos = -lightDir * 20.0f;
	glm::vec3 target = glm::vec3(0.0f);

	glm::mat4 lightView = glm::lookAt(
		lightPos,
		target,
		glm::vec3(0.0f, 1.0f, 0.0f));

	// Orthographic projection is typical for directional shadow maps.
	float orthoRange = 20.0f;
	glm::mat4 lightProjection = glm::ortho(
		-orthoRange, orthoRange,
		-orthoRange, orthoRange,
		1.0f, 50.0f);

	return lightProjection * lightView;
}
