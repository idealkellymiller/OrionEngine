#include "Renderer.hpp"
#include "IRenderBackend.hpp"
#include "OpenGLBackend.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Camera.hpp"
#include "RenderScene.hpp"
#include "Renderable.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>	// for std::sort
#include <glm/glm.hpp>
#include <glad/glad.h>


// Static member definitions
IRenderBackend* Renderer::s_Backend = nullptr;
int Renderer::s_WindowWidth;
int Renderer::s_WindowHeight;
std::vector<Renderer::DrawCommand> Renderer::s_OpaqueQueue;
std::vector<Renderer::DrawCommand> Renderer::s_TransparentQueue;

DirectionalLight Renderer::s_DirectionalLight;
bool Renderer::s_HasDirectionalLight;

unsigned int Renderer::s_ShadowFBO = 0;
unsigned int Renderer::s_ShadowDepthTexture = 0;
glm::mat4 Renderer::s_LightSpaceMatrix = glm::mat4(1.0f);
int Renderer::s_ShadowMapWidth = 2048;
int Renderer::s_ShadowMapHeight = 2048;
Shader* Renderer::s_ShadowShader = nullptr;

bool Renderer::Init() {
	// Create the backend object.
	// For now, we are hardcoding OpenGL.
	// Later this could be chosen from config, platform settings, etc.
	s_Backend = new OpenGLBackend();

	InitShadowResources();

	return s_Backend->Init();
}

void Renderer::Shutdown()
{
	if (s_Backend){
		s_Backend->Shutdown();
		delete s_Backend;
		s_Backend = nullptr;
	}

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
	if (s_Backend) 
		s_Backend->SetClearColor(r, g, b, a);
}

void Renderer::BeginFrame()
{
	if (s_Backend) 
		s_Backend->BeginFrame();
}

void Renderer::EndFrame()
{
	if (s_Backend) 
		s_Backend->EndFrame();
}

void Renderer::DrawMesh(Mesh& mesh, Material& material, const glm::mat4& modelMatrix)
{
	// Apply fixed-function/OpenGL pipeline state like depth test, blending, and culling before binding and drawing.
	ApplyMaterialRenderState(material);

	// Bind shader, textures, and per-material uniforms
	material.Bind();

	// Ask the material which shader it uses
	Shader* shader = material.GetShader();
	// Validate draw state before touching OpenGL
	if (shader == nullptr || !s_Backend || !mesh.IsValid())
		return;

	// Upload per-object transform data.
	UploadObjectUniforms(*shader, modelMatrix);

	// Submit geometry to OpenGL.
	IssueDraw(mesh);
}

void Renderer::Render(const RenderScene& scene)
{
	Camera* camera = scene.GetActiveCamera();
	if (camera == nullptr)
		return;

	// Cache scene lighting for this frame
	s_HasDirectionalLight = scene.HasDirectionalLight();
	if (s_HasDirectionalLight) {
		s_DirectionalLight = scene.GetDirectionalLight();
	}

	// Build internal queues from the submitted renderables
	BuildRenderQueue(scene);

	// Render light-space depth first
	ExecuteShadowPass();

	// Return viewport  for normal rendering to the window framebuffer
	SetViewport(0, 0, s_WindowWidth, s_WindowHeight);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Describe the passes we want to run this frame.
	RenderPassDesc opaquePass;
	opaquePass.Type = RenderPassType::Opaque;

	RenderPassDesc transparentPass;
	transparentPass.Type = RenderPassType::Transparent;

	// Execute passes in forward-rendering order
	ExecutePass(opaquePass, s_OpaqueQueue, *camera);
	ExecutePass(transparentPass, s_TransparentQueue, *camera);

	// Clear temporary per-frame queues.
	ClearQueues();
}

void Renderer::BuildRenderQueue(const RenderScene& scene)
{
	Camera* camera = scene.GetActiveCamera();
	if (!camera)
		return;

	// Build frustum once for the whole frame
	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 projection = camera->GetProjectionMatrix();
	glm::mat4 viewProjection = projection * view;

	Frustum frustum;
	frustum.Build(viewProjection);

	// Cache scene lighting for this frame
	s_HasDirectionalLight = scene.HasDirectionalLight();
	if (s_HasDirectionalLight) {
		s_DirectionalLight = scene.GetDirectionalLight();
	}

	const std::vector<Renderable>& renderables = scene.GetRenderables();

	for (const Renderable& renderable : renderables) {

		// Skip invalid entries
		if (!renderable.IsValid()) {
			continue;
		}

		// Skip invisible (out of view) objects before they enter render queues.
		if (!IsRenderableVisible(renderable, frustum)) {
			continue;
		}


		DrawCommand cmd;
		cmd.MeshPtr = renderable.MeshPtr;
		cmd.MaterialPtr = renderable.MaterialPtr;
		cmd.ModelMatrix = renderable.ModelMatrix;

		// Extract object world position from model matrix.
		// In  standard transform matrix, column 3 stores tranlation.
		glm::vec3 objectWorldPos = glm::vec3(renderable.ModelMatrix[3]);

		// Compute distance to camer afor future sorting.
		cmd.CameraDistance = glm::length(camera->GetPosition() - objectWorldPos);

		if (renderable.MaterialPtr->IsTransparent()) {
			s_TransparentQueue.push_back(cmd);
		}
		else {
			s_OpaqueQueue.push_back(cmd);
		}
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

void Renderer::SetupPass(const RenderPassDesc& pass)
{
	switch (pass.Type) {
	case RenderPassType::Opaque: {
		// Opaque pass usually writes depth and does not blend
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		break;
	}

	case RenderPassType::Transparent: {
		// Transparent pass usually keeps depth testing on,
		// but most materials will disable depth writes.
		// We do not force blending on here because material state
		// still controls the exact blend behavior
		break;
	}
	}
}

void Renderer::TeardownPass(const RenderPassDesc& pass)
{
	switch (pass.Type) {
	case RenderPassType::Opaque: {
		// Nothing special needed right now.
		break;
	}

	case RenderPassType::Transparent: {
		// Restore safe defaults after transparent rendering.
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		break;
	}
	}
}

void Renderer::SortDrawQueue(const RenderPassDesc& pass, std::vector<DrawCommand>& queue)
{
	switch (pass.Type) {
	case RenderPassType::Opaque: {
		// Opaque objects are sorted to reduce shader/material/mesh changes.
		std::sort(queue.begin(), queue.end(),
			[](const DrawCommand& a, const DrawCommand& b)
			{
				if (a.MaterialPtr != b.MaterialPtr)
					return a.MaterialPtr < b.MaterialPtr;

				return a.MeshPtr < b.MeshPtr;
			});
		break;
	}

	case RenderPassType::Transparent:
	{
		// Transparent objects must render back-to-front for blending.
		std::sort(queue.begin(), queue.end(),
			[](const DrawCommand& a, const DrawCommand& b)
			{
				return a.CameraDistance > b.CameraDistance;
			});
		break;
	}
	}
}

void Renderer::ExecutePass(const RenderPassDesc& pass, std::vector<DrawCommand>& queue, const Camera& camera)
{
	// Configure high-level GPU state for this pass.
	SetupPass(pass);

	// Sort the queue based on the pass rules
	SortDrawQueue(pass, queue);

	Shader* currentShader = nullptr;

	for (const DrawCommand& cmd : queue) {
		if (!cmd.MeshPtr || !cmd.MaterialPtr)
			continue;

		Shader* shader = cmd.MaterialPtr->GetShader();
		if (!shader)
			continue;

		// When switching shaders, upload shared pass/frame data once.
		if (shader != currentShader) {
			shader->Bind();
			SetupShaderForPass(*shader, camera);
			currentShader = shader;
		}

		// Draw this object using its material and model transform
		DrawMesh(*cmd.MeshPtr, *cmd.MaterialPtr, cmd.ModelMatrix);
	}

	// Restore any state this pass may have changed
	TeardownPass(pass);
}

void Renderer::ClearQueues()
{
	s_OpaqueQueue.clear();
	s_TransparentQueue.clear();
	s_HasDirectionalLight = false;
}

void Renderer::SetupShaderForPass(Shader& shader, const Camera& camera)
{
	// Upload data shared by all using this shader in the current pass.
	UploadFrameUniforms(shader, camera);
	UploadLightingUniforms(shader);
}

void Renderer::ApplyMaterialRenderState(const Material& material)
{
	if (s_Backend)
		s_Backend->ApplyMaterialRenderState(material);
}


// ------ Uniforms ------
void Renderer::UploadFrameUniforms(Shader& shader, const Camera& camera)
{
	// Upload camera matrices used by every object this frame.
	shader.SetMat4("u_View", camera.GetViewMatrix());
	shader.SetMat4("u_Projection", camera.GetProjectionMatrix());

	// Camera position is useful for specular lighting and other view-dependent effects.
	shader.SetVec3("u_CameraPos", camera.GetPosition());
}

void Renderer::UploadLightingUniforms(Shader& shader)
{
	if (s_HasDirectionalLight) {
		// Normalize direction so lighting math is stable and predictable
		glm::vec3 lightDir = glm::normalize(s_DirectionalLight.Direction);

		shader.SetInt("u_HasDirectionalLight", 1);
		shader.SetVec3("u_DirectionalLight.direction", lightDir);
		shader.SetVec3("u_DirectionalLight.color", s_DirectionalLight.Color);
		shader.SetFloat("u_DirectionalLight.intensity", s_DirectionalLight.Intensity);
		
		// Upload the matrix used to transform world positions into light space.
		shader.SetMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);

		// Bind the shadow map to texture unit 1.
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, s_ShadowDepthTexture);

		shader.SetInt("u_ShadowMap", 1);
	}
	else {
		// Shader can fall back to ambient-only behavior
		shader.SetInt("u_HasDirectionalLight", 0);
	}
}

void Renderer::UploadObjectUniforms(Shader& shader, const glm::mat4& modelMatrix)
{
	// Model matrix is unique per rendered object.
	shader.SetMat4("u_Model", modelMatrix);
}

void Renderer::IssueDraw(const Mesh& mesh)
{

	if (mesh.HasIndices()) {
		// Draw indexed triangles using the bound element/index buffer.
		s_Backend->DrawIndexed(mesh.GetVertexArray(), mesh.GetIndexCount());
	}
	else {
		// Draw non-indexed triangles directly from the buffer.
		s_Backend->Draw(mesh.GetVertexArray(), mesh.GetVertexCount());
	}
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

void Renderer::ExecuteShadowPass()
{
	if (!s_HasDirectionalLight || !s_ShadowShader)
	{
		return;
	}

	// Save the framebuffer and viewport that were active before the shadow pass.
	// This is important because the main renderer may be targeting an  editor viewport FBO,
	// not the default window framebuffer.
	GLint previousFBO = 0;
	GLint previousViewport[4] = { 0, 0, 0, 0 };

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
	glGetIntegerv(GL_VIEWPORT, previousViewport);

	// Compute and cache the light-space matrix for this frame.
	s_LightSpaceMatrix = BuildLightSpaceMatrix();

	// Bind the shadow framebuffer so rendering writes into the depth texture.
	glViewport(0, 0, s_ShadowMapWidth, s_ShadowMapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, s_ShadowFBO);

	// Clear previous dept hvalues from the shadow map.
	glClear(GL_DEPTH_BUFFER_BIT);

	// Shadow pass typically only need sdepth testing and depth writes.
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Cull front faces sometimes helps reduce shadow acne.
	// this is optional but common for basic first implementations
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	s_ShadowShader->Bind();

	// Upload the light-space matrix once for the whole shadow pass.
	s_ShadowShader->SetMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);

	for (const DrawCommand& cmd : s_OpaqueQueue) {
		if (!cmd.MeshPtr)
			continue;

		// Only model matrix changes per object in the shadow pass.
		s_ShadowShader->SetMat4("u_Model", cmd.ModelMatrix);

		// Bind the geometry and issue the draw.
		IssueDraw(*cmd.MeshPtr);
	}
	
	// Restore defaults
	glCullFace(GL_BACK);

	// Restore the framebuffer and viewport that were active before the shadow rendering.
	glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
	SetViewport(
		previousViewport[0],
		previousViewport[1],
		previousViewport[2],
		previousViewport[3]
	);
}