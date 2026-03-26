#pragma once
#include "EngineCore.h"

namespace Orion {

    class ORION_API EditorCameraInput
    {
    public:
        static void AddScrollDelta(float delta);
        static float ConsumeScrollDelta();

    private:
        static float s_ScrollDelta;
    };
}