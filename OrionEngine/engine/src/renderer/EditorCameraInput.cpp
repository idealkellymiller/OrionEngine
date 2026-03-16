#include "EditorCameraInput.hpp"


float EditorCameraInput::s_ScrollDelta = 0.0f;

void EditorCameraInput::AddScrollDelta(float delta)
{
	s_ScrollDelta += delta;
}

float EditorCameraInput::ConsumeScrollDelta()
{
	float value = s_ScrollDelta;
	s_ScrollDelta = 0.0f;
	return value;
}