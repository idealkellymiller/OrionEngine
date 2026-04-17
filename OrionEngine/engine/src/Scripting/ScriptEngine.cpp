// ============================================================
// Apollo ScriptEngine — Implementation
// ============================================================
// This is the only file that includes the full sol2 / Lua headers.
// Everything else just includes Apollo.h (which forward-declares sol::state).

#include "Scripting/Apollo.h"
#include "Physics/PhysicsWorld.h"
#include "Audio/AudioEngine.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"
#include "Application.h"
#include "Core/Input.h"

#include <glm/gtc/matrix_transform.hpp>

// Full sol2 header — pulls in Lua and all sol2 machinery.
// SOL_ALL_SAFETIES_ON enables runtime type-checking in debug builds.
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <system_error>
#include <GLFW/glfw3.h>
#include "Application.h"


namespace Orion {

    // ================================================================
    // Constructor / Destructor
    // ================================================================

    ScriptEngine::ScriptEngine()
    {
    }

    ScriptEngine::~ScriptEngine()
    {
        // Safety net — Shutdown should already have been called by RuntimeLayer.
        if (m_Lua)
            Shutdown();
    }

    // ================================================================
    // Lifecycle
    // ================================================================

    void ScriptEngine::Init(std::shared_ptr<Scene> scene, const std::string& assetsPath,
                            PhysicsWorld* physicsWorld, AudioEngine* audioEngine)
    {
        m_Scene = scene;
        m_AssetsPath = assetsPath;
        m_PhysicsWorld = physicsWorld;
        m_AudioEngine  = audioEngine;

        // Create the Lua virtual machine and open the standard libraries
        // (string, table, math, etc. — no io/os for sandboxing).
        m_Lua = new sol::state();
        m_Lua->open_libraries(
            sol::lib::base,
            sol::lib::string,
            sol::lib::table,
            sol::lib::math
        );

        // Register all engine bindings so scripts can call Transform.*, Input.*, etc.
        RegisterTransformBindings();
        RegisterInputBindings();
        RegisterEntityBindings();
        RegisterTimeBindings();
        RegisterPhysicsBindings();
        RegisterLogBindings();
        RegisterSceneBindings();
        RegisterAudioBindings();

        // Walk every entity that has a ScriptComponent and load its script file.
        for (EntityID entity : scene->GetEntities()) {
            ScriptComponent* sc = scene->GetScriptComponent(entity);
            if (!sc || sc->scriptPath.empty())
                continue;

            // Build the full file path from assets folder + script relative path.
            std::string fullPath = m_AssetsPath + sc->scriptPath;

            if (LoadScript(entity, fullPath)) {
                std::cout << "[Apollo] Loaded script for entity " << entity
                          << ": " << sc->scriptPath << "\n";
            } else {
                std::cout << "[Apollo] FAILED to load script for entity " << entity
                          << ": " << fullPath << "\n";
            }
        }

        std::cout << "[Apollo] Initialized with " << m_Instances.size() << " script(s).\n";
    }

    void ScriptEngine::OnStart()
    {
        // Call OnStart() once for each script that hasn't started yet.
        for (auto& [entity, instance] : m_Instances) {
            if (instance.started)
                continue;

            instance.started = true;

            // Find this entity's Lua environment.
            auto envIt = m_ScriptEnvs.find(entity);
            if (envIt == m_ScriptEnvs.end())
                continue;

            // Retrieve the environment table from the Lua registry.
            sol::environment env(m_Lua->lua_state(), sol::ref_index(envIt->second));

            // Set "this entity" so bindings know who they're acting on.
            m_CurrentEntity = entity;

            // Look for an OnStart function in this script's environment.
            sol::optional<sol::function> onStart = env["OnStart"];
            if (onStart) {
                // Call it inside a protected call so a script error
                // doesn't crash the entire engine.
                sol::protected_function_result result = (*onStart)();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cout << "[Apollo] OnStart error (entity " << entity << "): "
                              << err.what() << "\n";
                }
            }
        }

        m_CurrentEntity = INVALID_ENTITY;
    }

    void ScriptEngine::OnUpdate(float deltaTime)
    {
        m_DeltaTime = deltaTime;
        m_ElapsedTime += deltaTime;

        // Call OnUpdate(dt) on every loaded script.
        for (auto& [entity, instance] : m_Instances) {
            auto envIt = m_ScriptEnvs.find(entity);
            if (envIt == m_ScriptEnvs.end())
                continue;

            sol::environment env(m_Lua->lua_state(), sol::ref_index(envIt->second));

            // Set "this entity" for the duration of the call.
            m_CurrentEntity = entity;

            sol::optional<sol::function> onUpdate = env["OnUpdate"];
            if (onUpdate) {
                sol::protected_function_result result = (*onUpdate)(deltaTime);
                if (!result.valid()) {
                    sol::error err = result;
                    std::cout << "[Apollo] OnUpdate error (entity " << entity << "): "
                              << err.what() << "\n";
                }
            }
        }

        m_CurrentEntity = INVALID_ENTITY;
    }

    void ScriptEngine::Shutdown()
    {
        // Release all Lua registry references for script environments.
        if (m_Lua) {
            for (auto& [entity, refIdx] : m_ScriptEnvs)
                luaL_unref(m_Lua->lua_state(), LUA_REGISTRYINDEX, refIdx);
        }

        m_ScriptEnvs.clear();
        m_Instances.clear();
        m_Scene.reset();
        m_DeltaTime = 0.0f;
        m_ElapsedTime = 0.0f;
        m_CurrentEntity = INVALID_ENTITY;

        // Destroy the Lua VM — frees all Lua memory.
        delete m_Lua;
        m_Lua = nullptr;

        std::cout << "[Apollo] Shutdown.\n";
    }

    // ================================================================
    // Script loading
    // ================================================================

    bool ScriptEngine::LoadScript(EntityID entity, const std::string& filePath)
    {
        // Read the .lua file into a string.
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        std::stringstream ss;
        ss << file.rdbuf();
        std::string source = ss.str();

        // Create a sandboxed environment for this script.
        // The environment inherits from the global table (so scripts can use
        // print, math.*, etc.) but any new globals the script creates stay
        // isolated in its own table.
        sol::environment env(*m_Lua, sol::create, m_Lua->globals());

        // Execute the script source inside this environment.
        // This defines the script's functions (OnStart, OnUpdate, etc.)
        // inside `env` rather than in the global table.
        auto result = m_Lua->safe_script(source, env, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            std::cout << "[Apollo] Script load error (" << filePath << "): "
                      << err.what() << "\n";
            return false;
        }

        // Store the environment in the Lua registry so it persists.
        // lua_ref gives us an integer key we can use to retrieve it later.
        env.push();
        int refIdx = luaL_ref(m_Lua->lua_state(), LUA_REGISTRYINDEX);
        m_ScriptEnvs[entity] = refIdx;

        // Create a ScriptInstance to track OnStart state and hot-reload info.
        ScriptInstance inst;
        inst.entity = entity;
        inst.started = false;
        inst.filePath = filePath;
        std::error_code ec;
        inst.lastWriteTime = std::filesystem::last_write_time(filePath, ec);
        // If stat fails (shouldn't — we just read the file), leave default-constructed.
        // A mismatch on first CheckHotReload() will harmlessly trigger one reload.
        m_Instances[entity] = inst;

        return true;
    }

    bool ScriptEngine::ReloadScript(EntityID entity)
    {
        auto instIt = m_Instances.find(entity);
        if (instIt == m_Instances.end())
            return false;

        const std::string filePath = instIt->second.filePath;
        if (filePath.empty())
            return false;

        // Read the updated source from disk.
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;
        std::stringstream ss;
        ss << file.rdbuf();
        std::string source = ss.str();

        // Drop the old sandbox environment so its OnStart/OnUpdate closures
        // (and any globals they defined) are garbage-collected.
        auto envIt = m_ScriptEnvs.find(entity);
        if (envIt != m_ScriptEnvs.end()) {
            luaL_unref(m_Lua->lua_state(), LUA_REGISTRYINDEX, envIt->second);
            m_ScriptEnvs.erase(envIt);
        }

        // Build a fresh sandbox and run the new source inside it.
        sol::environment env(*m_Lua, sol::create, m_Lua->globals());
        auto result = m_Lua->safe_script(source, env, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            std::cout << "[Apollo] Hot-reload error (" << filePath << "): "
                      << err.what() << "\n";
            // The script is now in a broken state: no environment is registered, so
            // OnUpdate calls for this entity will silently no-op until a subsequent
            // edit parses successfully and we retry the reload.
            return false;
        }

        env.push();
        int refIdx = luaL_ref(m_Lua->lua_state(), LUA_REGISTRYINDEX);
        m_ScriptEnvs[entity] = refIdx;

        // Update mtime and reset started so OnStart() fires again on the next tick.
        std::error_code ec;
        instIt->second.lastWriteTime = std::filesystem::last_write_time(filePath, ec);
        instIt->second.started = false;

        return true;
    }

    void ScriptEngine::CheckHotReload()
    {
        if (!m_Lua)
            return;

        bool anyReloaded = false;

        // Snapshot entity IDs first — ReloadScript mutates m_Instances values
        // (not keys), but iterating during reload is fine regardless.
        for (auto& [entity, instance] : m_Instances) {
            if (instance.filePath.empty())
                continue;

            std::error_code ec;
            auto mtime = std::filesystem::last_write_time(instance.filePath, ec);
            if (ec)
                continue;  // file missing / unreadable — skip, try again next frame

            if (mtime != instance.lastWriteTime) {
                std::cout << "[Apollo] Hot-reloading script for entity " << entity
                          << ": " << instance.filePath << "\n";
                if (ReloadScript(entity))
                    anyReloaded = true;
                else {
                    // Prevent retrying every frame on a broken file by advancing the
                    // stored mtime even though the reload failed. The user's next save
                    // will bump mtime again and we'll try again.
                    instance.lastWriteTime = mtime;
                }
            }
        }

        if (anyReloaded) {
            // Re-run OnStart for any scripts whose started flag was just reset.
            // OnStart() already iterates only entities with started == false.
            OnStart();
        }
    }

    // ================================================================
    // Bindings: Transform
    // ================================================================
    // Exposes Transform.GetPosition(), Transform.SetPosition(x,y,z), etc.
    // These always act on m_CurrentEntity — the entity whose script is running.

    void ScriptEngine::RegisterTransformBindings()
    {
        sol::state& lua = *m_Lua;

        // Create the "Transform" table in Lua's global scope.
        sol::table transform = lua.create_named_table("Transform");

        // Transform.GetPosition() -> returns x, y, z as three values
        transform["GetPosition"] = [this]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return { 0, 0, 0 };

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 0, 0, 0 };

            return { tc->position.x, tc->position.y, tc->position.z };
        };

        // Transform.SetPosition(x, y, z)
        transform["SetPosition"] = [this](float x, float y, float z) {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return;

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return;

            tc->position = { x, y, z };
        };

        // Transform.GetRotation() -> returns x, y, z (Euler radians)
        transform["GetRotation"] = [this]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return { 0, 0, 0 };

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 0, 0, 0 };

            return { tc->rotation.x, tc->rotation.y, tc->rotation.z };
        };

        // Transform.SetRotation(x, y, z)
        transform["SetRotation"] = [this](float x, float y, float z) {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return;

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return;

            tc->rotation = { x, y, z };
        };

        // Transform.GetScale() -> returns x, y, z
        transform["GetScale"] = [this]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return { 1, 1, 1 };

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 1, 1, 1 };

            return { tc->scale.x, tc->scale.y, tc->scale.z };
        };

        // Transform.SetScale(x, y, z)
        transform["SetScale"] = [this](float x, float y, float z) {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return;

            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return;

            tc->scale = { x, y, z };
        };

        // Helper: compose the rotation matrix using the same X->Y->Z order
        // that the renderer (Scene::BuildLocalTransform, ApplyRuntimeCamera) uses.
        // Capturing `this` isn't useful here — make it a plain local lambda.
        auto buildRot = [](const glm::vec3& euler) -> glm::mat4 {
            glm::mat4 rot(1.0f);
            if (euler.x != 0.0f) rot = glm::rotate(rot, euler.x, glm::vec3(1, 0, 0));
            if (euler.y != 0.0f) rot = glm::rotate(rot, euler.y, glm::vec3(0, 1, 0));
            if (euler.z != 0.0f) rot = glm::rotate(rot, euler.z, glm::vec3(0, 0, 1));
            return rot;
        };

        // Transform.Translate(x, y, z) — additive offset in local space (same axes as position).
        transform["Translate"] = [this](float x, float y, float z) {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return;
            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (tc) tc->position += glm::vec3(x, y, z);
        };

        // Transform.Rotate(x, y, z) — additive Euler rotation (radians).
        transform["Rotate"] = [this](float x, float y, float z) {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return;
            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (tc) tc->rotation += glm::vec3(x, y, z);
        };

        // Transform.GetForward() -> x, y, z
        // The "look direction" of the entity: (0,0,-1) rotated by the entity's Euler angles.
        // This matches the camera convention used by ApplyRuntimeCamera.
        transform["GetForward"] = [this, buildRot]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return { 0, 0, -1 };
            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 0, 0, -1 };
            glm::vec3 fwd = glm::vec3(buildRot(tc->rotation) * glm::vec4(0, 0, -1, 0));
            return { fwd.x, fwd.y, fwd.z };
        };

        // Transform.GetRight() -> x, y, z
        transform["GetRight"] = [this, buildRot]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return { 1, 0, 0 };
            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 1, 0, 0 };
            glm::vec3 right = glm::vec3(buildRot(tc->rotation) * glm::vec4(1, 0, 0, 0));
            return { right.x, right.y, right.z };
        };

        // Transform.GetUp() -> x, y, z
        transform["GetUp"] = [this, buildRot]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return { 0, 1, 0 };
            TransformComponent* tc = m_Scene->GetTransformComponent(m_CurrentEntity);
            if (!tc) return { 0, 1, 0 };
            glm::vec3 up = glm::vec3(buildRot(tc->rotation) * glm::vec4(0, 1, 0, 0));
            return { up.x, up.y, up.z };
        };

        // Transform.GetWorldPosition() -> x, y, z
        // Walks the parent chain (Scene already implements this). For entities without
        // a parent this returns the same thing as GetPosition.
        transform["GetWorldPosition"] = [this]() -> std::tuple<float, float, float> {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY) return { 0, 0, 0 };
            glm::mat4 world = m_Scene->GetWorldTransform(m_CurrentEntity);
            glm::vec3 pos = glm::vec3(world[3]);  // translation column
            return { pos.x, pos.y, pos.z };
        };
    }

    // ================================================================
    // Bindings: Input
    // ================================================================
    // Exposes Input.IsKeyDown("W"), Input.IsMouseButtonDown(0), etc.
    // Queries GLFW key/mouse state directly (same approach as EditorLayer).

    void ScriptEngine::RegisterInputBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table input = lua.create_named_table("Input");

        // All key-based queries route through Input::KeyNameToCode for a single
        // canonical set of accepted names: "A"-"Z", "0"-"9", Space, Enter, Escape,
        // Tab, LeftShift/RightShift, LeftCtrl/RightCtrl, Up/Down/Left/Right.
        //
        // Each of the three below uses the edge-detection state maintained by
        // Input::NewFrame(), so "pressed"/"released" fire for exactly one frame.

        // Input.IsKeyDown("W") -> true while the key is held
        input["IsKeyDown"] = [](const std::string& name) -> bool {
            int code = Input::KeyNameToCode(name);
            return code >= 0 && Input::IsKeyDown(code);
        };

        // Input.IsKeyPressed("E") -> true only on the frame the key goes down
        input["IsKeyPressed"] = [](const std::string& name) -> bool {
            int code = Input::KeyNameToCode(name);
            return code >= 0 && Input::IsKeyPressed(code);
        };

        // Input.IsKeyReleased("E") -> true only on the frame the key goes up
        input["IsKeyReleased"] = [](const std::string& name) -> bool {
            int code = Input::KeyNameToCode(name);
            return code >= 0 && Input::IsKeyReleased(code);
        };

        // Mouse button queries. button: 0 = left, 1 = right, 2 = middle.
        input["IsMouseButtonDown"]     = [](int b) -> bool { return Input::IsMouseButtonDown(b); };
        input["IsMouseButtonPressed"]  = [](int b) -> bool { return Input::IsMouseButtonPressed(b); };
        input["IsMouseButtonReleased"] = [](int b) -> bool { return Input::IsMouseButtonReleased(b); };

        // Input.GetMousePosition() -> x, y (in window pixels)
        input["GetMousePosition"] = []() -> std::tuple<float, float> {
            glm::vec2 p = Input::GetMousePosition();
            return { p.x, p.y };
        };

        // Input.GetMouseDelta() -> dx, dy (pixels moved since last frame)
        input["GetMouseDelta"] = []() -> std::tuple<float, float> {
            glm::vec2 d = Input::GetMouseDelta();
            return { d.x, d.y };
        };

        // Input.GetScrollDelta() -> scalar vertical scroll amount this frame.
        input["GetScrollDelta"] = []() -> float { return Input::GetScrollDelta(); };

        // Input.GetAxis("Horizontal") -> -1..1 from A/D or arrow keys.
        // Input.GetAxis("Vertical")   -> -1..1 from W/S or arrow keys.
        input["GetAxis"] = [](const std::string& axisName) -> float {
            return Input::GetAxis(axisName);
        };
    }

    // ================================================================
    // Bindings: Entity
    // ================================================================
    // Exposes Entity.GetID(), Entity.FindByName("name"), etc.
    // Allows scripts to query and interact with other entities.

    void ScriptEngine::RegisterEntityBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table entity = lua.create_named_table("Entity");

        // Entity.GetID() -> the ID of the entity this script is attached to
        entity["GetID"] = [this]() -> EntityID {
            return m_CurrentEntity;
        };

        // Entity.GetName() -> the name of this entity (from EntityDataComponent)
        entity["GetName"] = [this]() -> std::string {
            if (!m_Scene || m_CurrentEntity == INVALID_ENTITY)
                return "";

            EntityDataComponent* edc = m_Scene->GetEntityDataComponent(m_CurrentEntity);
            return edc ? edc->name : "";
        };

        // Entity.FindByName("Main Camera") -> returns EntityID or 0 (INVALID_ENTITY)
        // Searches all entities in the scene for a matching name.
        entity["FindByName"] = [this](const std::string& name) -> EntityID {
            if (!m_Scene) return INVALID_ENTITY;

            for (EntityID e : m_Scene->GetEntities()) {
                EntityDataComponent* edc = m_Scene->GetEntityDataComponent(e);
                if (edc && edc->name == name)
                    return e;
            }
            return INVALID_ENTITY;
        };

        // Entity.GetPosition(entityID) -> x, y, z
        // Read another entity's position (not just "this" entity).
        entity["GetPosition"] = [this](EntityID id) -> std::tuple<float, float, float> {
            if (!m_Scene) return { 0, 0, 0 };
            TransformComponent* tc = m_Scene->GetTransformComponent(id);
            if (!tc) return { 0, 0, 0 };
            return { tc->position.x, tc->position.y, tc->position.z };
        };

        // Entity.SetPosition(entityID, x, y, z)
        // Write another entity's position.
        entity["SetPosition"] = [this](EntityID id, float x, float y, float z) {
            if (!m_Scene) return;
            TransformComponent* tc = m_Scene->GetTransformComponent(id);
            if (!tc) return;
            tc->position = { x, y, z };
        };
    }

    // ================================================================
    // Bindings: Time
    // ================================================================
    // Exposes Time.deltaTime and Time.elapsed as readable values.
    // Updated each frame before OnUpdate is called.

    void ScriptEngine::RegisterTimeBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table time = lua.create_named_table("Time");

        // Time.deltaTime -> seconds since last frame
        // Uses a lambda that reads from ScriptEngine's cached value
        // so it's always up-to-date each frame.
        time["deltaTime"] = [this]() -> float { return m_DeltaTime; };

        // Time.elapsed -> total seconds since play mode started
        time["elapsed"] = [this]() -> float { return m_ElapsedTime; };
    }

    // ================================================================
    // Collision dispatch
    // ================================================================
    // Called by PhysicsWorld's ContactListener. Invokes OnCollision(otherID, isTrigger)
    // on both entities' scripts (if they have one).

    void ScriptEngine::OnCollision(EntityID entityA, EntityID entityB, bool isTrigger)
    {
        if (!m_Lua) return;

        // Helper: call OnCollision on one entity's script
        auto dispatch = [&](EntityID self, EntityID other) {
            auto envIt = m_ScriptEnvs.find(self);
            if (envIt == m_ScriptEnvs.end()) return;

            sol::environment env(m_Lua->lua_state(), sol::ref_index(envIt->second));
            sol::optional<sol::function> onCollision = env["OnCollision"];
            if (!onCollision) return;

            m_CurrentEntity = self;
            sol::protected_function_result result = (*onCollision)((int)other, isTrigger);
            if (!result.valid()) {
                sol::error err = result;
                std::cout << "[Apollo] OnCollision error (entity " << self << "): "
                          << err.what() << "\n";
            }
        };

        dispatch(entityA, entityB);
        dispatch(entityB, entityA);

        m_CurrentEntity = INVALID_ENTITY;
    }

    // ================================================================
    // Bindings: Physics
    // ================================================================
    // Exposes Physics.AddForce(x,y,z), Physics.AddImpulse(x,y,z),
    // Physics.SetVelocity(x,y,z), Physics.GetVelocity() -> x,y,z,
    // Physics.AddTorque(x,y,z).
    // All act on the current entity (the one whose script is running).
    // Also supports targeting other entities by ID.

    void ScriptEngine::RegisterPhysicsBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table physics = lua.create_named_table("Physics");

        // Physics.AddForce(x, y, z) — continuous force (applied over time, use in OnUpdate)
        physics["AddForce"] = [this](float x, float y, float z) {
            if (!m_PhysicsWorld || m_CurrentEntity == INVALID_ENTITY) return;
            m_PhysicsWorld->AddForce(m_CurrentEntity, { x, y, z });
        };

        // Physics.AddForceToEntity(entityID, x, y, z) — apply force to a specific entity
        physics["AddForceToEntity"] = [this](EntityID id, float x, float y, float z) {
            if (!m_PhysicsWorld) return;
            m_PhysicsWorld->AddForce(id, { x, y, z });
        };

        // Physics.AddImpulse(x, y, z) — instantaneous velocity change (e.g. jump)
        physics["AddImpulse"] = [this](float x, float y, float z) {
            if (!m_PhysicsWorld || m_CurrentEntity == INVALID_ENTITY) return;
            m_PhysicsWorld->AddImpulse(m_CurrentEntity, { x, y, z });
        };

        // Physics.AddImpulseToEntity(entityID, x, y, z)
        physics["AddImpulseToEntity"] = [this](EntityID id, float x, float y, float z) {
            if (!m_PhysicsWorld) return;
            m_PhysicsWorld->AddImpulse(id, { x, y, z });
        };

        // Physics.AddTorque(x, y, z) — rotational force
        physics["AddTorque"] = [this](float x, float y, float z) {
            if (!m_PhysicsWorld || m_CurrentEntity == INVALID_ENTITY) return;
            m_PhysicsWorld->AddTorque(m_CurrentEntity, { x, y, z });
        };

        // Physics.SetVelocity(x, y, z) — override linear velocity directly
        physics["SetVelocity"] = [this](float x, float y, float z) {
            if (!m_PhysicsWorld || m_CurrentEntity == INVALID_ENTITY) return;
            m_PhysicsWorld->SetLinearVelocity(m_CurrentEntity, { x, y, z });
        };

        // Physics.GetVelocity() -> x, y, z
        physics["GetVelocity"] = [this]() -> std::tuple<float, float, float> {
            if (!m_PhysicsWorld || m_CurrentEntity == INVALID_ENTITY)
                return { 0, 0, 0 };
            glm::vec3 vel = m_PhysicsWorld->GetLinearVelocity(m_CurrentEntity);
            return { vel.x, vel.y, vel.z };
        };

        // Physics.Raycast(ox, oy, oz, dx, dy, dz, maxDistance) -> table or nil
        //
        // On hit, returns a table:
        //   hit.entity       (number)  — EntityID of the body hit
        //   hit.distance     (number)  — distance from origin along the ray
        //   hit.point.x/y/z  (numbers) — world-space hit position
        //   hit.normal.x/y/z (numbers) — surface normal at the hit point
        //
        // On miss (or if physics isn't initialized / direction is zero), returns nil.
        // Direction need not be unit-length; it's normalized internally.
        //
        // Lua usage:
        //   local hit = Physics.Raycast(ox, oy, oz, dx, dy, dz, 100)
        //   if hit then print("Hit " .. hit.entity .. " at dist " .. hit.distance) end
        physics["Raycast"] = [this](float ox, float oy, float oz,
                                     float dx, float dy, float dz,
                                     float maxDist) -> sol::optional<sol::table>
        {
            if (!m_PhysicsWorld || !m_Lua)
                return sol::nullopt;

            RaycastHit hit;
            if (!m_PhysicsWorld->Raycast({ ox, oy, oz }, { dx, dy, dz }, maxDist, hit))
                return sol::nullopt;

            // Build the result table. Using nested tables for point/normal so Lua
            // callers get familiar `hit.point.x` syntax instead of flat numerics.
            sol::table result = m_Lua->create_table();
            result["entity"]   = hit.entity;
            result["distance"] = hit.distance;

            sol::table point = m_Lua->create_table();
            point["x"] = hit.point.x;
            point["y"] = hit.point.y;
            point["z"] = hit.point.z;
            result["point"] = point;

            sol::table normal = m_Lua->create_table();
            normal["x"] = hit.normal.x;
            normal["y"] = hit.normal.y;
            normal["z"] = hit.normal.z;
            result["normal"] = normal;

            return result;
        };
    }

    // ================================================================
    // Bindings: Log
    // ================================================================
    // Wraps std::cout / std::cerr with a [Script] prefix and severity label.
    // Lua's built-in print() still works; these exist so scripts can distinguish
    // info/warn/error output and so we have a central place to later hook into
    // the engine's Console panel (Core/Console) for in-editor log viewing.

    void ScriptEngine::RegisterLogBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table log = lua.create_named_table("Log");

        log["Info"] = [this](const std::string& msg) {
            std::cout << "[Script] " << msg << "\n";
        };

        log["Warn"] = [this](const std::string& msg) {
            std::cout << "[Script WARN] " << msg << "\n";
        };

        log["Error"] = [this](const std::string& msg) {
            std::cerr << "[Script ERROR] " << msg << "\n";
        };
    }

    // ================================================================
    // Bindings: Scene / Application
    // ================================================================
    // Scene.Load("levels/next.scene")  -- queued; RuntimeLayer performs the swap
    // Scene.Reload()                   -- re-load the currently active scene
    // Application.Quit()               -- graceful shutdown at end of frame
    // Application.SetTimeScale(f)      -- 0 = pause, 0.5 = slow-mo, 2.0 = fast-forward
    // Application.GetTimeScale()       -- current scale

    void ScriptEngine::RegisterSceneBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table scene = lua.create_named_table("Scene");

        // Scene.Load(path). The swap itself is deferred — we can't tear down the
        // scene from inside a script that's currently executing against it.
        // RuntimeLayer drains m_PendingSceneLoad at the end of OnUpdate().
        scene["Load"] = [this](const std::string& path) {
            m_PendingSceneLoad = path;
        };

        // Scene.Reload() — sentinel value, handled by RuntimeLayer.
        scene["Reload"] = [this]() {
            m_PendingSceneLoad = "__RELOAD__";
        };

        sol::table app = lua.create_named_table("Application");

        // Application.Quit() — closes the window at the end of the current frame.
        app["Quit"] = []() {
            Application::Get().Close();
        };

        // Uniform time-scale multiplier. RuntimeLayer applies this to dt before
        // invoking scripts and physics, so both stay in sync.
        app["SetTimeScale"] = [](float scale) {
            Application::SetTimeScale(scale);
        };

        app["GetTimeScale"] = []() -> float {
            return Application::GetTimeScale();
        };
    }

    // ================================================================
    // Bindings: Audio
    // ================================================================
    // Exposes Audio.Play(), Audio.Stop(), Audio.Pause(), Audio.Resume(),
    // Audio.SetVolume(), Audio.SetPitch(), Audio.IsPlaying(),
    // Audio.PlayOneShot(), Audio.PlayOneShot2D().
    //
    // All entity-targeted functions accept an optional EntityID so scripts
    // can control other entities' audio, not just the entity running the script.

    void ScriptEngine::RegisterAudioBindings()
    {
        sol::state& lua = *m_Lua;

        sol::table audio = lua.create_named_table("Audio");

        // Audio.Play([entityID])
        audio["Play"] = sol::overload(
            [this]() {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->Play(m_CurrentEntity);
            },
            [this](EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->Play(id);
            }
        );

        // Audio.Stop([entityID])
        audio["Stop"] = sol::overload(
            [this]() {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->Stop(m_CurrentEntity);
            },
            [this](EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->Stop(id);
            }
        );

        // Audio.Pause([entityID])
        audio["Pause"] = sol::overload(
            [this]() {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->Pause(m_CurrentEntity);
            },
            [this](EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->Pause(id);
            }
        );

        // Audio.Resume([entityID])
        audio["Resume"] = sol::overload(
            [this]() {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->Resume(m_CurrentEntity);
            },
            [this](EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->Resume(id);
            }
        );

        // Audio.IsPlaying([entityID]) -> bool
        audio["IsPlaying"] = sol::overload(
            [this]() -> bool {
                if (!m_AudioEngine || m_CurrentEntity == INVALID_ENTITY) return false;
                return m_AudioEngine->IsPlaying(m_CurrentEntity);
            },
            [this](EntityID id) -> bool {
                if (!m_AudioEngine) return false;
                return m_AudioEngine->IsPlaying(id);
            }
        );

        // Audio.SetVolume(volume [, entityID])
        audio["SetVolume"] = sol::overload(
            [this](float volume) {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->SetVolume(m_CurrentEntity, volume);
            },
            [this](float volume, EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->SetVolume(id, volume);
            }
        );

        // Audio.SetPitch(pitch [, entityID])
        audio["SetPitch"] = sol::overload(
            [this](float pitch) {
                if (m_AudioEngine && m_CurrentEntity != INVALID_ENTITY)
                    m_AudioEngine->SetPitch(m_CurrentEntity, pitch);
            },
            [this](float pitch, EntityID id) {
                if (m_AudioEngine)
                    m_AudioEngine->SetPitch(id, pitch);
            }
        );

        // Audio.PlayOneShot(clipPath [, volume [, x, y, z]])
        // 3D one-shot at optional world position (defaults to origin)
        audio["PlayOneShot"] = sol::overload(
            [this](const std::string& path) {
                if (m_AudioEngine) m_AudioEngine->PlayOneShot(path, 0, 0, 0, 1.0f);
            },
            [this](const std::string& path, float volume) {
                if (m_AudioEngine) m_AudioEngine->PlayOneShot(path, 0, 0, 0, volume);
            },
            [this](const std::string& path, float volume, float x, float y, float z) {
                if (m_AudioEngine) m_AudioEngine->PlayOneShot(path, x, y, z, volume);
            }
        );

        // Audio.PlayOneShot2D(clipPath [, volume])
        audio["PlayOneShot2D"] = sol::overload(
            [this](const std::string& path) {
                if (m_AudioEngine) m_AudioEngine->PlayOneShot2D(path, 1.0f);
            },
            [this](const std::string& path, float volume) {
                if (m_AudioEngine) m_AudioEngine->PlayOneShot2D(path, volume);
            }
        );

        // Audio.StopAll()
        audio["StopAll"] = [this]() {
            if (m_AudioEngine) m_AudioEngine->StopAll();
        };
    }

}
