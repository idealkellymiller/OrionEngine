#pragma once
#include <glm/glm.hpp>


class Camera {
public:
	Camera() = default;
	~Camera() = default;

	// POsition / oritentation data
	void SetPosition(const glm::vec3& position);
	void SetTarget(const glm::vec3& target);
	void SetUp(const glm::vec3& up);

	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetTarget() const { return m_Target; }
	const glm::vec3& GetUp() const { return m_Up; }

	// Perspective projection settings
	void SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);

	float GetFOVDegrees() const { return m_FOVDegrees; }
	float GetAspectRatio() const { return m_AspectRatio; }
	float GetNearPlane() const { return m_NearPlane; }
	float GetFarPlane() const { return m_FarPlane; }

	// Matrix generation
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;

private:
	// Camera transform state
	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 m_Target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);

	// Perspective settings
	float m_FOVDegrees = 45.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	float m_NearPlane = 0.1f;
	float m_FarPlane = 100.0f;
};