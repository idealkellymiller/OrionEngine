#pragma once

#include "Actions/Action.h"

namespace Orion {

	class ORION_API ActionStack
	{
		public:
			ActionStack();
			~ActionStack();

			void Undo();
			void Redo();

			void PushUndoAction(std::shared_ptr<Action> action);
			Action& PopUndoAction();

			void PushRedoAction(Action& action);
			Action& PopRedoAction();

		private:
			std::vector<std::shared_ptr<Action>> m_UndoActions;
			std::vector<std::shared_ptr<Action>> m_RedoActions;

			size_t m_UndoActionInsertIndex = 0;
			size_t m_RedoActionInsertIndex = 0;
	};
}