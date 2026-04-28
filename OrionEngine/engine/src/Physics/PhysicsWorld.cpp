// Jolt requires this to be defined before including any Jolt headers in .cpp files
#include <Jolt/Jolt.h>

#include "Physics/PhysicsWorld.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Body/BodyLock.h>

#include <iostream>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <vector>

// Jolt uses its own namespace
using namespace JPH;

namespace {

	// -------- Object layers (what kind of object) --------
	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING     = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS  = 2;
	}

	// -------- Broad-phase layers (coarse grouping) --------
	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr unsigned int NUM_LAYERS = 2;
	}

	// Jolt trace callback
	static void JoltTraceImpl(const char* inFmt, ...)
	{
		va_list args;
		va_start(args, inFmt);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFmt, args);
		va_end(args);
		std::cout << "[Jolt] " << buffer << "\n";
	}

#ifdef JPH_ENABLE_ASSERTS
	static bool JoltAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, unsigned int inLine)
	{
		std::cout << "[Jolt Assert] " << inFile << ":" << inLine << " (" << inExpression << ") " << (inMessage ? inMessage : "") << "\n";
		return true; // break into debugger
	}
#endif

} // anonymous namespace

namespace Orion {

	// ============================================================
	// Contact listener — queues collision events for main-thread dispatch
	// ============================================================
	// OnContactAdded is called from Jolt's job-system threads during
	// PhysicsSystem::Update().  Calling Lua (or any non-thread-safe code)
	// directly from here causes crashes when many collisions happen at once.
	// Instead we push events into a mutex-protected queue and flush them on
	// the main thread after Update() returns.
	struct PendingCollision {
		EntityID a;
		EntityID b;
		bool     isTrigger;
	};

	class OrionContactListener : public JPH::ContactListener {
	public:
		const std::unordered_map<uint32_t, EntityID>* bodyToEntity = nullptr;

		// Called from Jolt worker threads — ONLY push to the queue here.
		void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
			const JPH::ContactManifold& /*inManifold*/, JPH::ContactSettings& /*ioSettings*/) override
		{
			if (!bodyToEntity) return;

			auto it1 = bodyToEntity->find(inBody1.GetID().GetIndex());
			auto it2 = bodyToEntity->find(inBody2.GetID().GetIndex());
			if (it1 == bodyToEntity->end() || it2 == bodyToEntity->end()) return;

			bool isTrigger = inBody1.IsSensor() || inBody2.IsSensor();

			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Pending.push_back({ it1->second, it2->second, isTrigger });
		}

		// Called on the main thread after PhysicsSystem::Update() returns.
		// Drains the queue and fires the callback safely.
		void FlushEvents(const CollisionCallback& callback)
		{
			std::vector<PendingCollision> events;
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				events.swap(m_Pending);
			}
			for (const auto& ev : events)
				callback(ev.a, ev.b, ev.isTrigger);
		}

	private:
		std::mutex                   m_Mutex;
		std::vector<PendingCollision> m_Pending;
	};

	// ============================================================
	// PhysicsWorld implementation
	// ============================================================

	// Static contact listener instance (lives alongside PhysicsWorld)
	static OrionContactListener s_ContactListener;

	PhysicsWorld::PhysicsWorld() = default;
	PhysicsWorld::~PhysicsWorld()
	{
		if (m_Initialized)
			Shutdown();
	}

	void PhysicsWorld::Init(std::shared_ptr<Scene> scene)
	{
		if (m_Initialized)
			Shutdown();

		m_Scene = scene;
		m_Accumulator = 0.0f;

		// ---- Jolt global init (only once per process) ----
		static bool s_JoltInitialized = false;
		if (!s_JoltInitialized) {
			JPH::RegisterDefaultAllocator();
			JPH::Trace = JoltTraceImpl;
#ifdef JPH_ENABLE_ASSERTS
			JPH::AssertFailed = JoltAssertFailed;
#endif
			JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();
			s_JoltInitialized = true;
		}

		// ---- Allocators ----
		m_TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB
		int numThreads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
		m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>(
			JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads
		);

		// ---- Layer setup ----
		// Object-layer pair filter must be created FIRST because the broadphase
		// filter reads from it during construction.
		auto* objPairFilter = new JPH::ObjectLayerPairFilterTable(Layers::NUM_LAYERS);
		objPairFilter->EnableCollision(Layers::NON_MOVING, Layers::MOVING);
		objPairFilter->EnableCollision(Layers::MOVING,     Layers::MOVING);

		// BroadPhaseLayerInterfaceTable maps ObjectLayer -> BroadPhaseLayer
		auto* bpLayerInterface = new JPH::BroadPhaseLayerInterfaceTable(Layers::NUM_LAYERS, BroadPhaseLayers::NUM_LAYERS);
		bpLayerInterface->MapObjectToBroadPhaseLayer(Layers::NON_MOVING, BroadPhaseLayers::NON_MOVING);
		bpLayerInterface->MapObjectToBroadPhaseLayer(Layers::MOVING,     BroadPhaseLayers::MOVING);

		// ObjectVsBroadPhaseLayerFilterTable precomputes which object layers can
		// collide with which broadphase layers, using the pair filter above.
		auto* objVsBpFilter = new JPH::ObjectVsBroadPhaseLayerFilterTable(
			*bpLayerInterface, BroadPhaseLayers::NUM_LAYERS,
			*objPairFilter, Layers::NUM_LAYERS
		);

		// ---- Physics system ----
		constexpr uint maxBodies = 4096;
		constexpr uint numBodyMutexes = 0; // auto
		constexpr uint maxBodyPairs = 4096;
		constexpr uint maxContactConstraints = 2048;

		m_PhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
		m_PhysicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
			*bpLayerInterface, *objVsBpFilter, *objPairFilter);

		// Gravity
		m_PhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

		// Contact listener — queues events from Jolt threads; flushed on main thread in Step()
		s_ContactListener.bodyToEntity = &m_BodyIndexToEntity;
		m_PhysicsSystem->SetContactListener(&s_ContactListener);

		// ---- Create bodies for every entity that has Rigidbody + Collider ----
		for (EntityID entity : scene->GetEntities()) {
			if (scene->HasRigidbodyComponent(entity) && scene->HasColliderComponent(entity))
				CreateBody(entity, *scene);
		}

		// Optimize broadphase after bulk-adding bodies
		m_PhysicsSystem->OptimizeBroadPhase();

		m_Initialized = true;
		std::cout << "[PhysicsWorld] Initialized with " << m_EntityToBody.size() << " bodies.\n";
	}

	void PhysicsWorld::CreateBody(EntityID entity, Scene& scene)
	{
		RigidbodyComponent* rb  = scene.GetRigidbodyComponent(entity);
		ColliderComponent*  col = scene.GetColliderComponent(entity);
		TransformComponent* tc  = scene.GetTransformComponent(entity);
		if (!rb || !col || !tc) return;

		// Collider dimensions are world-space values — the inspector always shows
		// exactly what Jolt receives. When the user drags Scale in the inspector,
		// DrawTransformFields proportionally adjusts these values to stay in sync,
		// so no multiplication by tc->scale is needed here.
		JPH::RefConst<JPH::Shape> shape;
		if (col->shape == ColliderShape::Box) {
			glm::vec3 extents = glm::max(col->boxHalfExtents, glm::vec3(0.001f));
			float convexRadius = std::min(0.05f, std::min({ extents.x, extents.y, extents.z }) * 0.5f);
			shape = new JPH::BoxShape(JPH::Vec3(extents.x, extents.y, extents.z), convexRadius);
		}
		else {
			float radius = std::max(col->sphereRadius, 0.001f);
			shape = new JPH::SphereShape(radius);
		}

		// Position & rotation from Transform
		JPH::RVec3 position(tc->position.x + col->offset.x,
		                     tc->position.y + col->offset.y,
		                     tc->position.z + col->offset.z);

		// Convert Euler to quaternion (matching RenderScene X->Y->Z order)
		JPH::Quat rotation = JPH::Quat::sEulerAngles(JPH::Vec3(tc->rotation.x, tc->rotation.y, tc->rotation.z));

		// Map our BodyType to Jolt's EMotionType and object layer
		JPH::EMotionType motionType;
		JPH::ObjectLayer  objectLayer;
		switch (rb->bodyType) {
			case BodyType::Static:
				motionType  = JPH::EMotionType::Static;
				objectLayer = Layers::NON_MOVING;
				break;
			case BodyType::Kinematic:
				motionType  = JPH::EMotionType::Kinematic;
				objectLayer = Layers::MOVING;
				break;
			case BodyType::Dynamic:
			default:
				motionType  = JPH::EMotionType::Dynamic;
				objectLayer = Layers::MOVING;
				break;
		}

		JPH::EActivation activation = (rb->bodyType == BodyType::Static)
			? JPH::EActivation::DontActivate
			: JPH::EActivation::Activate;

		JPH::BodyCreationSettings bodySettings(shape, position, rotation, motionType, objectLayer);

		// Configure mass / damping
		if (rb->bodyType == BodyType::Dynamic) {
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			bodySettings.mMassPropertiesOverride.mMass = rb->mass;
		}
		bodySettings.mLinearDamping  = rb->linearDamping;
		bodySettings.mAngularDamping = rb->angularDamping;
		bodySettings.mGravityFactor  = rb->gravityScale;
		bodySettings.mIsSensor       = col->isTrigger;

		// Freeze rotation axes
		if (rb->freezeRotationX || rb->freezeRotationY || rb->freezeRotationZ) {
			// Jolt uses AllowedDOFs bitmask
			JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
			if (rb->freezeRotationX) dofs &= ~JPH::EAllowedDOFs::RotationX;
			if (rb->freezeRotationY) dofs &= ~JPH::EAllowedDOFs::RotationY;
			if (rb->freezeRotationZ) dofs &= ~JPH::EAllowedDOFs::RotationZ;
			bodySettings.mAllowedDOFs = dofs;
		}

		// Add body to the physics system
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
		JPH::Body* body = bodyInterface.CreateBody(bodySettings);
		if (!body) {
			std::cout << "[PhysicsWorld] Failed to create body for entity " << entity << "\n";
			return;
		}

		bodyInterface.AddBody(body->GetID(), activation);

		// Store mappings
		m_EntityToBody[entity] = body->GetID();
		m_BodyIndexToEntity[body->GetID().GetIndex()] = entity;

		// Debug logging
		EntityDataComponent* edc = scene.GetEntityDataComponent(entity);
		std::string name = edc ? edc->name : "unnamed";
		const char* typeStr = (rb->bodyType == BodyType::Static) ? "Static" : (rb->bodyType == BodyType::Dynamic) ? "Dynamic" : "Kinematic";
		std::cout << "[PhysicsWorld] Created body for entity " << entity << " (" << name << ")"
		          << " type=" << typeStr
		          << " pos=(" << tc->position.x << "," << tc->position.y << "," << tc->position.z << ")";
		if (col->shape == ColliderShape::Box) {
			glm::vec3 extents = glm::max(col->boxHalfExtents, glm::vec3(0.001f));
			std::cout << " Box(half=" << extents.x << "," << extents.y << "," << extents.z << ")";
		} else {
			std::cout << " Sphere(r=" << std::max(col->sphereRadius, 0.001f) << ")";
		}
		std::cout << "\n";
	}

	void PhysicsWorld::Step(float dt)
	{
		if (!m_Initialized || !m_Scene) return;

		// Fixed timestep accumulator
		m_Accumulator += dt;
		int steps = 0;
		while (m_Accumulator >= FIXED_TIMESTEP && steps < MAX_SUBSTEPS) {
			m_PhysicsSystem->Update(FIXED_TIMESTEP, 1, m_TempAllocator.get(), m_JobSystem.get());
			m_Accumulator -= FIXED_TIMESTEP;
			steps++;

			// Flush queued collision events on the main thread, safely away from
			// Jolt's job threads.  Only fire if a callback is registered.
			if (m_CollisionCallback)
				s_ContactListener.FlushEvents(m_CollisionCallback);
		}

		// Clamp accumulator to prevent spiral of death
		if (m_Accumulator > FIXED_TIMESTEP)
			m_Accumulator = 0.0f;

		// ---- Sync Jolt body transforms back into ECS ----
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		for (auto& [entity, bodyID] : m_EntityToBody) {
			// Only sync dynamic/kinematic bodies (static don't move)
			RigidbodyComponent* rb = m_Scene->GetRigidbodyComponent(entity);
			if (!rb || rb->bodyType == BodyType::Static) continue;

			TransformComponent* tc = m_Scene->GetTransformComponent(entity);
			ColliderComponent*  col = m_Scene->GetColliderComponent(entity);
			if (!tc) continue;

			JPH::RVec3 pos = bodyInterface.GetPosition(bodyID);
			JPH::Quat  rot = bodyInterface.GetRotation(bodyID);

			// Subtract collider offset to get entity position
			glm::vec3 offset = col ? col->offset : glm::vec3(0.0f);
			tc->position = glm::vec3((float)pos.GetX() - offset.x,
			                         (float)pos.GetY() - offset.y,
			                         (float)pos.GetZ() - offset.z);

			// Convert quaternion back to Euler angles
			JPH::Vec3 euler = rot.GetEulerAngles();
			tc->rotation = glm::vec3(euler.GetX(), euler.GetY(), euler.GetZ());
		}
	}

	void PhysicsWorld::Shutdown()
	{
		if (!m_Initialized) return;

		// Remove all bodies
		if (m_PhysicsSystem) {
			JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
			for (auto& [entity, bodyID] : m_EntityToBody) {
				bodyInterface.RemoveBody(bodyID);
				bodyInterface.DestroyBody(bodyID);
			}
		}

		m_EntityToBody.clear();
		m_BodyIndexToEntity.clear();

		m_PhysicsSystem.reset();
		m_JobSystem.reset();
		m_TempAllocator.reset();
		m_Scene.reset();

		m_Initialized = false;
		std::cout << "[PhysicsWorld] Shutdown.\n";
	}

	// ---- Force / impulse API ----

	void PhysicsWorld::AddForce(EntityID entity, const glm::vec3& force)
	{
		if (!m_Initialized) return;
		auto it = m_EntityToBody.find(entity);
		if (it == m_EntityToBody.end()) return;

		JPH::BodyInterface& bi = m_PhysicsSystem->GetBodyInterface();
		bi.AddForce(it->second, JPH::Vec3(force.x, force.y, force.z));
	}

	void PhysicsWorld::AddImpulse(EntityID entity, const glm::vec3& impulse)
	{
		if (!m_Initialized) return;
		auto it = m_EntityToBody.find(entity);
		if (it == m_EntityToBody.end()) return;

		JPH::BodyInterface& bi = m_PhysicsSystem->GetBodyInterface();
		bi.AddImpulse(it->second, JPH::Vec3(impulse.x, impulse.y, impulse.z));
	}

	void PhysicsWorld::AddTorque(EntityID entity, const glm::vec3& torque)
	{
		if (!m_Initialized) return;
		auto it = m_EntityToBody.find(entity);
		if (it == m_EntityToBody.end()) return;

		JPH::BodyInterface& bi = m_PhysicsSystem->GetBodyInterface();
		bi.AddTorque(it->second, JPH::Vec3(torque.x, torque.y, torque.z));
	}

	void PhysicsWorld::SetLinearVelocity(EntityID entity, const glm::vec3& velocity)
	{
		if (!m_Initialized) return;
		auto it = m_EntityToBody.find(entity);
		if (it == m_EntityToBody.end()) return;

		JPH::BodyInterface& bi = m_PhysicsSystem->GetBodyInterface();
		bi.SetLinearVelocity(it->second, JPH::Vec3(velocity.x, velocity.y, velocity.z));
	}

	glm::vec3 PhysicsWorld::GetLinearVelocity(EntityID entity) const
	{
		if (!m_Initialized) return glm::vec3(0.0f);
		auto it = m_EntityToBody.find(entity);
		if (it == m_EntityToBody.end()) return glm::vec3(0.0f);

		const JPH::BodyInterface& bi = m_PhysicsSystem->GetBodyInterface();
		JPH::Vec3 vel = bi.GetLinearVelocity(it->second);
		return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
	}

	bool PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction,
	                            float maxDistance, RaycastHit& outHit) const
	{
		if (!m_Initialized || maxDistance <= 0.0f)
			return false;

		// Jolt's RRayCast takes (origin, ray-vector) where the ray-vector is
		// direction * length. Anything beyond `length` is not reported as a hit.
		// Normalize the input first so the user can pass any vector.
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		float len = dir.Length();
		if (len < 1e-6f)
			return false;
		dir = dir / len;

		const JPH::RVec3 originVec(origin.x, origin.y, origin.z);
		const JPH::RRayCast ray(originVec, dir * maxDistance);

		// Closest-hit query — Jolt walks the broad/narrow phase and fills mFraction
		// with the smallest fraction along the ray that intersects a body.
		JPH::RayCastResult result;
		if (!m_PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, result))
			return false;

		// Convert hit fraction to a world-space point and a distance.
		// Distance = fraction * (ray length) since dir is unit-length.
		const JPH::RVec3 hitPoint = ray.GetPointOnRay(result.mFraction);
		const float distance = result.mFraction * maxDistance;

		// Map BodyID back to the owning ECS entity. Same lookup the contact
		// listener uses for collision dispatch.
		EntityID hitEntity = INVALID_ENTITY;
		auto it = m_BodyIndexToEntity.find(result.mBodyID.GetIndex());
		if (it != m_BodyIndexToEntity.end())
			hitEntity = it->second;

		// Surface normal at the hit point. Requires reading the body's shape,
		// which means locking the body for thread-safe access. We're on the
		// main thread between physics steps, so the lock will succeed instantly.
		glm::vec3 normal(0.0f);
		{
			JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), result.mBodyID);
			if (lock.Succeeded()) {
				const JPH::Body& body = lock.GetBody();
				JPH::Vec3 n = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint);
				normal = glm::vec3(n.GetX(), n.GetY(), n.GetZ());
			}
		}

		outHit.entity   = hitEntity;
		outHit.point    = glm::vec3((float)hitPoint.GetX(), (float)hitPoint.GetY(), (float)hitPoint.GetZ());
		outHit.normal   = normal;
		outHit.distance = distance;
		return true;
	}

} // namespace Orion
