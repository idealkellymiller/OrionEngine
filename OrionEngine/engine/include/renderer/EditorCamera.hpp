#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>


class Camera;


// EditorCamera is a controller for a camera.
// It reads input from GLFW and updates position/orientation.
class EditorCamera {
public:
	EditorCamera();

	// Set starting transform
	void SetPosition(const glm::vec3& position);
	void SetYawPitch(float yawDegrees, float pitchDegrees);

	// Per-frame update
	// deltatime = frame time in seconds
	// viewportHovered = mouse is over the viewport window
	// viewportFocused = viewport window is focused/active
	void Update(
		GLFWwindow* window,
		Camera& camera,
		float deltaTime,
		bool viewportHovered,
		bool viewportFocused
	);

	// Optional speed controls
	void SetMoveSpeed(float speed);
	float GetMoveSpeed() const { return m_MoveSpeed; }

	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetForward() const { return m_Forward; }
	glm::vec3 GetRight() const { return m_Right; }
	glm::vec3 GetUp() const { return m_Up; }

private:
	// Rebuild foward/right/up vectors from yaw/pitch
	void UpdateVectors();

	// Handle mouse look
	void UpdateMouseLook(GLFWwindow* window, float deltaTime);

	// Handle WASD/QE movement
	void UpdateMovement(GLFWwindow* window, float deltaTime);

private:
	glm::vec3 m_Position;

	// Camera orientation stored as yaw/pitch in degress
	float m_Yaw;
	float m_Pitch;

	// Basis vectors derived from yaw/pitch
	glm::vec3 m_Forward;
	glm::vec3 m_Right;
	glm::vec3 m_Up;

	// Movement settings
	float m_MoveSpeed;
	float m_MouseSensitivity;

	// RMB state tracking
	bool m_WasRightMouseDown;

	// Last mouse position for delta calculation
	bool m_FirstMouseFrame;
	double m_LastMouseX;
	double m_LastMouseY;
};