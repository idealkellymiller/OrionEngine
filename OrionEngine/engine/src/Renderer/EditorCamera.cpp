#include "Renderer/EditorCamera.h"
#include "Renderer/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm> // for std::clamp
#include <cmath>
#include <cstring> // for memset

// GLFW key/button codes used by events (match GLFW defines without including glfw3.h)
namespace KeyCode {
    constexpr int W          = 87;
    constexpr int A          = 65;
    constexpr int S          = 83;
    constexpr int D          = 68;
    constexpr int Q          = 81;
    constexpr int E          = 69;
    constexpr int LeftShift  = 340;
}
namespace MouseCode {
    constexpr int ButtonRight = 1;  // GLFW_MOUSE_BUTTON_RIGHT
}


namespace Orion {

    EditorCamera::EditorCamera()
        : m_Position(0.0f, 0.0f, 5.0f),
        m_Yaw(-90.0f),          // -90 means looking down -Z initially
        m_Pitch(0.0f),
        m_Forward(0.0f, 0.0f, -1.0f),
        m_Right(1.0f, 0.0f, 0.0f),
        m_Up(0.0f, 1.0f, 0.0f),
        m_MoveSpeed(5.0f),
        m_MouseSensitivity(0.12f),
        m_RightMouseDown(false),
        m_MouseX(0.0f),
        m_MouseY(0.0f),
        m_ScrollDelta(0.0f),
        m_FirstMouseFrame(true),
        m_LastMouseX(0.0f),
        m_LastMouseY(0.0f)
    {
        std::memset(m_KeyStates, 0, sizeof(m_KeyStates));
        UpdateVectors();
    }

    void EditorCamera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
    }

    void EditorCamera::SetYawPitch(float yawDegrees, float pitchDegrees)
    {
        m_Yaw = yawDegrees;
        m_Pitch = std::clamp(pitchDegrees, -89.0f, 89.0f);
        UpdateVectors();
    }

    void EditorCamera::FocusOn(const glm::vec3& target, float distance)
    {
        m_Position = target - m_Forward * distance;
    }

    void EditorCamera::SetMoveSpeed(float speed)
    {
        m_MoveSpeed = (speed < 0.1f) ? 0.1f : speed;
    }

    // ---------- Event-driven input setters ----------

    void EditorCamera::OnKeyPressed(int keyCode)
    {
        if (keyCode >= 0 && keyCode < MAX_KEYS)
            m_KeyStates[keyCode] = true;
    }

    void EditorCamera::OnKeyReleased(int keyCode)
    {
        if (keyCode >= 0 && keyCode < MAX_KEYS)
            m_KeyStates[keyCode] = false;
    }

    void EditorCamera::OnMouseMoved(float x, float y)
    {
        m_MouseX = x;
        m_MouseY = y;
    }

    void EditorCamera::OnMouseButtonPressed(int button)
    {
        if (button == MouseCode::ButtonRight)
            m_RightMouseDown = true;
    }

    void EditorCamera::OnMouseButtonReleased(int button)
    {
        if (button == MouseCode::ButtonRight) {
            m_RightMouseDown = false;
            // Reset so next RMB press doesn't produce a delta jump
            m_FirstMouseFrame = true;
        }
    }

    void EditorCamera::OnMouseScrolled(float yOffset)
    {
        m_ScrollDelta += yOffset;
    }

    // ---------- Per-frame update ----------

    void EditorCamera::Update(
        Camera* camera,
        float deltaTime,
        bool viewportHovered,
        bool viewportFocused)
    {
        // Only allow editor camera controls when the viewport is relevant.
        bool canControl = viewportHovered || viewportFocused;

        if (canControl && m_RightMouseDown) {
            UpdateMouseLook(deltaTime);
            UpdateMovement(deltaTime);
        }
        else {
            // Reset delta startup so next RMB press does not jump.
            m_FirstMouseFrame = true;

            // Clear all key states to prevent "sticky keys".
            // A key-down event can reach us while the viewport is focused, but the
            // matching key-up may be swallowed by ImGui if focus moved to another panel.
            // Clearing here ensures no phantom movement on the next RMB fly.
            std::memset(m_KeyStates, 0, sizeof(m_KeyStates));
        }

        // Consume accumulated scroll delta
        float scrollDelta = m_ScrollDelta;
        m_ScrollDelta = 0.0f;

        if (canControl && scrollDelta != 0.0f)
        {
            // Mouse wheel changes fly speed
            m_MoveSpeed += scrollDelta * 0.5f;

            if (m_MoveSpeed < 0.5f)
                m_MoveSpeed = 0.5f;
        }

        // Push final transform into Orion Camera
        camera->SetPosition(m_Position);
        camera->SetTarget(m_Position + m_Forward);
        camera->SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void EditorCamera::UpdateMouseLook(float deltaTime)
    {
        (void)deltaTime;

        // On the first RMB frame, initialize previous position so we do not snap.
        if (m_FirstMouseFrame) {
            m_LastMouseX = m_MouseX;
            m_LastMouseY = m_MouseY;
            m_FirstMouseFrame = false;
        }

        float deltaX = m_MouseX - m_LastMouseX;
        float deltaY = m_MouseY - m_LastMouseY;

        m_LastMouseX = m_MouseX;
        m_LastMouseY = m_MouseY;

        // Yaw rotates left/right, pitch rotates up/down
        m_Yaw += deltaX * m_MouseSensitivity;
        m_Pitch -= deltaY * m_MouseSensitivity;

        // Clamp pitch so we do not flip upside down
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

        UpdateVectors();
    }

    void EditorCamera::UpdateMovement(float deltaTime)
    {
        // Shift makes movement faster, common in editors
        float speedMultiplier = 1.0f;
        if (m_KeyStates[KeyCode::LeftShift])
            speedMultiplier = 3.0f;

        float velocity = m_MoveSpeed * speedMultiplier * deltaTime;

        // Move forward/back along look direction
        if (m_KeyStates[KeyCode::W])
            m_Position += m_Forward * velocity;
        if (m_KeyStates[KeyCode::S])
            m_Position -= m_Forward * velocity;

        // Strafe left/right
        if (m_KeyStates[KeyCode::A])
            m_Position -= m_Right * velocity;
        if (m_KeyStates[KeyCode::D])
            m_Position += m_Right * velocity;

        // Move vertically in world space
        if (m_KeyStates[KeyCode::E])
            m_Position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
        if (m_KeyStates[KeyCode::Q])
            m_Position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    }

    void EditorCamera::UpdateVectors()
    {
        // Convert yaw/pitch angles into a normalized forward vector
        glm::vec3 forward;
        forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        forward.y = sin(glm::radians(m_Pitch));
        forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

        m_Forward = glm::normalize(forward);

        // Build orthonormal basis from world up
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        m_Right = glm::normalize(glm::cross(m_Forward, worldUp));
        m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
    }
}
