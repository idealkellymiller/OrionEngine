#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>


void Camera::SetPosition(const glm::vec3& position)
{
	m_Position = position;
}

void Camera::SetTarget(const glm::vec3& target)
{
    m_Target = target;
}

void Camera::SetUp(const glm::vec3& up)
{
    m_Up = up;
}

void Camera::SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane)
{
    m_FOVDegrees = fovDegrees;
    m_AspectRatio = aspectRatio;
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
}

glm::mat4 Camera::GetViewMatrix() const
{
    // Builds a view matrix from camera position/orientation data.
    //
    // glm::lookAt:
    // - first parameter: where the camera is
    // - second parameter: what the camera is looking at
    // - third parameter: which direction counts as "up"
    return glm::lookAt(m_Position, m_Target, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    // Builds a perspective projection matrix using the camera's lens settings.
    //
    // glm::perspective expects:
    // - field of view in radians
    // - aspect ratio
    // - near clip plane
    // - far clip plane
    return glm::perspective(
        glm::radians(m_FOVDegrees),
        m_AspectRatio,
        m_NearPlane,
        m_FarPlane
    );
}