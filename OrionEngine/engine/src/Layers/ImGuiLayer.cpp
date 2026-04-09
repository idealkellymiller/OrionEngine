#include "EngineCore.h"
#include "Layers/ImGuiLayer.h"
#include "Layers/EditorLayer.h"
#include "Application.h"
#include "Renderer/Renderer.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"
#include "Core/ProjectSettings.h"

#include "imgui.h"
#include "libintl.h"

// #include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include "../external/ImGUI/backends/imgui_impl_glfw.h"
#include "../external/ImGUI/backends/imgui_impl_opengl3.h"

// TODO: remove glfw include here
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <format>
#include <iostream>
#include <algorithm>

#define _(x) gettext(x)

namespace Orion {

	// Initialize static members
	EntityID ImGuiLayer::s_HoveredEntity = INVALID_ENTITY;

	bool ImGuiLayer::s_ViewportHovered = false;
	bool ImGuiLayer::s_ViewportFocused = false;
	bool ImGuiLayer::s_ViewportDragging = false;
	ImVec2 ImGuiLayer::s_ViewportImageMin = { 0, 0 };
	ImVec2 ImGuiLayer::s_ViewportImageMax = { 0, 0 };


	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;


		Application& app = Application::Get();
		ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow()), true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnUpdate()
	{
		
		// -------------------------------- ImGui ------------------------------------
		
		
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		float time = (float)glfwGetTime();
		io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport();

		static bool show = true;
		 //ImGui::ShowDemoWindow(&show);
		ShowMainMenuBar();
		ShowInspectorModule();
		ShowViewportModule();
		ShowHierarchyModule();
		ShowConsoleModule();
		ShowControlsModule();
		ShowProjectSettingsWindow();

		// Tell ImGui to finalize all UI draw data
		ImGui::Render();




		// Object Picking:
		// TODO: mousePos is being overwritten so this bool will always be false. Get mouse input through event system
		ImVec2 mousePos = ImGui::GetMousePos();
		bool mouseInsideViewportImage =
			mousePos.x >= s_ViewportImageMin.x &&
			mousePos.x < s_ViewportImageMax.x &&
			mousePos.y >= s_ViewportImageMin.y &&
			mousePos.y < s_ViewportImageMax.y;
		if (mouseInsideViewportImage) {
			int localMouseX = static_cast<int>(mousePos.x - s_ViewportImageMin.x);
			int localMouseY = static_cast<int>(mousePos.y - s_ViewportImageMin.y);
			// std::cout << localMouseX << "," << localMouseY << std::endl;
			s_HoveredEntity = Renderer::PickEntity(localMouseX, localMouseY);
			// std::cout << "Hovered entity: " << hovered << " at (" << localMouseX << "," << localMouseY << ")\n";

		}
		else {
			// std::cout << "Hovered entity: 000\n";
			// std::cout << "Outside viewport" << std::endl;
		}




		// Draw all ImGui windows, including the Viewport image
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and render additional platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			// for OpenGL: restore current GL context.
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		ImGuiIO& io = ImGui::GetIO();

		switch (event.GetEventType())
		{
			case EventType::KeyPressed:
			case EventType::KeyReleased:
			{
				// Block keyboard events only when ImGui truly needs them
				// (e.g. typing in an InputText) AND the user isn't actively
				// interacting with the viewport (camera fly uses WASD).
				if (io.WantCaptureKeyboard && !s_ViewportFocused && !s_ViewportDragging)
				{
					event.Handled = true;
				}
				break;
			}
			case EventType::MouseButtonPressed:
			{
				// If a mouse press starts in the viewport, begin a viewport drag.
				// This keeps events flowing to EditorLayer even if the cursor
				// leaves the viewport mid-drag (e.g. camera fly with RMB).
				if (s_ViewportHovered)
					s_ViewportDragging = true;

				if (io.WantCaptureMouse && !s_ViewportHovered)
					event.Handled = true;
				break;
			}
			case EventType::MouseButtonReleased:
			{
				// End viewport drag on any release.
				s_ViewportDragging = false;

				if (io.WantCaptureMouse && !s_ViewportHovered)
					event.Handled = true;
				break;
			}
			case EventType::MouseMoved:
			case EventType::MouseScrolled:
			{
				// Allow mouse events through if the cursor is over the viewport
				// OR if a drag started in the viewport (camera fly, gizmo drag, etc.).
				if (io.WantCaptureMouse && !s_ViewportHovered && !s_ViewportDragging)
				{
					event.Handled = true;
				}
				break;
			}
			case EventType::WindowResize:
			{
				WindowResizeEvent& e = (WindowResizeEvent&)event;

				// tell imgui the new app window size
				io.DisplaySize = ImVec2((float)e.GetWidth(), e.GetHeight());
				break;
			}
			default:
				break;
		}
	}

	// -------------------------IMGUI MODULE DEFINITIONS-------------------------

	// MODULE STATE BOOLS to keep track of shown/unshown modules
	static bool showInspectorModule = true;
	static bool showViewportModule = true;
	static bool showHierarchyModule = true;
	static bool showFileDirectoryModule = true;
	static bool showConsoleModule = true;
	static bool showControlsModule = true;
	static bool showProjectSettings = false;

#define CHECKED_MENU_ITEM(menuItemName, checkedState) if (ImGui::MenuItem(menuItemName, NULL, checkedState)) { checkedState = !checkedState; }

	void ImGuiLayer::ShowMainMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			// file menu bar option
			if (ImGui::BeginMenu(_("File")))
			{
				if (ImGui::MenuItem("Save", "CTRL + S")) {}
				if (ImGui::MenuItem("Save as")) {}
				if (ImGui::MenuItem("Import")) {}
				ImGui::EndMenu();
			}
			// settings menu bar option
			if (ImGui::BeginMenu("Settings"))
			{
				if (ImGui::MenuItem("Project Settings"))
					showProjectSettings = true;
				ImGui::EndMenu();
			}
			// view module menu bar option
			if (ImGui::BeginMenu("View Module"))
			{
				// show all module options to close/open specific modules
				CHECKED_MENU_ITEM("Inspector", showInspectorModule);
				CHECKED_MENU_ITEM("Viewport", showViewportModule);
				CHECKED_MENU_ITEM("Hierarchy", showHierarchyModule);
				CHECKED_MENU_ITEM("File Directory", showFileDirectoryModule);
				CHECKED_MENU_ITEM("Console", showConsoleModule);
				CHECKED_MENU_ITEM("Controls", showControlsModule);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	// ========================== INSPECTOR ==========================

	// Helper: human-readable name for each component type
	static const char* ComponentTypeName(ComponentType type)
	{
		switch (type) {
			case ComponentType::EntityData: return "Entity Data";
			case ComponentType::Transform:  return "Transform";
			case ComponentType::Mesh:       return "Mesh";
			case ComponentType::Material:   return "Material";
			case ComponentType::Camera:     return "Camera";
			case ComponentType::Script:     return "Script";
			case ComponentType::Rigidbody:  return "Rigidbody";
			case ComponentType::Collider:   return "Collider";
			default:                        return "Unknown";
		}
	}

	bool ImGuiLayer::IsRequiredComponent(ComponentType type)
	{
		// EntityData and Transform are mandatory — every entity must have them.
		return type == ComponentType::EntityData || type == ComponentType::Transform;
	}

	void ImGuiLayer::RebuildComponentOrder(EntityID entity, Scene& scene)
	{
		// List every component the entity actually owns, in default order.
		std::vector<ComponentType> order;

		if (scene.HasEntityDataComponent(entity))
			order.push_back(ComponentType::EntityData);
		if (scene.HasTransformComponent(entity))
			order.push_back(ComponentType::Transform);
		if (scene.HasMeshComponent(entity))
			order.push_back(ComponentType::Mesh);
		if (scene.HasMaterialComponent(entity))
			order.push_back(ComponentType::Material);
		if (scene.HasCameraComponent(entity))
			order.push_back(ComponentType::Camera);
		if (scene.HasScriptComponent(entity))
			order.push_back(ComponentType::Script);
		if (scene.HasRigidbodyComponent(entity))
			order.push_back(ComponentType::Rigidbody);
		if (scene.HasColliderComponent(entity))
			order.push_back(ComponentType::Collider);

		m_ComponentOrder[entity] = order;
	}

	// ---------- Individual component field drawers ----------

	void ImGuiLayer::DrawEntityDataFields(EntityID entity, Scene& scene)
	{
		EntityDataComponent* data = scene.GetEntityDataComponent(entity);
		if (!data) return;

		// Editable entity name
		char nameBuf[256];
		strncpy_s(nameBuf, data->name.c_str(), sizeof(nameBuf) - 1);
		nameBuf[sizeof(nameBuf) - 1] = '\0';
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
			data->name = nameBuf;
		}

		ImGui::Checkbox("Enabled", &data->enabled);
	}

	void ImGuiLayer::DrawTransformFields(EntityID entity, Scene& scene)
	{
		TransformComponent* tc = scene.GetTransformComponent(entity);
		if (!tc) return;

		ImGui::DragFloat3("Position", &tc->position.x, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
		ImGui::DragFloat3("Rotation", &tc->rotation.x, 0.1f,   -FLT_MAX, +FLT_MAX, "%.3f");

		// Uniform-scale toggle
		static bool scaleUniform = false;
		glm::vec3 oldScale = tc->scale;

		if (ImGui::DragFloat3("Scale", &tc->scale.x, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f"))
		{
			if (scaleUniform) {
				glm::vec3 delta = tc->scale - oldScale;
				if (delta.x != 0.0f) {
					tc->scale.y = oldScale.y + delta.x;
					tc->scale.z = oldScale.z + delta.x;
				}
				else if (delta.y != 0.0f) {
					tc->scale.x = oldScale.x + delta.y;
					tc->scale.z = oldScale.z + delta.y;
				}
				else if (delta.z != 0.0f) {
					tc->scale.x = oldScale.x + delta.z;
					tc->scale.y = oldScale.y + delta.z;
				}
			}
		}
		ImGui::Checkbox("Scale Uniform", &scaleUniform);
	}

	void ImGuiLayer::DrawMeshFields(EntityID entity, Scene& scene)
	{
		MeshComponent* mc = scene.GetMeshComponent(entity);
		if (!mc) return;

		// Build the preview label for the dropdown (current selection)
		std::string previewLabel = "(none)";
		if (mc->mesh != INVALID_ASSET_ID) {
			MeshAsset* current = AssetManager::GetMeshAsset(mc->mesh);
			if (current && !current->name.empty())
				previewLabel = current->name;
			else
				previewLabel = "ID: " + std::to_string(mc->mesh);
		}

		// Dropdown to pick a mesh asset
		if (ImGui::BeginCombo("Mesh", previewLabel.c_str()))
		{
			// Option to clear the mesh
			if (ImGui::Selectable("(none)", mc->mesh == INVALID_ASSET_ID))
				mc->mesh = INVALID_ASSET_ID;

			// List every loaded mesh asset
			for (auto& [id, asset] : AssetManager::GetAllMeshAssets())
			{
				bool isSelected = (mc->mesh == id);
				std::string label = asset.name.empty()
					? ("ID: " + std::to_string(id))
					: asset.name;

				if (ImGui::Selectable(label.c_str(), isSelected))
					mc->mesh = id;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Show path as read-only info below the dropdown
		if (mc->mesh != INVALID_ASSET_ID) {
			MeshAsset* asset = AssetManager::GetMeshAsset(mc->mesh);
			if (asset)
				ImGui::TextDisabled("Path: %s", asset->filePath.c_str());
		}
	}

	void ImGuiLayer::DrawMaterialFields(EntityID entity, Scene& scene)
	{
		MaterialComponent* matComp = scene.GetMaterialComponent(entity);
		if (!matComp) return;

		// Build the preview label for the dropdown (current selection)
		std::string previewLabel = "(none)";
		if (matComp->material != INVALID_ASSET_ID) {
			MaterialAsset* current = AssetManager::GetMaterialAsset(matComp->material);
			if (current && !current->name.empty())
				previewLabel = current->name;
			else
				previewLabel = "ID: " + std::to_string(matComp->material);
		}

		// Dropdown to pick a material asset
		if (ImGui::BeginCombo("Material", previewLabel.c_str()))
		{
			// Option to clear the material
			if (ImGui::Selectable("(none)", matComp->material == INVALID_ASSET_ID))
				matComp->material = INVALID_ASSET_ID;

			// List every loaded material asset
			for (auto& [id, asset] : AssetManager::GetAllMaterialAssets())
			{
				bool isSelected = (matComp->material == id);
				std::string label = asset.name.empty()
					? ("ID: " + std::to_string(id))
					: asset.name;

				if (ImGui::Selectable(label.c_str(), isSelected))
					matComp->material = id;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Show editable material properties below the dropdown
		if (matComp->material != INVALID_ASSET_ID) {
			MaterialAsset* asset = AssetManager::GetMaterialAsset(matComp->material);
			if (asset) {
				ImGui::TextDisabled("Path: %s", asset->filePath.c_str());
				ImGui::Separator();
				ImGui::ColorEdit4("Color Tint", &asset->colorTint.x);
				ImGui::ColorEdit3("Specular Color", &asset->specularColor.x);
				ImGui::DragFloat("Shininess", &asset->specularShininess, 0.5f, 0.0f, 256.0f);
				ImGui::Checkbox("Transparent", &asset->isTransparent);
			}
		}
	}

	void ImGuiLayer::DrawCameraFields(EntityID entity, Scene& scene)
	{
		CameraComponent* cam = scene.GetCameraComponent(entity);
		if (!cam) return;

		ImGui::DragFloat("FOV (degrees)", &cam->fovDegrees, 0.5f, 1.0f, 179.0f);
		ImGui::DragFloat("Near Plane", &cam->nearPlane, 0.01f, 0.001f, 100.0f, "%.3f");
		ImGui::DragFloat("Far Plane", &cam->farPlane, 1.0f, 1.0f, 10000.0f);
		ImGui::Checkbox("Active", &cam->isActive);
	}

	void ImGuiLayer::DrawScriptFields(EntityID entity, Scene& scene)
	{
		ScriptComponent* sc = scene.GetScriptComponent(entity);
		if (!sc) return;

		// Editable script path
		char pathBuf[512];
		strncpy_s(pathBuf, sc->scriptPath.c_str(), sizeof(pathBuf) - 1);
		pathBuf[sizeof(pathBuf) - 1] = '\0';
		if (ImGui::InputText("Script Path", pathBuf, sizeof(pathBuf))) {
			sc->scriptPath = pathBuf;
		}

		if (sc->scriptPath.empty()) {
			ImGui::TextDisabled("(no script assigned)");
		}
	}

	void ImGuiLayer::DrawRigidbodyFields(EntityID entity, Scene& scene)
	{
		RigidbodyComponent* rb = scene.GetRigidbodyComponent(entity);
		if (!rb) return;

		// Body type dropdown
		const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
		int currentType = static_cast<int>(rb->bodyType);
		if (ImGui::Combo("Body Type", &currentType, bodyTypes, IM_ARRAYSIZE(bodyTypes))) {
			rb->bodyType = static_cast<BodyType>(currentType);
		}

		// Only show mass for dynamic bodies
		if (rb->bodyType == BodyType::Dynamic) {
			ImGui::DragFloat("Mass", &rb->mass, 0.1f, 0.01f, 10000.0f, "%.2f");
		}

		ImGui::DragFloat("Linear Damping", &rb->linearDamping, 0.01f, 0.0f, 100.0f, "%.3f");
		ImGui::DragFloat("Angular Damping", &rb->angularDamping, 0.01f, 0.0f, 100.0f, "%.3f");
		ImGui::DragFloat("Gravity Scale", &rb->gravityScale, 0.05f, 0.0f, 10.0f, "%.2f");

		ImGui::Separator();
		ImGui::Text("Freeze Rotation");
		ImGui::Checkbox("X##FreezeRotX", &rb->freezeRotationX);
		ImGui::SameLine();
		ImGui::Checkbox("Y##FreezeRotY", &rb->freezeRotationY);
		ImGui::SameLine();
		ImGui::Checkbox("Z##FreezeRotZ", &rb->freezeRotationZ);
	}

	void ImGuiLayer::DrawColliderFields(EntityID entity, Scene& scene)
	{
		ColliderComponent* col = scene.GetColliderComponent(entity);
		if (!col) return;

		// Shape dropdown
		const char* shapes[] = { "Box", "Sphere" };
		int currentShape = static_cast<int>(col->shape);
		if (ImGui::Combo("Shape", &currentShape, shapes, IM_ARRAYSIZE(shapes))) {
			col->shape = static_cast<ColliderShape>(currentShape);
		}

		// Shape-specific fields
		if (col->shape == ColliderShape::Box) {
			ImGui::DragFloat3("Half Extents", &col->boxHalfExtents.x, 0.05f, 0.01f, 1000.0f, "%.3f");
		}
		else if (col->shape == ColliderShape::Sphere) {
			ImGui::DragFloat("Radius", &col->sphereRadius, 0.05f, 0.01f, 1000.0f, "%.3f");
		}

		ImGui::Checkbox("Is Trigger", &col->isTrigger);
		ImGui::DragFloat3("Offset", &col->offset.x, 0.05f, -1000.0f, 1000.0f, "%.3f");
	}

	// ---------- Draw one component entry ----------

	// Helper: draw a button that appears grayed-out and does nothing when disabled.
	// Works with any ImGui version (no BeginDisabled needed).
	static bool DisableableButton(const char* label, ImVec2 size, bool enabled)
	{
		if (!enabled) {
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
			ImGui::Button(label, size);
			ImGui::PopStyleVar();
			return false;
		}
		return ImGui::Button(label, size);
	}

	bool ImGuiLayer::DrawComponent(ComponentType type, int index, EntityID entity, Scene& scene)
	{
		auto& order = m_ComponentOrder[entity];
		bool removed = false;

		// Unique ImGui ID per component slot so headers don't collide
		ImGui::PushID(index);

		// --- Header row: collapsing header + action buttons on the right ---
		float contentWidth = ImGui::GetContentRegionAvail().x;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen
			| ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_AllowOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		bool open = ImGui::TreeNodeEx(ComponentTypeName(type), flags);
		ImGui::PopStyleVar();

		// Right-aligned buttons: [^] [v] [X]
		bool isRequired = IsRequiredComponent(type);
		int buttonCount = isRequired ? 2 : 3;
		float buttonWidth = 25.0f;
		float buttonsWidth = buttonCount * (buttonWidth + ImGui::GetStyle().ItemSpacing.x);

		ImGui::SameLine(contentWidth - buttonsWidth);

		// Move Up (grayed out if already first)
		if (DisableableButton("^", ImVec2(buttonWidth, lineHeight), index > 0)) {
			std::swap(order[index], order[index - 1]);
		}

		ImGui::SameLine();

		// Move Down (grayed out if already last)
		if (DisableableButton("v", ImVec2(buttonWidth, lineHeight), index < (int)order.size() - 1)) {
			std::swap(order[index], order[index + 1]);
		}

		// Remove button — only for non-required components
		if (!isRequired) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button("X", ImVec2(buttonWidth, lineHeight))) {
				removed = true;
			}

			ImGui::PopStyleColor(3);
		}

		// Draw component fields if the header is expanded
		if (open) {
			switch (type) {
				case ComponentType::EntityData: DrawEntityDataFields(entity, scene);  break;
				case ComponentType::Transform:  DrawTransformFields(entity, scene);   break;
				case ComponentType::Mesh:       DrawMeshFields(entity, scene);        break;
				case ComponentType::Material:   DrawMaterialFields(entity, scene);    break;
				case ComponentType::Camera:     DrawCameraFields(entity, scene);      break;
				case ComponentType::Script:     DrawScriptFields(entity, scene);      break;
				case ComponentType::Rigidbody:  DrawRigidbodyFields(entity, scene);   break;
				case ComponentType::Collider:   DrawColliderFields(entity, scene);    break;
			}
			ImGui::TreePop();
		}

		ImGui::PopID();

		// Handle removal after drawing (so ImGui IDs stay stable this frame)
		if (removed) {
			switch (type) {
				case ComponentType::Mesh:     scene.RemoveMeshComponent(entity);     break;
				case ComponentType::Material: scene.RemoveMaterialComponent(entity);  break;
				case ComponentType::Camera:    scene.RemoveCameraComponent(entity);    break;
				case ComponentType::Script:    scene.RemoveScriptComponent(entity);    break;
				case ComponentType::Rigidbody: scene.RemoveRigidbodyComponent(entity); break;
				case ComponentType::Collider:  scene.RemoveColliderComponent(entity);  break;
				default: break;
			}
			order.erase(order.begin() + index);
		}

		return removed;
	}

	// ---------- Add Component popup ----------

	void ImGuiLayer::DrawAddComponentPopup(EntityID entity, Scene& scene)
	{
		if (ImGui::BeginPopup("AddComponentPopup")) {
			ImGui::Text("Add Component");
			ImGui::Separator();

			auto& order = m_ComponentOrder[entity];

			// Helper: checks if a type is already in the display order
			auto hasType = [&](ComponentType t) {
				return std::find(order.begin(), order.end(), t) != order.end();
			};

			int addableCount = 0;

			if (!hasType(ComponentType::Mesh)) {
				addableCount++;
				if (ImGui::Selectable("Mesh")) {
					scene.AddMeshComponent(entity, MeshComponent{});
					order.push_back(ComponentType::Mesh);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Material)) {
				addableCount++;
				if (ImGui::Selectable("Material")) {
					scene.AddMaterialComponent(entity, MaterialComponent{});
					order.push_back(ComponentType::Material);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Camera)) {
				addableCount++;
				if (ImGui::Selectable("Camera")) {
					scene.AddCameraComponent(entity, CameraComponent{});
					order.push_back(ComponentType::Camera);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Script)) {
				addableCount++;
				if (ImGui::Selectable("Script")) {
					scene.AddScriptComponent(entity, ScriptComponent{});
					order.push_back(ComponentType::Script);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Rigidbody)) {
				addableCount++;
				if (ImGui::Selectable("Rigidbody")) {
					scene.AddRigidbodyComponent(entity, RigidbodyComponent{});
					order.push_back(ComponentType::Rigidbody);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Collider)) {
				addableCount++;
				if (ImGui::Selectable("Collider")) {
					scene.AddColliderComponent(entity, ColliderComponent{});
					order.push_back(ComponentType::Collider);
					ImGui::CloseCurrentPopup();
				}
			}

			if (addableCount == 0) {
				ImGui::TextDisabled("All components added");
			}

			ImGui::EndPopup();
		}
	}

	// ---------- Main Inspector module ----------

	void ImGuiLayer::ShowInspectorModule()
	{
		if (showInspectorModule)
		{
			if (ImGui::Begin("Inspector", &showInspectorModule))
			{
				EntityID selected = EditorLayer::GetSelectedEntity();
				auto scene = SceneManager::GetActiveScene();

				// Only show content if a valid entity is selected and the scene exists
				if (selected != INVALID_ENTITY && scene && scene->IsValidEntity(selected))
				{
					// Rebuild the component display order when selection changes
					if (m_InspectorEntity != selected) {
						m_InspectorEntity = selected;
						// Only rebuild if we don't already have a cached order for this entity
						if (m_ComponentOrder.find(selected) == m_ComponentOrder.end()) {
							RebuildComponentOrder(selected, *scene);
						}
					}

					ImGui::Text("Entity ID: %u", selected);
					ImGui::Separator();

					// Draw each component in the user-defined display order
					auto& order = m_ComponentOrder[selected];
					for (int i = 0; i < (int)order.size(); i++) {
						// DrawComponent returns true if the component was removed,
						// which shifts indices — decrement i to recheck this slot.
						if (DrawComponent(order[i], i, selected, *scene)) {
							i--;
						}
					}

					ImGui::Separator();

					// Centered "Add Component" button
					float buttonWidth = 200.0f;
					float windowWidth = ImGui::GetContentRegionAvail().x;
					float cursorX = (windowWidth - buttonWidth) * 0.5f;
					if (cursorX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursorX);

					if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0))) {
						ImGui::OpenPopup("AddComponentPopup");
					}

					DrawAddComponentPopup(selected, *scene);
				}
				else
				{
					ImGui::TextDisabled("No entity selected");
				}
			}
			ImGui::End();
		}
	}

	GLuint viewportTexture;
	int viewportWidth = 800;
	int viewportHeight = 600;

	void ImGuiLayer::ShowViewportModule()
	{


		if (showViewportModule)
		{

			static ImVec2 viewportSize = ImVec2(0.0f, 0.0f);
			//bool viewportHovered = false;
			//bool viewportFocused = false;

			if (ImGui::Begin("Viewport", &showViewportModule))
			{
				s_ViewportHovered = ImGui::IsWindowHovered();
				s_ViewportFocused = ImGui::IsWindowFocused();

				// Viewport window
				viewportSize = ImGui::GetContentRegionAvail();

				// Prevent weird zero-cases when minimized/collapsed
				if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
				if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;

				// Resize the framebuffer if panel size changed
				// TODO: make viewportframebuffer static variable that is accessed through renderer facade
				Renderer::GetViewportFramebuffer()->Resize(
					static_cast<unsigned int>(viewportSize.x),
					static_cast<unsigned int>(viewportSize.y)
				);


				// Get the size available inside this window for content
				// Save image rect before drawing.
				s_ViewportImageMin = ImGui::GetCursorScreenPos();
				// Show the framebuffer's color texture inside ImGui
				// ImGui uses ImTextureID, and for OpenGL that is just the texture handle cast.
				// UVs are flipped vertically because OpenGL texture origin is bottom-left,
				// while ImGui expects top-left style display.
				ImGui::Image(
					(ImTextureID)(intptr_t)Renderer::GetViewportFramebuffer()->GetColorAttachment(),
					ImVec2(viewportSize.x, viewportSize.y),
					ImVec2(0, 1),   // UV top-left
					ImVec2(1, 0)    // UV bottom-right
				);

				// IMPORTANT for picking
				// Top-left of where the image will be drawn in screen coordinates
				s_ViewportImageMax = ImVec2(
					s_ViewportImageMin.x + viewportSize.x,
					s_ViewportImageMin.y + viewportSize.y
				);
			}


			ImGui::End();
		}
	}

	// ================================================================
	// Hierarchy Panel — shows all entities as a tree using ECS data
	// ================================================================

	void ImGuiLayer::ShowHierarchyModule()
	{
		if (!showHierarchyModule)
			return;

		if (ImGui::Begin("Hierarchy", &showHierarchyModule))
		{
			auto scene = SceneManager::GetActiveScene();
			if (scene)
			{
				// Only draw root entities (those with no parent).
				// Children are drawn recursively inside DrawEntityNode.
				std::vector<EntityID> roots = scene->GetRootEntities();
				for (EntityID root : roots)
				{
					DrawEntityNode(root, *scene);
				}

				// Right-click on empty space = context menu for the whole panel
				if (ImGui::BeginPopupContextWindow("HierarchyContextMenu",
					ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
					if (ImGui::MenuItem("Create Empty Entity")) {
						EntityID newEntity = scene->CreateEntity();
						scene->AddEntityDataComponent(newEntity, EntityDataComponent{});
						scene->AddTransformComponent(newEntity, TransformComponent{});
						EditorLayer::SetSelectedEntity(newEntity);
					}
					ImGui::EndPopup();
				}

				// Drop on empty space = unparent (make root)
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
					{
						EntityID droppedEntity = *(const EntityID*)payload->Data;
						scene->RemoveParent(droppedEntity);
					}
					ImGui::EndDragDropTarget();
				}
			}
		}
		ImGui::End();
	}

	void ImGuiLayer::DrawEntityNode(EntityID entity, Scene& scene)
	{
		// Get the entity's display name.
		EntityDataComponent* edc = scene.GetEntityDataComponent(entity);
		std::string name = edc ? edc->name : ("Entity " + std::to_string(entity));

		// Is this entity currently selected?
		bool isSelected = (entity == EditorLayer::GetSelectedEntity());

		// Does this entity have children?
		bool hasChildren = scene.HasChildren(entity);

		// Build tree node flags:
		//   - OpenOnArrow: only expand when clicking the arrow, not the label
		//   - SpanAvailWidth: the node spans the full width (easier to click)
		//   - Selected: highlight if this is the selected entity
		//   - Leaf: no arrow if no children
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
		                         | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		// Use the entity ID as the ImGui ID to keep nodes unique.
		bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", name.c_str());

		// --- Click to select ---
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			EditorLayer::SetSelectedEntity(entity);
		}

		// --- Drag source: start dragging this entity ---
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(EntityID));
			ImGui::Text("%s", name.c_str());
			ImGui::EndDragDropSource();
		}

		// --- Drop target: reparent the dragged entity under this one ---
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
			{
				EntityID droppedEntity = *(const EntityID*)payload->Data;
				// Don't parent an entity to itself.
				if (droppedEntity != entity)
				{
					scene.SetParent(droppedEntity, entity);
				}
			}
			ImGui::EndDragDropTarget();
		}

		// --- Right-click context menu on this entity ---
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Child Entity"))
			{
				EntityID child = scene.CreateEntity();
				scene.AddEntityDataComponent(child, EntityDataComponent{});
				scene.AddTransformComponent(child, TransformComponent{});
				scene.SetParent(child, entity);
				EditorLayer::SetSelectedEntity(child);
			}

			if (ImGui::MenuItem("Delete Entity"))
			{
				// If this was selected, clear selection.
				if (EditorLayer::GetSelectedEntity() == entity)
					EditorLayer::SetSelectedEntity(INVALID_ENTITY);

				scene.DestroyEntity(entity);

				ImGui::EndPopup();
				// Node was destroyed — don't draw children.
				if (nodeOpen && hasChildren)
					ImGui::TreePop();
				return;
			}

			if (scene.HasParent(entity))
			{
				if (ImGui::MenuItem("Unparent"))
				{
					scene.RemoveParent(entity);
				}
			}

			ImGui::EndPopup();
		}

		// --- Recursively draw children if the node is open ---
		if (nodeOpen && hasChildren)
		{
			const auto& children = scene.GetChildren(entity);
			for (EntityID child : children)
			{
				DrawEntityNode(child, scene);
			}
			ImGui::TreePop();
		}
	}

	////holds the files
	//namespace fs = std::filesystem;
	//void ImGuiLayer::DrawDirectoryTree(const fs::path& path)
	//{
	//	for (const auto& entry : fs::directory_iterator(path))
	//	{
	//		const auto& p = entry.path();
	//		std::string name = p.filename().string();
	//		if (entry.is_directory())
	//		{
	//			if (ImGui::TreeNode(name.c_str()))
	//			{
	//				DrawDirectoryTree(p);
	//				ImGui::TreePop();
	//			}
	//		}
	//		else
	//		{
	//			ImGui::BulletText("%s", name.c_str());
	//		}
	//	}
	//}

	////search bar for files
	//void ImGuiLayer::DrawDirectorySearch(const fs::path& path, ImGuiTextFilter& filter)
	//{
	//	for (const auto& entry : fs::recursive_directory_iterator(path))
	//	{
	//		std::string name = entry.path().filename().string();

	//		if (filter.PassFilter(name.c_str()))
	//		{
	//			if (ImGui::Selectable(name.c_str()))
	//			{

	//			}
	//		}
	//	}
	//}

	////puts them together
	//void ImGuiLayer::DrawDirectory(const fs::path& path)
	//{
	//	static ImGuiTextFilter filter;
	//	filter.Draw("Search");
	//	bool searching = strlen(filter.InputBuf) > 0;
	//	if (searching)
	//		DrawDirectorySearch(path, filter);
	//	else
	//		DrawDirectoryTree(path);
	//}

	//void ImGuiLayer::ShowFileDirectoryModule()
	//{
	//	if (showFileDirectoryModule)
	//	{
	//		if (ImGui::Begin("File Directory", &showFileDirectoryModule))
	//		{
	//			//used the helper cause i thought it would be helpful lol
	//			// HelpMarker("This showes the files in the project, you can use the search bar to find a specific files");

	//			DrawDirectory(".");

	//		}
	//		ImGui::End();
	//	}
	//}

	void ImGuiLayer::ShowConsoleTraceOutput(const char* source, const char* message)
	{
		ImU32 consoleTraceGray = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleTraceGray);
	}

	void ImGuiLayer::ShowConsoleWarningOutput(const char* source, const char* message)
	{
		ImU32 consoleWarningYellow = ImGui::GetColorU32(ImVec4(0.6f, 0.4f, 0.04f, 0.85f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleWarningYellow);
	}

	void ImGuiLayer::ShowConsoleErrorOutput(const char* source, const char* message)
	{
		ImU32 consoleErrorRed = ImGui::GetColorU32(ImVec4(0.8f, 0.1f, 0.04f, 0.65f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleErrorRed);
	}

	void ImGuiLayer::ShowConsoleModule()
	{
		if (showConsoleModule)
		{
			if (ImGui::Begin("Console", &showConsoleModule))
			{
				static ImGuiTextFilter filter;
				filter.Draw("Search");
				//const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
				//for (int i = 0; i < IM_COUNTOF(lines); i++)
				//	if (filter.PassFilter(lines[i]))
				//		ImGui::BulletText("%s", lines[i]);
				//ImGui::TreePop();

				if (ImGui::BeginTable("ConsoleOutputTable", 1))
				{
					ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthFixed);

					ImGui::TableNextRow();
					if (ImGui::TableSetColumnIndex(0))
					{
						ShowConsoleTraceOutput("TEST", "This is an example trace output.");
					}

					ImGui::TableNextRow();
					if (ImGui::TableNextColumn())
					{
						ShowConsoleWarningOutput("TEST", "This is an example warning output.");
					}

					ImGui::TableNextRow();
					if (ImGui::TableNextColumn())
					{
						ShowConsoleErrorOutput("TEST", "This is an example error output!");
					}

					for (int i = 0; i < 50; i++) {
						ImGui::TableNextRow();
						if (ImGui::TableNextColumn())
						{
							ShowConsoleTraceOutput("TEST", std::format("Output {}", i).c_str());
						}
					}

				}
				ImGui::EndTable();
			}
			ImGui::End();
		}
	}

	void ImGuiLayer::ShowControlsModule()
	{
		// control keybinds - hardcoded because not planning on being customizable
		std::vector<std::pair<const char*, const char*>> ControlTextMap =
		{
			{"Undo", "CTRL + Z"},
			{"Redo", "CTRL + Y"},
			{"Rotate Viewport Angle", "ALT + MMB"},
			{"Zoom In","Mouse Scroll Down"},
			{"Zoom Out","Mouse Scroll Up"},
			{"Create Camera From View","CTRL + SHIFT + C"},
			{"Move","W"},
			{"Rotate","E"},
			{"Scale","R"},
			{"Duplicate","CTRL + D"},
			{"Delete","DELETE"}
		};

		if (showControlsModule)
		{
			if (ImGui::Begin("Controls", &showControlsModule))
			{
				// HelpMarker("Control keybinds in Orion are not currently editable!");
				static ImGuiTableFlags flags =
					ImGuiTableFlags_SizingFixedFit |
					ImGuiTableFlags_Hideable;

				if (ImGui::BeginTable("ControlsTable", 3, flags))
				{
					// make control name column fixed width, keybind column stretch
					ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Keybind", ImGuiTableColumnFlags_WidthStretch);
					auto it = ControlTextMap.begin();

					// set up all row entries
					for (int row = 0; row < ControlTextMap.size(); row++)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text(it->first);
						ImGui::TableSetColumnIndex(1);
						ImGui::Text(it->second);

						// iterate to next control keybind to print
						it++;
					}
					ImGui::EndTable();
				}
				ImGui::End();
			}
		}
	}

	// ========================== PROJECT SETTINGS ==========================

	void ImGuiLayer::ShowProjectSettingsWindow()
	{
		if (!showProjectSettings)
			return;

		ProjectSettings& settings = ProjectSettings::Get();

		ImGui::SetNextWindowSize(ImVec2(480, 520), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Project Settings", &showProjectSettings))
		{
			// ---------- Project Info ----------
			if (ImGui::CollapsingHeader("Project Info", ImGuiTreeNodeFlags_DefaultOpen))
			{
				char nameBuf[256];
				strncpy_s(nameBuf, settings.projectName.c_str(), sizeof(nameBuf) - 1);
				nameBuf[sizeof(nameBuf) - 1] = '\0';
				if (ImGui::InputText("Project Name", nameBuf, sizeof(nameBuf)))
					settings.projectName = nameBuf;

				char sceneBuf[512];
				strncpy_s(sceneBuf, settings.startingScene.c_str(), sizeof(sceneBuf) - 1);
				sceneBuf[sizeof(sceneBuf) - 1] = '\0';
				if (ImGui::InputText("Starting Scene", sceneBuf, sizeof(sceneBuf)))
					settings.startingScene = sceneBuf;
			}

			ImGui::Separator();

			// ---------- Background / Sky ----------
			if (ImGui::CollapsingHeader("Background", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Mode selector dropdown
				const char* modeNames[] = { "Solid Color", "Gradient", "Cubemap (coming soon)" };
				int currentMode = static_cast<int>(settings.backgroundMode);
				if (ImGui::Combo("Mode", &currentMode, modeNames, IM_ARRAYSIZE(modeNames)))
				{
					// Don't allow selecting Cubemap yet — snap back to current if attempted
					if (currentMode == static_cast<int>(BackgroundMode::Cubemap))
						currentMode = static_cast<int>(settings.backgroundMode);
					settings.backgroundMode = static_cast<BackgroundMode>(currentMode);
				}

				// Show mode-specific controls
				switch (settings.backgroundMode)
				{
					case BackgroundMode::SolidColor:
					{
						ImGui::ColorEdit3("Color", &settings.solidColor.x);
						break;
					}
					case BackgroundMode::Gradient:
					{
						ImGui::ColorEdit3("Top Color", &settings.gradientTopColor.x);
						ImGui::ColorEdit3("Bottom Color", &settings.gradientBottomColor.x);
						break;
					}
					case BackgroundMode::Cubemap:
					{
						ImGui::TextDisabled("Cubemap skyboxes are not yet implemented.");
						ImGui::TextDisabled("This will support 6-face cubemap textures.");

						char cubeBuf[512];
						strncpy_s(cubeBuf, settings.cubemapPath.c_str(), sizeof(cubeBuf) - 1);
						cubeBuf[sizeof(cubeBuf) - 1] = '\0';

						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						ImGui::InputText("Cubemap Path", cubeBuf, sizeof(cubeBuf));
						ImGui::PopStyleVar();
						break;
					}
				}
			}

			ImGui::Separator();

			// ---------- Lighting ----------
			if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Directional Light (Sun)");
				ImGui::DragFloat3("Direction", &settings.sunDirection.x, 0.01f, -1.0f, 1.0f);
				ImGui::ColorEdit3("Sun Color", &settings.sunColor.x);
				ImGui::DragFloat("Sun Intensity", &settings.sunIntensity, 0.01f, 0.0f, 10.0f);

				//ImGui::Separator();
				//ImGui::Text("Ambient");
				//ImGui::DragFloat("Ambient Intensity", &settings.ambientIntensity, 0.005f, 0.0f, 1.0f);
				//ImGui::TextDisabled("(Ambient uniform not yet wired to shader)");
			}

			ImGui::Separator();

			// ---------- Language ----------
			if (ImGui::CollapsingHeader("Language", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* languages[] = { "English", "Spanish" };
				static int language_current = 0;
				ImGui::Combo("Choose Editor Language", &language_current, languages, IM_COUNTOF(languages));
			}

			ImGui::Separator();

			// ---------- Debug / Editor ----------
			if (ImGui::CollapsingHeader("Debug Rendering", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Show World Grid", &settings.showGrid);
				if (settings.showGrid) {
					ImGui::DragFloat("Grid Spacing", &settings.gridSpacing, 0.25f, 0.25f, 20.0f, "%.2f");
					ImGui::DragFloat("Grid Extent", &settings.gridHalfExtent, 5.0f, 10.0f, 500.0f, "%.0f");
				}

			}
		
		ImGui::End();
		}

	}
}