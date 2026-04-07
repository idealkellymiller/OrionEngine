#pragma once

#include "EngineCore.h"
#include "Layer.h"

#include <vector>

namespace Orion {

	class ORION_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
	private:
		std::vector<Layer*> m_Layers;
		// Index-based insert point instead of an iterator.
		// Overlays live after this index; regular layers live before it.
		// Using an index avoids invalidation when the vector reallocates.
		size_t m_LayerInsertIndex = 0;
	};

}
