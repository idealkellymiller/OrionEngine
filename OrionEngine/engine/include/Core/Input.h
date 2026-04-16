// ============================================================
// Input — per-frame polling cache on top of GLFW
// ============================================================
// Tracks the difference between the previous frame's key/button state
// and the current frame's, so callers can distinguish "held" from
// "went down this frame" (pressed) and "went up this frame" (released).
//
// Also accumulates per-frame mouse motion delta and scroll delta.
//
// Call flow:
//   Application::Run loop -> Input::NewFrame() once per frame, before layer updates.
//   Application::OnEvent -> Input::OnEvent(e) so scroll events are captured.
//
// Anyone — scripts, editor, gameplay — can then query from any thread-less context.
// ============================================================

#pragma once
#include "EngineCore.h"
#include <string>
#include <glm/glm.hpp>

namespace Orion {

    class Event;

    class ORION_API Input {
    public:
        // Called once per frame by Application before layer updates.
        // Copies current -> previous state, re-polls GLFW, resets scroll accumulator,
        // and recomputes mouse delta from the current cursor position.
        static void NewFrame();

        // Forward events from Application::OnEvent so we can accumulate scroll delta.
        static void OnEvent(Event& event);

        // ----- Keyboard -----
        // keyCode is a GLFW_KEY_* constant.
        static bool IsKeyDown(int keyCode);       // currently held
        static bool IsKeyPressed(int keyCode);    // went down this frame
        static bool IsKeyReleased(int keyCode);   // went up this frame

        // ----- Mouse buttons -----
        // button is a GLFW_MOUSE_BUTTON_* constant (0 = left, 1 = right, 2 = middle).
        static bool IsMouseButtonDown(int button);
        static bool IsMouseButtonPressed(int button);
        static bool IsMouseButtonReleased(int button);

        // ----- Mouse motion / scroll -----
        static glm::vec2 GetMousePosition();  // in window pixels
        static glm::vec2 GetMouseDelta();     // pixels moved since previous frame
        static float GetScrollDelta();        // vertical scroll accumulated this frame

        // ----- Axes -----
        // "Horizontal" -> A/Left = -1, D/Right = +1 (0 if both or neither)
        // "Vertical"   -> S/Down = -1, W/Up    = +1
        // Returns 0 for unknown axis names.
        static float GetAxis(const std::string& axisName);

        // Resolve a Lua-friendly key name like "Space" or "A" to a GLFW key code.
        // Returns -1 if unknown. Single letters A-Z (any case) and digits 0-9 work;
        // named keys: Space, Enter, Escape, Tab, LeftShift, RightShift, LeftCtrl,
        // RightCtrl, Up, Down, Left, Right.
        static int KeyNameToCode(const std::string& name);
    };

}
