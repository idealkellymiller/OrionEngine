// ============================================================
// Apollo Scripting Engine
// ============================================================
// Apollo is the gameplay scripting layer for OrionEngine.
// It embeds a Lua 5.4 VM (via sol2) and exposes engine systems
// to per-entity Lua scripts.
//
// Each entity with a ScriptComponent gets its own Lua environment.
// Scripts define two optional lifecycle callbacks:
//   OnStart()      -- called once when play mode begins
//   OnUpdate(dt)   -- called every frame with delta time in seconds
//
// Scripts can access:
//   Transform  -- get/set position, rotation, scale of this entity
//   Input      -- query keyboard and mouse state
//   Entity     -- find other entities, create/destroy entities
//   Time       -- deltaTime, elapsed time
//
// Scripts CANNOT access:
//   Rendering internals, shaders, OpenGL, file system, layers
//
// Usage:
//   #include <Scripting/Apollo.h>
// ============================================================

#pragma once
#include "EngineCore.h"
#include "ECS/Scene.h"

#include <string>
#include <unordered_map>
#include <memory>

// Forward-declare sol::state to avoid pulling sol2 into every header.
// Only ScriptEngine.cpp includes the full sol2 headers.
namespace sol { class state; }

namespace Orion {

    // Per-entity script instance data.
    // Holds the entity ID and a reference to its Lua environment (table),
    // which isolates its globals from other scripts.
    struct ScriptInstance {
        EntityID entity = INVALID_ENTITY;
        // The Lua environment index is stored internally by ScriptEngine.
        // This struct is here so we can track which entities have been initialized.
        bool started = false;
    };


    class ORION_API ScriptEngine {
    public:
        ScriptEngine();
        ~ScriptEngine();

        // ----- Lifecycle (called by RuntimeLayer) -----

        // Create the Lua VM, register all engine bindings (Transform, Input, etc.),
        // and load every ScriptComponent in the scene.
        void Init(std::shared_ptr<Scene> scene, const std::string& assetsPath);

        // Call OnStart() on all scripts that haven't started yet.
        void OnStart();

        // Call OnUpdate(dt) on all loaded scripts.
        void OnUpdate(float deltaTime);

        // Destroy the Lua VM and all script instances.
        void Shutdown();

        bool IsInitialized() const { return m_Lua != nullptr; }

    private:
        // ----- Binding registration -----
        // Each of these registers a Lua "module" table with functions.

        void RegisterTransformBindings();   // Transform.GetPosition(), .SetPosition(), etc.
        void RegisterInputBindings();       // Input.IsKeyDown(), .IsMouseButtonDown(), etc.
        void RegisterEntityBindings();      // Entity.FindByName(), .GetID(), etc.
        void RegisterTimeBindings();        // Time.deltaTime, Time.elapsed

        // ----- Script loading -----

        // Load a single .lua file into a sandboxed environment for the given entity.
        bool LoadScript(EntityID entity, const std::string& filePath);

    private:
        // The single Lua VM shared by all scripts.
        // Using a raw pointer + explicit new/delete to avoid pulling sol2 into the header.
        sol::state* m_Lua = nullptr;

        // Map from EntityID -> its Lua environment reference (registry index).
        // Each script gets its own environment table so globals don't collide.
        std::unordered_map<EntityID, int> m_ScriptEnvs;

        // Track which entities have had OnStart() called.
        std::unordered_map<EntityID, ScriptInstance> m_Instances;

        // Cached reference to the active scene (set in Init, used by bindings).
        std::shared_ptr<Scene> m_Scene;

        // Path prefix for resolving script file paths.
        std::string m_AssetsPath;

        // Frame timing (written by OnUpdate, read by Time bindings).
        float m_DeltaTime = 0.0f;
        float m_ElapsedTime = 0.0f;

        // The entity currently being executed (set before each script call).
        // Bindings like Transform.GetPosition() use this to know which entity to act on.
        EntityID m_CurrentEntity = INVALID_ENTITY;
    };

}
