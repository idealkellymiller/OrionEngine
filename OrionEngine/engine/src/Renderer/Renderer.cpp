#include "Renderer/Renderer.h"

#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderScene.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Gizmo.h"

#include "ECS/Scene.h"
#include "ECS/SceneManager.h"

#include "Application.h"

#include "Layers/EditorLayer.h"
#include "Core/ProjectSettings.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>	// for std::sort
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>



namespace Orion {

	// Static declarations.
	float Renderer::m_ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

	int Renderer::s_WindowWidth;
	int Renderer::s_WindowHeight;

	Framebuffer Renderer::s_ViewportFramebuffer;

	Shader Renderer::s_LitShader;
	Shader Renderer::s_ShadowShader;
	Shader Renderer::s_PickingShader;

	std::vector<DrawCommand> Renderer::s_OpaqueQueue;
	std::vector<DrawCommand> Renderer::s_TransparentQueue;

	DirectionalLight Renderer::s_DirectionalLight;
	bool Renderer::s_HasDirectionalLight;

	unsigned int Renderer::s_ShadowFBO = 0;
	unsigned int Renderer::s_ShadowDepthTexture = 0;
	glm::mat4 Renderer::s_LightSpaceMatrix = glm::mat4(1.0f);
	int Renderer::s_ShadowMapWidth = 2048;
	int Renderer::s_ShadowMapHeight = 2048;

	Camera Renderer::s_ActiveCamera;

	RenderScene Renderer::s_ActiveRenderScene;

	unsigned int Renderer::s_PickingFBO = 0;
	unsigned int Renderer::s_PickingColorTexture = 0;
	unsigned int Renderer::s_PickingDepthRBO = 0;

	GizmoPass Renderer::s_GizmoPass;
	DebugPass Renderer::s_DebugPass;

	Shader Renderer::s_GradientShader;
	unsigned int Renderer::s_EmptyVAO = 0;


	bool Renderer::Init() {
		// Create the editor viewport framebuffer
		s_ViewportFramebuffer.Create(1280, 720);


		InitShadowResources();
		InitPickingResources();
		InitGradientResources();

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
			"../engine/shaders/Lit.vert",
			"../engine/shaders/Lit.frag"))
		{
			std::cout << "Failed to create lit Shader\n";
		}

		Shader shadowShader;
		if (!shadowShader.CreateFromFiles(
			"../engine/shaders/Shadow.vert",
			"../engine/shaders/Shadow.frag"))
		{
			std::cout << "Failed to create shadow Shader\n";
		}

		Shader pickingShader;
		if (!pickingShader.CreateFromFiles(
			"../engine/shaders/Picking.vert",
			"../engine/shaders/Picking.frag"))
		{
			std::cout << "Failed to create picking Shader\n";
		}

		s_LitShader = litShader;
		s_ShadowShader = shadowShader;
		s_PickingShader = pickingShader;


		s_GizmoPass.Init();
		s_DebugPass.Init();

		SetClearColor(0.6f, 0.6f, 0.6f, 1.0f);
		printf("Renderer Initialized.\n");
		return true; //s_Backend->Init();
	}

	void Renderer::Shutdown()
	{

		ShutdownShadowResources();
		ShutdownPickingResources();
		ShutdownGradientResources();

		s_GizmoPass.Shutdown();
		s_DebugPass.Shutdown();
	}

	void Renderer::SetViewport(int x, int y, int width, int height)
	{
		s_WindowWidth = width;
		s_WindowHeight = height;


		glViewport(x, y, width, height);

		ResizePickingResources(width, height);
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
		s_ViewportFramebuffer.Bind();

		// Set viewport to framebuffer size
		SetViewport(0, 0, s_ViewportFramebuffer.GetWidth(), s_ViewportFramebuffer.GetHeight());

		glEnable(GL_DEPTH_TEST);

		// --- Background: driven by ProjectSettings ---
		ProjectSettings& settings = ProjectSettings::Get();

		switch (settings.backgroundMode)
		{
			case BackgroundMode::Gradient:
			{
				// Clear depth only — the gradient shader will fill color.
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				RenderGradientBackground();
				break;
			}
			case BackgroundMode::Cubemap:
				// (Future) — fall through to solid color for now.
			case BackgroundMode::SolidColor:
			default:
			{
				glClearColor(
					settings.solidColor.r,
					settings.solidColor.g,
					settings.solidColor.b,
					settings.solidColor.a);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				break;
			}
		}



		// Collect Rendables into a RenderScene from Scene
		RenderScene scene;
		scene.BuildRenderScene();
		s_ActiveRenderScene = scene;

		// Update camera aspect ratio to match viewport.
		// During play mode, RuntimeLayer sets full projection from CameraComponent.
		// During edit mode, apply default editor projection.
		float aspectRatio = (float)s_ViewportFramebuffer.GetWidth() / (float)s_ViewportFramebuffer.GetHeight();
		if (!EditorLayer::IsPlaying()) {
			s_ActiveCamera.SetPerspective(45.0f, aspectRatio, 0.1f, 100.0f);
		} else {
			// Just update aspect ratio, keep FOV/near/far from CameraComponent
			s_ActiveCamera.SetPerspective(
				s_ActiveCamera.GetFOVDegrees(), aspectRatio,
				s_ActiveCamera.GetNearPlane(), s_ActiveCamera.GetFarPlane()
			);
		}
	}

	void Renderer::EndFrame()
	{
		// Nothing here rn
		// TODO: add frame stats


		ClearQueues();


		s_ViewportFramebuffer.Unbind();

		// -------- UI RENDERER --------
		Application& app = Application::Get();
		// io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		// Render ImGui to the actual window backbuffer
		int displayW, displayH;
		glfwGetFramebufferSize(static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow()), &displayW, &displayH);

		// Set viewport back to the real windwo size
		glViewport(0, 0, displayW, displayH);
		// Usually editor background does not need depth test
		glDisable(GL_DEPTH_TEST);

		// Clear the actual screen
		glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw all ImGui windows, including the Viewport image
		//ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void Renderer::BlitToScreen()
	{
		Application& app = Application::Get();
		int displayW, displayH;
		glfwGetFramebufferSize(static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow()), &displayW, &displayH);

		// Resize the viewport framebuffer to match the window if needed
		if ((unsigned int)displayW != s_ViewportFramebuffer.GetWidth() ||
			(unsigned int)displayH != s_ViewportFramebuffer.GetHeight()) {
			s_ViewportFramebuffer.Resize(displayW, displayH);
		}

		// Blit the framebuffer's color attachment to the default framebuffer (screen)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, s_ViewportFramebuffer.GetFBO());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(
			0, 0, s_ViewportFramebuffer.GetWidth(), s_ViewportFramebuffer.GetHeight(),
			0, 0, displayW, displayH,
			GL_COLOR_BUFFER_BIT, GL_LINEAR
		);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Renderer::Render()
	{
		BeginFrame();


		Camera* camera = GetActiveCamera();
		if (camera == nullptr)
			return;

		// Cache scene lighting for this frame.
		s_HasDirectionalLight = s_ActiveRenderScene.HasDirectionalLight();
		if (s_HasDirectionalLight) {
			s_DirectionalLight = s_ActiveRenderScene.GetDirectionalLight();
		}

		// Start from a clean frame state
		ClearQueues();

		// Build internal draw commands from the submitted scene.
		BuildRenderQueue(s_ActiveRenderScene);

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

		// ---- Debug pass: grid + collider wireframe ----
		if (!EditorLayer::IsPlaying()) {
			// World grid
			if (ProjectSettings::Get().showGrid)
				s_DebugPass.DrawGrid(s_ActiveCamera);

			// Collider wireframe for selected entity
			EntityID selected = EditorLayer::GetSelectedEntity();
			if (selected != INVALID_ENTITY) {
				auto scene = SceneManager::GetActiveScene();
				if (scene && scene->HasColliderComponent(selected))
					s_DebugPass.DrawCollider(s_ActiveCamera, selected, *scene);
			}
		}

		// Only render gizmo to screen if an object is selected and not in play mode
		if (EditorLayer::GetSelectedEntity() != INVALID_ENTITY && !EditorLayer::IsPlaying()) {
			auto scene = SceneManager::GetActiveScene();
			EntityID selected = EditorLayer::GetSelectedEntity();
			if (scene->HasTransformComponent(selected)) {
				// Use world transform so gizmo aligns with parented objects
				glm::mat4 worldTransform = scene->GetWorldTransform(selected);

				// Extract world position from column 3
				glm::vec3 worldPos = glm::vec3(worldTransform[3]);

				// Extract world orientation (strip translation and scale)
				glm::mat4 orient(1.0f);
				for (int i = 0; i < 3; i++) {
					glm::vec3 col = glm::vec3(worldTransform[i]);
					float len = glm::length(col);
					if (len > 1e-6f)
						orient[i] = glm::vec4(col / len, 0.0f);
				}

				GizmoData gizmo;
				gizmo.position = worldPos;
				gizmo.orientation = orient;
				gizmo.axisLength = 2.5f;
				gizmo.mode = EditorLayer::GetGizmoMode();
				s_GizmoPass.Execute(s_ActiveCamera, gizmo);
			}
		}

		RenderPickingPass();

		
		EndFrame();
	}

	void Renderer::BuildRenderQueue(const RenderScene& scene)
	{
		//Camera* camera = scene.GetActiveCamera();
		//if (!camera)
		//	return;



		// Build frustum once for this frame.
		// We use it to reject objects outside of the visible region
		glm::mat4 view = s_ActiveCamera.GetViewMatrix();
		glm::mat4 projection = s_ActiveCamera.GetProjectionMatrix();
		glm::mat4 viewProjection = projection * view;

		Frustum frustum;
		frustum.Build(viewProjection);

		const std::vector<Renderable>& renderables = scene.GetRenderables();

		for (const Renderable& renderable : renderables) {

			// !!!!! --- SKIP CULLING FOR NOW UNTIL DISAPPEARING SHADOWS BUG IS FIXED --- !!!!!
			// Skip invalid entries or culled objects before creating draw commands.
			// if (!ShouldSubmitRenderable(renderable, frustum))
			// 	continue;

			// Convert scene submission into renderer-owned frame data
			DrawCommand cmd = BuildDrawCommand(renderable, s_ActiveCamera);

			// Send the command into the correct queue
			ClassifyDrawCommand(cmd);
		}
	}

	bool Renderer::ShouldSubmitRenderable(const Renderable& renderable, const Frustum& frustum)
	{
		// Skip invisible objects outside the camera frustum.
		if (!IsRenderableVisible(renderable, frustum))
			return false;

		return true;
	}

	DrawCommand Renderer::BuildDrawCommand(const Renderable& renderable, const Camera& camera)
	{
		DrawCommand cmd;
		cmd.Entity = renderable.entity;
		cmd.MeshPtr = renderable.mesh;
		cmd.MaterialPtr = renderable.material;
		cmd.ModelMatrix = renderable.worldTransform;

		// Extract world position from the matrix translation column
		glm::vec3 objectWorldPos = glm::vec3(renderable.worldTransform[3]);

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
		const BoundingSphere& localBounds = renderable.mesh->GetBounds();

		// Transform local center into world space.
		glm::vec4 worldCenter4 = renderable.worldTransform * glm::vec4(localBounds.Center, 1.0f);
		glm::vec3 worldCenter = glm::vec3(worldCenter4);

		// Extract scale from model matrix basis vectors.
		// The length of each basis vector gives scale on that axis.
		float scaleX = glm::length(glm::vec3(renderable.worldTransform[0]));
		float scaleY = glm::length(glm::vec3(renderable.worldTransform[1]));
		float scaleZ = glm::length(glm::vec3(renderable.worldTransform[2]));

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


	void Renderer::InitPickingResources() {
		glGenFramebuffers(1, &s_PickingFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, s_PickingFBO);

		// Integer texture storing one EntityID per pixel
		glGenTextures(1, &s_PickingColorTexture);
		glBindTexture(GL_TEXTURE_2D, s_PickingColorTexture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_R32UI,
			std::max(1, s_WindowWidth),
			std::max(1, s_WindowHeight),
			0,
			GL_RED_INTEGER,
			GL_UNSIGNED_INT,
			nullptr);

		// No filtering for ID textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D,
			s_PickingColorTexture,
			0);

		// Depth renderbuffer so nearest visible object wins
		glGenRenderbuffers(1, &s_PickingDepthRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, s_PickingDepthRBO);
		glRenderbufferStorage(
			GL_RENDERBUFFER,
			GL_DEPTH24_STENCIL8,
			std::max(1, s_WindowWidth),
			std::max(1, s_WindowHeight));

		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER,
			GL_DEPTH_STENCIL_ATTACHMENT,
			GL_RENDERBUFFER,
			s_PickingDepthRBO);

		GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Picking framebuffer is incomplete\n";

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Renderer::ShutdownPickingResources()
	{
		if (s_PickingDepthRBO != 0) {
			glDeleteRenderbuffers(1, &s_PickingDepthRBO);
			s_PickingDepthRBO = 0;
		}

		if (s_PickingColorTexture != 0) {
			glDeleteTextures(1, &s_PickingColorTexture);
			s_PickingColorTexture = 0;
		}

		if (s_PickingFBO != 0) {
			glDeleteFramebuffers(1, &s_PickingFBO);
			s_PickingFBO = 0;
		}
	}

	void Renderer::ResizePickingResources(int width, int height)
	{
		width = std::max(1, width);
		height = std::max(1, height);

		if (s_PickingColorTexture != 0) {
			glBindTexture(GL_TEXTURE_2D, s_PickingColorTexture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_R32UI,
				width,
				height,
				0,
				GL_RED_INTEGER,
				GL_UNSIGNED_INT,
				nullptr);
		}

		if (s_PickingDepthRBO != 0) {
			glBindRenderbuffer(GL_RENDERBUFFER, s_PickingDepthRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void Renderer::RenderPickingPass()
	{
		GLint previousFBO = 0;
		GLint previousViewport[4] = { 0, 0, 0, 0 };

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
		glGetIntegerv(GL_VIEWPORT, previousViewport);

		glBindFramebuffer(GL_FRAMEBUFFER, s_PickingFBO);
		glViewport(0, 0, s_WindowWidth, s_WindowHeight);

		// Clear ID target to 0 = no entity
		GLuint clearValue = 0;
		glClearBufferuiv(GL_COLOR, 0, &clearValue);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Render both queues. For picking, transparent can be treated like opaque.
		RenderPass::ExecutePickingPass(
			reinterpret_cast<const std::vector<DrawCommand>&>(s_OpaqueQueue),
			&s_PickingShader,
			s_ActiveCamera
		);

		RenderPass::ExecutePickingPass(
			reinterpret_cast<const std::vector<DrawCommand>&>(s_TransparentQueue),
			&s_PickingShader,
			s_ActiveCamera
		);

		// re-bind back to the editor viewport to be rendered to after rendering this pass to this framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
		glViewport(
			previousViewport[0],
			previousViewport[1],
			previousViewport[2],
			previousViewport[3]
		);
	}

	EntityID Renderer::PickEntity(int mouseX, int mouseY)
	{
		if (s_PickingFBO == 0)
			return 0;

		if (mouseX < 0 || mouseX >= s_WindowWidth || mouseY < 0 || mouseY >= s_WindowHeight)
			return 0;

		// OpenGL framebuffer origin is bottom-left
		const int readY = s_WindowHeight - 1 - mouseY;

		GLuint pickedID = 0;

		// GLint previousFBO = 0;
		// GLint previousViewport[4] = { 0, 0, 0, 0 };

		// glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
		// glGetIntegerv(GL_VIEWPORT, previousViewport);

		glBindFramebuffer(GL_FRAMEBUFFER, s_PickingFBO);
		// glViewport(0, 0, s_WindowHeight, s_WindowHeight);

		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glReadPixels(
			mouseX,
			readY,
			1,
			1,
			GL_RED_INTEGER,
			GL_UNSIGNED_INT,
			&pickedID);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);

		return static_cast<EntityID>(pickedID);
	}


	// ---------- Gradient background ----------

	void Renderer::InitGradientResources()
	{
		// Load the gradient fullscreen shader
		Shader gradientShader;
		if (!gradientShader.CreateFromFiles(
			"../engine/shaders/Gradient.vert",
			"../engine/shaders/Gradient.frag"))
		{
			std::cout << "Failed to create gradient Shader\n";
		}
		s_GradientShader = gradientShader;

		// Create an empty VAO for the fullscreen-triangle trick.
		// The vertex shader generates positions from gl_VertexID alone,
		// so no vertex buffer is needed.
		glGenVertexArrays(1, &s_EmptyVAO);
	}

	void Renderer::ShutdownGradientResources()
	{
		if (s_EmptyVAO != 0) {
			glDeleteVertexArrays(1, &s_EmptyVAO);
			s_EmptyVAO = 0;
		}
	}

	void Renderer::RenderGradientBackground()
	{
		ProjectSettings& settings = ProjectSettings::Get();

		// Disable depth testing entirely — the gradient is just a background fill.
		// Scene geometry rendered afterward will have depth testing re-enabled and
		// will naturally draw in front.
		glDisable(GL_DEPTH_TEST);

		s_GradientShader.Bind();
		s_GradientShader.SetVec4("u_TopColor", settings.gradientTopColor);
		s_GradientShader.SetVec4("u_BottomColor", settings.gradientBottomColor);

		glBindVertexArray(s_EmptyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		// Re-enable depth testing for the rest of the frame.
		glEnable(GL_DEPTH_TEST);
	}
}