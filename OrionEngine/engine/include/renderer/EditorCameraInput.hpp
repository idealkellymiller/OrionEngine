#pragma once

class EditorCameraInput
{
public:
    static void AddScrollDelta(float delta);
    static float ConsumeScrollDelta();

private:
    static float s_ScrollDelta;
};