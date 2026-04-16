#pragma once
#include "EngineCore.h"
#include "ECS/Scene.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>

#include <unordered_map>
#include <memory>
#include <functional>

namespace Orion {

	// Callback signature: (EntityID self, EntityID other, bool isTrigger)
	using CollisionCallback = std::function<void(EntityID, EntityID, bool)>;

	// Result of a single raycast against the physics world.
	// `entity` is INVALID_ENTITY if the hit body has no associated ECS entity
	// (shouldn't happen in normal use, but guarded for safety).
	struct RaycastHit {
		EntityID  entity   = INVALID_ENTITY;
		glm::vec3 point    = { 0.0f, 0.0f, 0.0f };  // world-space hit position
		glm::vec3 normal   = { 0.0f, 0.0f, 0.0f };  // surface normal at hit point (world-space)
		float     distance = 0.0f;                  // distance from origin along the ray
	};

	class ORION_API PhysicsWorld {
	public:
		PhysicsWorld();
		~PhysicsWorld();

		// Build the Jolt world from a scene's Rigidbody + Collider components.
		// Call once when play mode starts.
		void Init(std::shared_ptr<Scene> scene);

		// Advance the simulation by dt seconds.
		// Also syncs Jolt body transforms back into ECS TransformComponents.
		void Step(float dt);

		// Tear down Jolt world.
		void Shutdown();

		// --- Forces / impulses (called by Lua scripting) ---
		void AddForce(EntityID entity, const glm::vec3& force);
		void AddImpulse(EntityID entity, const glm::vec3& impulse);
		void AddTorque(EntityID entity, const glm::vec3& torque);
		void SetLinearVelocity(EntityID entity, const glm::vec3& velocity);
		glm::vec3 GetLinearVelocity(EntityID entity) const;

		// --- Queries ---
		// Cast a ray from `origin` in `direction` (need not be unit length — internally
		// normalized) up to `maxDistance` world units. Returns true on hit, populating
		// `outHit` with the closest body intersected. Returns false (and doesn't touch
		// outHit) on miss or if direction is zero-length.
		bool Raycast(const glm::vec3& origin, const glm::vec3& direction,
		             float maxDistance, RaycastHit& outHit) const;

		// --- Collision callbacks ---
		void SetCollisionCallback(CollisionCallback cb) { m_CollisionCallback = cb; }

		bool IsInitialized() const { return m_Initialized; }

	private:
		// Creates a Jolt body for one entity and adds it to the physics system.
		void CreateBody(EntityID entity, Scene& scene);

		// Jolt requires these helper objects
		std::unique_ptr<JPH::TempAllocatorImpl> m_TempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;
		std::unique_ptr<JPH::PhysicsSystem> m_PhysicsSystem;

		// Map EntityID <-> Jolt BodyID for runtime lookups
		std::unordered_map<EntityID, JPH::BodyID> m_EntityToBody;
		std::unordered_map<uint32_t, EntityID> m_BodyIndexToEntity;

		std::shared_ptr<Scene> m_Scene;

		CollisionCallback m_CollisionCallback;

		bool m_Initialized = false;

		// Accumulator for fixed-timestep physics
		float m_Accumulator = 0.0f;
		static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
		static constexpr int MAX_SUBSTEPS = 4;
	};

}
