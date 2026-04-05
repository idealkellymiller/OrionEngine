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


namespace Orion {

	// Initialize static members
	EntityID EditorLayer::s_SelectedEntity = INVALID_ENTITY;
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
			// Don't consume — fall through to the key handler below
			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& e) -> bool {
			s_EditorCamera.OnKeyReleased(e.GetKeyCode());
			return false;
		});

		dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent& e) -> bool {
			s_EditorCamera.OnMouseMoved(e.GetX(), e.GetY());
			return false;
		});

		dispatcher.Dispatch<MouseBtnPressedEvent>([this](MouseBtnPressedEvent& e) -> bool {
			s_EditorCamera.OnMouseButtonPressed(e.GetMouseBtn());
			// Also handle entity selection
			s_SelectedEntity = ImGuiLayer::GetHoveredEntity();
			return false;
		});

		dispatcher.Dispatch<MouseBtnReleasedEvent>([](MouseBtnReleasedEvent& e) -> bool {
			s_EditorCamera.OnMouseButtonReleased(e.GetMouseBtn());
			return false;
		});

		dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent& e) -> bool {
			s_EditorCamera.OnMouseScrolled(e.GetYOffset());
			return false;
		});

		// --- EditorLayer-specific key actions ---
		// Note: KeyPressedEvent was already dispatched to the camera above,
		// but since we returned false (not handled), the event is still valid.
		// However, EventDispatcher only dispatches once per type, so we check manually.
		if (!event.Handled && event.GetEventType() == EventType::KeyPressed) {
			KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(event);
			if (keyEvent.GetKeyCode() == GLFW_KEY_I) {
				AddPrimitive("cube");
				event.Handled = true;
			}
		}
	}



	// Add a primitive at world origin with
	void EditorLayer::AddPrimitive(std::string primitiveFileName)	// i.e. "cube"
	{
		// Get scene and the ID of our new entity
		std::shared_ptr<Scene> scene = SceneManager::GetActiveScene();
		EntityID entity = scene->CreateEntity();

		// Add metadata component
		EntityDataComponent edc;
		scene->AddEntityDataComponent(entity, edc);

		std::cout << "Added data C" << std::endl;

		// Add transform
		TransformComponent tc;
		scene->AddTransformComponent(entity, tc);

		std::cout << "Added Transform C" << std::endl;

		// Add mesh from the primitive meshes in assets
		// Path must match the key used during LoadAssetsFolder() (full path with backslashes)
		std::string meshPath = AssetManager::GetAssetsFolderPath() + "models\\" + primitiveFileName + ".obj";
		AssetID meshID = AssetManager::GetMeshID(meshPath);
		MeshComponent mc;
		mc.mesh = meshID;
		scene->AddMeshComponent(entity, mc);

		std::cout << "Added Mesh C" << std::endl;

		// Add default material
		std::string matPath = AssetManager::GetAssetsFolderPath() + "materials\\default.mtrl";
		AssetID matID = AssetManager::GetMaterialID(matPath);
		MaterialComponent matc;
		matc.material = matID;
		scene->AddMaterialComponent(entity, matc);

		std::cout << "Added default Mat C" << std::endl;
		std::cout << "Primitive added with EntityID: " << entity << std::endl;
	}

}
