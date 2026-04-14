#include "Actions/ActionStack.h"

namespace Orion {
	ActionStack::ActionStack()
	{
	}

	ActionStack::~ActionStack()
	{
	}

	void ActionStack::Undo()
	{
		if (m_UndoActions.empty()) { return; }

		// take top action off undo stack
		std::shared_ptr<Action> action = m_UndoActions.back();
		m_UndoActions.pop_back();

		// undo the action
		action->Undo();

		// add action to redo stack for possible future redo
		m_RedoActions.push_back(action);
	}

	void ActionStack::Redo()
	{
		if (m_RedoActions.empty()) { return; }

		// take top action of redo stack
		std::shared_ptr<Action> action = m_RedoActions.back();
		m_RedoActions.pop_back();

		// redo (re-execute) the action
		action->Execute();

		// add action to the undo stack for possible future undo
		m_UndoActions.push_back(action);
	}

	void ActionStack::PushUndoAction(std::shared_ptr<Action> action)
	{
		// Insert new action undo into undo stack
		m_UndoActions.push_back(action);
		// clear redo stack because of new action
		m_RedoActions.clear();
	}

}