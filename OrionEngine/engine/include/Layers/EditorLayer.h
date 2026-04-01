#pragma once
#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "ECS/Scene.h"


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

	private:
		static EntityID s_SelectedEntity;
	};
}