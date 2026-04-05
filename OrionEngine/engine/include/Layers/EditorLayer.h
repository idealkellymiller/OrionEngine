#pragma once
#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "ECS/Scene.h"
#include <Renderer/EditorCamera.h>

#include <string>


namespace Orion {

	class ORION_API EditorLayer : public Layer {

	public:
		EditorLayer();
		~EditorLayer();
		void OnAttach();
		void OnDetach();
		void OnUpdate();
		void OnEvent(Event& event);

		static EntityID GetSelectedEntity() { return s_SelectedEntity; }

		void AddPrimitive(std::string primitiveFileName);

	private:
		static EntityID s_SelectedEntity;

		static EditorCamera s_EditorCamera;
	};
}
