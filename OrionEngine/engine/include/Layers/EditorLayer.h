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

	private:
		static EntityID m_SelectedEntity;
	};
}