#include "Layers/LayerStack.h"

namespace Orion {

	LayerStack::LayerStack()
	{
		m_LayerInsert = m_Layers.begin();
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
			delete layer;
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_LayerInsert = m_Layers.emplace(m_LayerInsert, layer);
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			auto erasedIndex = std::distance(m_Layers.begin(), it);
			auto insertIndex = std::distance(m_Layers.begin(), m_LayerInsert);

			layer->OnDetach();
			m_Layers.erase(it);

			// Only shift the insert point back if the erased element was before it.
			// If the erased element WAS the insert point (or after), keep the same index.
			if (erasedIndex < insertIndex)
				m_LayerInsert = m_Layers.begin() + (insertIndex - 1);
			else
				m_LayerInsert = m_Layers.begin() + insertIndex;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
	}

}