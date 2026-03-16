#include "EditorCamera.hpp"
#include "Camera.hpp"
#include "EditorCameraInput.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm> // for std::clamp
#include <cmath>


EditorCamera::EditorCamera()
    : m_Position(0.0f, 0.0f, 5.0f),
    m_Yaw(-90.0f),          // -90 means looking down -Z initially
    m_Pitch(0.0f),
    m_Forward(0.0f, 0.0f, -1.0f),
    m_Right(1.0f, 0.0f, 0.0f),
    m_Up(0.0f, 1.0f, 0.0f),
    m_MoveSpeed(5.0f),
    m_MouseSensitivity(0.12f),
    m_WasRightMouseDown(false),
    m_FirstMouseFrame(true),
    m_LastMouseX(0.0),
    m_LastMouseY(0.0)
{
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

void EditorCamera::SetMoveSpeed(float speed)
{
    m_MoveSpeed = (speed < 0.1f) ? 0.1f : speed;
}

void EditorCamera::Update(
    GLFWwindow* window,
    Camera& camera,
    float deltaTime,
    bool viewportHovered,
    bool viewportFocused)
{
    // Only allow editor camera controls when the viewport is relevant.
    // hovered us useful for wheel/interaction.
    // Focsed helps ensure keyboard goes to the viewport.
    bool canControl = viewportHovered || viewportFocused;

    // Right mouse held means "Capture editor camera controls".
    bool rightMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    if (canControl && rightMouseDown) {
        UpdateMouseLook(window, deltaTime);
        UpdateMovement(window, deltaTime);
    }
    else {
        // Reset delta startup so next RMB press does not jump.
        m_FirstMouseFrame = true;
    }

    m_WasRightMouseDown = rightMouseDown;

    float scrollDelta = EditorCameraInput::ConsumeScrollDelta();
    if (canControl && scrollDelta != 0.0f)
    {
        // Mouse wheel changes fly speed
        m_MoveSpeed += scrollDelta * 0.5f;

        if (m_MoveSpeed < 0.5f)
            m_MoveSpeed = 0.5f;
    }

    // Push final transform into Orion Camera
    camera.SetPosition(m_Position);
    camera.SetTarget(m_Position + m_Forward);
    camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
}

void EditorCamera::UpdateMouseLook(GLFWwindow* window, float deltaTime)
{
    (void)deltaTime;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // ON the first RMB frame, initialize previous position so we do not snap.
    if (m_FirstMouseFrame) {
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;
        m_FirstMouseFrame = false;
    }

    double deltaX = mouseX - m_LastMouseX;
    double deltaY = mouseY - m_LastMouseY;

    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;

    // Yaw rotates left/right, pitch rotates up/down
    m_Yaw += static_cast<float>(deltaX) * m_MouseSensitivity;
    m_Pitch -= static_cast<float>(deltaY) * m_MouseSensitivity;

    // Clamp pitch so we do not flip upside down
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    UpdateVectors();
}

void EditorCamera::UpdateMovement(GLFWwindow* window, float deltaTime)
{
    // Dhift makes movement faster, common in editors
    float speedMultiplier = 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speedMultiplier = 3.0f;

    float velocity = m_MoveSpeed * speedMultiplier * deltaTime;

    // Move forward/back along look direction
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        m_Position += m_Forward * velocity;
    } 
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        m_Position -= m_Forward * velocity;
    }

    // Strafe left/right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        m_Position -= m_Right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        m_Position += m_Right * velocity;
    }

    // Move vertically in world space
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        m_Position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        m_Position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    }
}

void EditorCamera::UpdateVectors()
{
    // convert yaw/pitch angle sinto a normalized fowarad vector
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
