#pragma once
#include "EngineCore.h"
#include <glm/glm.hpp>


namespace Orion {

	class Camera;


	// EditorCamera is a fly-camera controller for the editor viewport.
	// Input state is fed via On*() methods (called from EditorLayer::OnEvent)
	// and consumed each frame in Update().
	class ORION_API EditorCamera {
	public:
		EditorCamera();

		// Set starting transform
		void SetPosition(const glm::vec3& position);
		void SetYawPitch(float yawDegrees, float pitchDegrees);

		// --- Event-driven input (called from EditorLayer::OnEvent) ---
		void OnKeyPressed(int keyCode);
		void OnKeyReleased(int keyCode);
		void OnMouseMoved(float x, float y);
		void OnMouseButtonPressed(int button);
		void OnMouseButtonReleased(int button);
		void OnMouseScrolled(float yOffset);

		// Per-frame update — reads accumulated input state, applies movement,
		// and writes the result into the given Camera.
		// viewportHovered/viewportFocused gate whether input is acted on.
		void Update(
			Camera* camera,
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
		// Rebuild forward/right/up vectors from yaw/pitch
		void UpdateVectors();

		// Handle mouse look (reads from stored mouse state)
		void UpdateMouseLook(float deltaTime);

		// Handle WASD/QE movement (reads from stored key state)
		void UpdateMovement(float deltaTime);

	private:
		glm::vec3 m_Position;

		// Camera orientation stored as yaw/pitch in degrees
		float m_Yaw;
		float m_Pitch;

		// Basis vectors derived from yaw/pitch
		glm::vec3 m_Forward;
		glm::vec3 m_Right;
		glm::vec3 m_Up;

		// Movement settings
		float m_MoveSpeed;
		float m_MouseSensitivity;

		// --- Input state fed by events ---
		static constexpr int MAX_KEYS = 512;
		bool m_KeyStates[MAX_KEYS];

		bool m_RightMouseDown;
		float m_MouseX;
		float m_MouseY;
		float m_ScrollDelta;  // accumulated between frames, consumed in Update()

		// Mouse-look delta tracking
		bool m_FirstMouseFrame;
		float m_LastMouseX;
		float m_LastMouseY;
	};
}
