#include "EngineCore.h"
#include "Layers/EditorLayer.h"
#include "Layers/ImGuiLayer.h"
#include "Application.h"

#include "Renderer/Renderer.h"
#include "imgui.h"
#include "ECS/Scene.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"

#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>


namespace Orion {

	// Build orientation matrix matching RenderScene rotation order (X→Y→Z)
	static glm::mat4 BuildOrientationMatrix(const TransformComponent& tc)
	{
		glm::mat4 orient(1.0f);
		if (tc.rotation.x != 0.0f)
			orient = glm::rotate(orient, tc.rotation.x, glm::vec3(1, 0, 0));
		if (tc.rotation.y != 0.0f)
			orient = glm::rotate(orient, tc.rotation.y, glm::vec3(0, 1, 0));
		if (tc.rotation.z != 0.0f)
			orient = glm::rotate(orient, tc.rotation.z, glm::vec3(0, 0, 1));
		return orient;
	}

	// Initialize static members
	EntityID EditorLayer::s_SelectedEntity = INVALID_ENTITY;
	GizmoMode EditorLayer::s_GizmoMode = GizmoMode::Translate;
	PlayState EditorLayer::s_PlayState = PlayState::Stopped;
	EditorCamera EditorLayer::s_EditorCamera;

	EditorLayer::EditorLayer() : Layer("EditorLayer")
	{

	}

	EditorLayer::~EditorLayer()
	{

	}

	void EditorLayer::OnAttach()
	{
		s_EditorCamera.SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
		s_EditorCamera.SetYawPitch(-90.0f, -10.0f);
	}

	void EditorLayer::OnDetach()
	{
		Renderer::Shutdown();
	}

	void EditorLayer::OnUpdate()
	{
		static float lastFrameTime = 0.0f;
		float currentTime = (float)glfwGetTime();
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		// Update editor camera — input state was already fed via OnEvent()
		s_EditorCamera.Update(
			Renderer::GetActiveCamera(),
			deltaTime,
			ImGuiLayer::GetViewportHovered(),
			ImGuiLayer::GetViewportFocused()
		);

		Renderer::Render();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		// --- Forward input events to the editor camera ---
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) -> bool {
			s_EditorCamera.OnKeyPressed(e.GetKeyCode());
			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& e) -> bool {
			s_EditorCamera.OnKeyReleased(e.GetKeyCode());
			return false;
		});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) -> bool {
			s_EditorCamera.OnMouseMoved(e.GetX(), e.GetY());

			// Update gizmo drag if active
			if (m_DraggingGizmo)
				UpdateGizmoDrag(e.GetX(), e.GetY());

			return false;
		});

		dispatcher.Dispatch<MouseBtnPressedEvent>([this](MouseBtnPressedEvent& e) -> bool {
			s_EditorCamera.OnMouseButtonPressed(e.GetMouseBtn());

			// Left-click: try gizmo grab first, then entity selection (editor only)
			if (e.GetMouseBtn() == GLFW_MOUSE_BUTTON_LEFT && s_PlayState == PlayState::Stopped) {
				ImVec2 mousePos = ImGui::GetMousePos();
				if (!TryBeginGizmoDrag(mousePos.x, mousePos.y)) {
					// No gizmo hit — select entity
					s_SelectedEntity = ImGuiLayer::GetHoveredEntity();
				}
			}
			return false;
		});

		dispatcher.Dispatch<MouseBtnReleasedEvent>([this](MouseBtnReleasedEvent& e) -> bool {
			s_EditorCamera.OnMouseButtonReleased(e.GetMouseBtn());

			if (e.GetMouseBtn() == GLFW_MOUSE_BUTTON_LEFT)
				EndGizmoDrag();

			return false;
		});

		dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent& e) -> bool {
			s_EditorCamera.OnMouseScrolled(e.GetYOffset());
			return false;
		});

		// --- EditorLayer-specific key actions ---
		if (!event.Handled && event.GetEventType() == EventType::KeyPressed) {
			KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(event);
			int key = keyEvent.GetKeyCode();

			// P toggles play mode — always available
			if (key == GLFW_KEY_P) {
				if (s_PlayState == PlayState::Stopped)
					EnterPlayMode();
				else
					ExitPlayMode();
				event.Handled = true;
			}
			// Editor-only keybinds (disabled during play mode)
			else if (s_PlayState == PlayState::Stopped) {
				if (key == GLFW_KEY_I) {
					AddPrimitive("cube");
					event.Handled = true;
				}
				else if (key == GLFW_KEY_1) {
					s_GizmoMode = GizmoMode::Translate;
					event.Handled = true;
				}
				else if (key == GLFW_KEY_2) {
					s_GizmoMode = GizmoMode::Rotate;
					event.Handled = true;
				}
				else if (key == GLFW_KEY_3) {
					s_GizmoMode = GizmoMode::Scale;
					event.Handled = true;
				}
			}
		}
	}


	// --- Play mode ---

	void EditorLayer::EnterPlayMode()
	{
		if (s_PlayState == PlayState::Playing)
			return;

		auto editorScene = SceneManager::GetActiveScene();
		if (!editorScene) return;

		// Create and push the runtime layer
		m_RuntimeLayer = new RuntimeLayer();
		Application::Get().PushLayer(m_RuntimeLayer);
		m_RuntimeLayer->BeginPlay(editorScene);

		s_PlayState = PlayState::Playing;
		s_SelectedEntity = INVALID_ENTITY; // Deselect during play

		std::cout << "[EditorLayer] Entered play mode (F5 to stop).\n";
	}

	void EditorLayer::ExitPlayMode()
	{
		if (s_PlayState == PlayState::Stopped || !m_RuntimeLayer)
			return;

		// Stop play and restore the editor scene
		m_RuntimeLayer->EndPlay();

		// Remove runtime layer from the stack (calls OnDetach) and free it
		Application::Get().PopLayer(m_RuntimeLayer);
		delete m_RuntimeLayer;
		m_RuntimeLayer = nullptr;

		s_PlayState = PlayState::Stopped;

		std::cout << "[EditorLayer] Exited play mode. Scene restored.\n";
	}


	// --- Gizmo interaction ---

	bool EditorLayer::TryBeginGizmoDrag(float screenMouseX, float screenMouseY)
	{
		if (s_SelectedEntity == INVALID_ENTITY)
			return false;

		// Convert screen coords to viewport-local coords
		ImVec2 vpMin = ImGuiLayer::GetViewportMin();
		ImVec2 vpMax = ImGuiLayer::GetViewportMax();

		// Check if mouse is inside viewport
		if (screenMouseX < vpMin.x || screenMouseX >= vpMax.x ||
			screenMouseY < vpMin.y || screenMouseY >= vpMax.y)
			return false;

		float localX = screenMouseX - vpMin.x;
		float localY = screenMouseY - vpMin.y;
		float vpWidth = vpMax.x - vpMin.x;
		float vpHeight = vpMax.y - vpMin.y;

		// Build gizmo data matching what the Renderer uses
		auto* tc = SceneManager::GetActiveScene()->GetTransformComponent(s_SelectedEntity);
		if (!tc) return false;

		GizmoData gizmo;
		gizmo.position = tc->position;
		gizmo.orientation = BuildOrientationMatrix(*tc);
		gizmo.axisLength = 2.5f;
		gizmo.mode = s_GizmoMode;

		GizmoAxis hit = GizmoInteraction::HitTest(
			*Renderer::GetActiveCamera(),
			gizmo, localX, localY, vpWidth, vpHeight
		);

		if (hit == GizmoAxis::None)
			return false;

		m_DraggingGizmo = true;
		m_DragAxis = hit;
		m_LastDragMouseX = screenMouseX;
		m_LastDragMouseY = screenMouseY;
		return true;
	}

	void EditorLayer::UpdateGizmoDrag(float screenMouseX, float screenMouseY)
	{
		if (!m_DraggingGizmo || s_SelectedEntity == INVALID_ENTITY)
			return;

		auto scene = SceneManager::GetActiveScene();
		auto* tc = scene->GetTransformComponent(s_SelectedEntity);
		if (!tc) return;

		const Camera& camera = *Renderer::GetActiveCamera();

		// Get viewport dimensions
		ImVec2 vpMin = ImGuiLayer::GetViewportMin();
		ImVec2 vpMax = ImGuiLayer::GetViewportMax();
		float vpWidth = vpMax.x - vpMin.x;
		float vpHeight = vpMax.y - vpMin.y;

		if (vpWidth < 1.0f || vpHeight < 1.0f)
			return;

		float mouseDX = screenMouseX - m_LastDragMouseX;
		float mouseDY = screenMouseY - m_LastDragMouseY;
		m_LastDragMouseX = screenMouseX;
		m_LastDragMouseY = screenMouseY;

		// Build gizmo data with orientation for axis directions
		GizmoData gizmo;
		gizmo.position = tc->position;
		gizmo.orientation = BuildOrientationMatrix(*tc);

		if (s_GizmoMode == GizmoMode::Rotate)
		{
			// --- Rotation via ray-plane intersection ---
			// The rotation plane has its normal = the oriented axis being rotated around.
			glm::vec3 planeNormal = gizmo.GetAxisDir(m_DragAxis);

			// Convert screen coords to viewport-local
			float prevLocalX = (screenMouseX - mouseDX) - vpMin.x;
			float prevLocalY = (screenMouseY - mouseDY) - vpMin.y;
			float curLocalX = screenMouseX - vpMin.x;
			float curLocalY = screenMouseY - vpMin.y;

			// Cast rays for previous and current mouse positions
			glm::vec3 prevOrigin, prevDir, curOrigin, curDir;
			GizmoInteraction::ViewportToRay(prevLocalX, prevLocalY, vpWidth, vpHeight, camera, prevOrigin, prevDir);
			GizmoInteraction::ViewportToRay(curLocalX, curLocalY, vpWidth, vpHeight, camera, curOrigin, curDir);

			// Intersect both rays with the rotation plane
			float tPrev, tCur;
			bool hitPrev = GizmoInteraction::RayPlaneIntersect(prevOrigin, prevDir, tc->position, planeNormal, tPrev);
			bool hitCur = GizmoInteraction::RayPlaneIntersect(curOrigin, curDir, tc->position, planeNormal, tCur);

			if (!hitPrev || !hitCur)
				return;

			// Get intersection points in the plane, relative to gizmo center
			glm::vec3 prevHit = prevOrigin + prevDir * tPrev - tc->position;
			glm::vec3 curHit = curOrigin + curDir * tCur - tc->position;

			float prevLen = glm::length(prevHit);
			float curLen = glm::length(curHit);
			if (prevLen < 1e-6f || curLen < 1e-6f)
				return;

			prevHit /= prevLen;
			curHit /= curLen;

			// Signed angle between the two vectors around the plane normal
			float dotVal = glm::clamp(glm::dot(prevHit, curHit), -1.0f, 1.0f);
			float angleDelta = acosf(dotVal);
			float sign = glm::dot(planeNormal, glm::cross(prevHit, curHit));
			if (sign < 0.0f) angleDelta = -angleDelta;

			if (m_DragAxis == GizmoAxis::X)      tc->rotation.x += angleDelta;
			else if (m_DragAxis == GizmoAxis::Y)  tc->rotation.y += angleDelta;
			else if (m_DragAxis == GizmoAxis::Z)  tc->rotation.z += angleDelta;
		}
		else
		{
			// --- Translate / Scale: project mouse delta onto the screen-space oriented axis ---
			glm::vec3 axisDir = gizmo.GetAxisDir(m_DragAxis);

			glm::vec2 originScreen = GizmoInteraction::WorldToViewport(
				tc->position, camera, vpWidth, vpHeight);
			glm::vec2 axisTipScreen = GizmoInteraction::WorldToViewport(
				tc->position + axisDir, camera, vpWidth, vpHeight);

			glm::vec2 screenAxis = axisTipScreen - originScreen;
			float screenAxisLenSq = glm::dot(screenAxis, screenAxis);

			if (screenAxisLenSq < 1e-4f)
				return; // Axis is nearly perpendicular to the view

			glm::vec2 mouseDelta(mouseDX, mouseDY);
			float projectedPx = glm::dot(mouseDelta, screenAxis) / screenAxisLenSq;

			if (s_GizmoMode == GizmoMode::Translate)
			{
				tc->position += axisDir * projectedPx;
			}
			else // Scale
			{
				if (m_DragAxis == GizmoAxis::X)      tc->scale.x += projectedPx;
				else if (m_DragAxis == GizmoAxis::Y)  tc->scale.y += projectedPx;
				else if (m_DragAxis == GizmoAxis::Z)  tc->scale.z += projectedPx;

				tc->scale = glm::max(tc->scale, glm::vec3(0.01f));
			}
		}
	}

	void EditorLayer::EndGizmoDrag()
	{
		m_DraggingGizmo = false;
		m_DragAxis = GizmoAxis::None;
	}


	// --- Primitives ---

	void EditorLayer::AddPrimitive(std::string primitiveFileName)
	{
		std::shared_ptr<Scene> scene = SceneManager::GetActiveScene();
		EntityID entity = scene->CreateEntity();

		EntityDataComponent edc;
		scene->AddEntityDataComponent(entity, edc);

		TransformComponent tc;
		scene->AddTransformComponent(entity, tc);

		std::string meshPath = AssetManager::GetAssetsFolderPath() + "models\\" + primitiveFileName + ".obj";
		AssetID meshID = AssetManager::GetMeshID(meshPath);
		MeshComponent mc;
		mc.mesh = meshID;
		scene->AddMeshComponent(entity, mc);

		std::string matPath = AssetManager::GetAssetsFolderPath() + "materials\\default.mtrl";
		AssetID matID = AssetManager::GetMaterialID(matPath);
		MaterialComponent matc;
		matc.material = matID;
		scene->AddMaterialComponent(entity, matc);

		std::cout << "Primitive added with EntityID: " << entity << std::endl;
	}

}
