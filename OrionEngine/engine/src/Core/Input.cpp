#include "Core/Input.h"
#include "Application.h"
#include "Events/MouseEvent.h"

#include <GLFW/glfw3.h>
#include <array>
#include <cctype>

namespace Orion {

    // GLFW_KEY_LAST is 348; allocate room for the full range.
    static constexpr int kKeyArraySize = GLFW_KEY_LAST + 1;
    static constexpr int kMouseArraySize = GLFW_MOUSE_BUTTON_LAST + 1;

    // Previous and current frame held state, sampled from GLFW at NewFrame().
    // "Pressed" = current && !prev; "Released" = !current && prev.
    static std::array<bool, kKeyArraySize>   s_KeyPrev{};
    static std::array<bool, kKeyArraySize>   s_KeyCurr{};
    static std::array<bool, kMouseArraySize> s_MousePrev{};
    static std::array<bool, kMouseArraySize> s_MouseCurr{};

    // Mouse tracking. Initialized lazily on the first NewFrame so the first
    // frame's delta is (0, 0) instead of a huge jump from the default (0, 0)
    // to wherever the cursor happens to be.
    static glm::vec2 s_MousePos{ 0.0f, 0.0f };
    static glm::vec2 s_MouseDelta{ 0.0f, 0.0f };
    static bool s_MouseInitialized = false;

    // Scroll delta is event-driven and accumulates between NewFrame() calls.
    static float s_ScrollDelta = 0.0f;
    static float s_ScrollDeltaAccum = 0.0f;

    // Fetch the GLFW window handle from the Application. Returns nullptr if
    // the Application or window isn't ready yet (e.g. during very early init).
    static GLFWwindow* GetWindow()
    {
        return static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    }

    void Input::NewFrame()
    {
        GLFWwindow* window = GetWindow();
        if (!window)
            return;

        // Keyboard: shift current -> previous, re-poll current from GLFW.
        // Only the range [GLFW_KEY_SPACE, GLFW_KEY_LAST] is valid for glfwGetKey;
        // passing anything else triggers GLFW_INVALID_ENUM. Slots below
        // GLFW_KEY_SPACE stay permanently false (they're never valid codes).
        s_KeyPrev = s_KeyCurr;
        for (int code = GLFW_KEY_SPACE; code <= GLFW_KEY_LAST; ++code) {
            s_KeyCurr[code] = (glfwGetKey(window, code) == GLFW_PRESS);
        }

        // Mouse buttons: valid range is [0, GLFW_MOUSE_BUTTON_LAST].
        s_MousePrev = s_MouseCurr;
        for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
            s_MouseCurr[b] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
        }

        // Mouse position & delta.
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        glm::vec2 newPos{ (float)mx, (float)my };

        if (!s_MouseInitialized) {
            s_MousePos = newPos;
            s_MouseDelta = { 0.0f, 0.0f };
            s_MouseInitialized = true;
        } else {
            s_MouseDelta = newPos - s_MousePos;
            s_MousePos = newPos;
        }

        // Scroll: publish the accumulator built up by OnEvent during the previous
        // frame, then reset it for the next.
        s_ScrollDelta = s_ScrollDeltaAccum;
        s_ScrollDeltaAccum = 0.0f;
    }

    void Input::OnEvent(Event& event)
    {
        // Accumulate scroll Y offset across whatever scroll events fire this frame.
        // GLFW deliver fractional scroll on some devices, so we use float.
        if (event.GetEventType() == EventType::MouseScrolled) {
            auto& e = static_cast<MouseScrolledEvent&>(event);
            s_ScrollDeltaAccum += e.GetYOffset();
        }
    }

    // ----- Keyboard queries -----

    bool Input::IsKeyDown(int keyCode)
    {
        if (keyCode < 0 || keyCode >= kKeyArraySize) return false;
        return s_KeyCurr[keyCode];
    }

    bool Input::IsKeyPressed(int keyCode)
    {
        if (keyCode < 0 || keyCode >= kKeyArraySize) return false;
        return s_KeyCurr[keyCode] && !s_KeyPrev[keyCode];
    }

    bool Input::IsKeyReleased(int keyCode)
    {
        if (keyCode < 0 || keyCode >= kKeyArraySize) return false;
        return !s_KeyCurr[keyCode] && s_KeyPrev[keyCode];
    }

    // ----- Mouse queries -----

    bool Input::IsMouseButtonDown(int button)
    {
        if (button < 0 || button >= kMouseArraySize) return false;
        return s_MouseCurr[button];
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        if (button < 0 || button >= kMouseArraySize) return false;
        return s_MouseCurr[button] && !s_MousePrev[button];
    }

    bool Input::IsMouseButtonReleased(int button)
    {
        if (button < 0 || button >= kMouseArraySize) return false;
        return !s_MouseCurr[button] && s_MousePrev[button];
    }

    glm::vec2 Input::GetMousePosition() { return s_MousePos; }
    glm::vec2 Input::GetMouseDelta()    { return s_MouseDelta; }
    float     Input::GetScrollDelta()   { return s_ScrollDelta; }

    // ----- Axis -----

    float Input::GetAxis(const std::string& axisName)
    {
        if (axisName == "Horizontal") {
            float v = 0.0f;
            if (IsKeyDown(GLFW_KEY_D) || IsKeyDown(GLFW_KEY_RIGHT)) v += 1.0f;
            if (IsKeyDown(GLFW_KEY_A) || IsKeyDown(GLFW_KEY_LEFT))  v -= 1.0f;
            return v;
        }
        if (axisName == "Vertical") {
            float v = 0.0f;
            if (IsKeyDown(GLFW_KEY_W) || IsKeyDown(GLFW_KEY_UP))    v += 1.0f;
            if (IsKeyDown(GLFW_KEY_S) || IsKeyDown(GLFW_KEY_DOWN))  v -= 1.0f;
            return v;
        }
        return 0.0f;
    }

    int Input::KeyNameToCode(const std::string& name)
    {
        // Single A-Z or 0-9 character.
        if (name.size() == 1) {
            char c = name[0];
            if (c >= 'A' && c <= 'Z') return GLFW_KEY_A + (c - 'A');
            if (c >= 'a' && c <= 'z') return GLFW_KEY_A + (c - 'a');
            if (c >= '0' && c <= '9') return GLFW_KEY_0 + (c - '0');
            return -1;
        }

        if (name == "Space")       return GLFW_KEY_SPACE;
        if (name == "Enter")       return GLFW_KEY_ENTER;
        if (name == "Escape")      return GLFW_KEY_ESCAPE;
        if (name == "Tab")         return GLFW_KEY_TAB;
        if (name == "LeftShift")   return GLFW_KEY_LEFT_SHIFT;
        if (name == "RightShift")  return GLFW_KEY_RIGHT_SHIFT;
        if (name == "LeftCtrl")    return GLFW_KEY_LEFT_CONTROL;
        if (name == "RightCtrl")   return GLFW_KEY_RIGHT_CONTROL;
        if (name == "Up")          return GLFW_KEY_UP;
        if (name == "Down")        return GLFW_KEY_DOWN;
        if (name == "Left")        return GLFW_KEY_LEFT;
        if (name == "Right")       return GLFW_KEY_RIGHT;
        return -1;
    }

}
