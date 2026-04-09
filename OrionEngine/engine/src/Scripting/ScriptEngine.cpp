// ============================================================
// Apollo ScriptEngine — Implementation
// ============================================================
// This is the only file that includes the full sol2 / Lua headers.
// Everything else just includes Apollo.h (which forward-declares sol::state).

#include "Scripting/Apollo.h"
#include "Physics/PhysicsWorld.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"

// Full sol2 header — pulls in Lua and all sol2 machinery.
// SOL_ALL_SAFETIES_ON enables runtime type-checking in debug builds.
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
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

    void ScriptEngine::Init(std::shared_ptr<Scene> scene, const std::string& assetsPath, PhysicsWorld* physicsWorld)
    {
        m_Scene = scene;
        m_AssetsPath = assetsPath;
        m_PhysicsWorld = physicsWorld;

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

        // Create a ScriptInstance to track OnStart state.
        ScriptInstance inst;
        inst.entity = entity;
        inst.started = false;
        m_Instances[entity] = inst;

        return true;
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

        // Input.IsKeyDown(keyName) -> bool
        // Accepts GLFW key names as strings: "W", "A", "S", "D", "Space", etc.
        // Also accepts raw GLFW key codes as integers.
        input["IsKeyDown"] = [](const std::string& keyName) -> bool {
            GLFWwindow* window = static_cast<GLFWwindow*>(
                Application::Get().GetWindow().GetNativeWindow());
            if (!window) return false;

            // Map common key names to GLFW key codes.
            int keyCode = -1;

            if (keyName.length() == 1) {
                // Single character: A-Z, 0-9
                char c = keyName[0];
                if (c >= 'A' && c <= 'Z')       keyCode = GLFW_KEY_A + (c - 'A');
                else if (c >= 'a' && c <= 'z')   keyCode = GLFW_KEY_A + (c - 'a');
                else if (c >= '0' && c <= '9')   keyCode = GLFW_KEY_0 + (c - '0');
            }
            else {
                // Named keys
                if (keyName == "Space")          keyCode = GLFW_KEY_SPACE;
                else if (keyName == "Enter")     keyCode = GLFW_KEY_ENTER;
                else if (keyName == "Escape")    keyCode = GLFW_KEY_ESCAPE;
                else if (keyName == "Tab")       keyCode = GLFW_KEY_TAB;
                else if (keyName == "LeftShift")   keyCode = GLFW_KEY_LEFT_SHIFT;
                else if (keyName == "RightShift")  keyCode = GLFW_KEY_RIGHT_SHIFT;
                else if (keyName == "LeftCtrl")    keyCode = GLFW_KEY_LEFT_CONTROL;
                else if (keyName == "RightCtrl")   keyCode = GLFW_KEY_RIGHT_CONTROL;
                else if (keyName == "Up")        keyCode = GLFW_KEY_UP;
                else if (keyName == "Down")      keyCode = GLFW_KEY_DOWN;
                else if (keyName == "Left")      keyCode = GLFW_KEY_LEFT;
                else if (keyName == "Right")     keyCode = GLFW_KEY_RIGHT;
            }

            if (keyCode < 0) return false;
            return glfwGetKey(window, keyCode) == GLFW_PRESS;
        };

        // Input.IsMouseButtonDown(button) -> bool
        // button: 0 = left, 1 = right, 2 = middle
        input["IsMouseButtonDown"] = [](int button) -> bool {
            GLFWwindow* window = static_cast<GLFWwindow*>(
                Application::Get().GetWindow().GetNativeWindow());
            if (!window) return false;
            return glfwGetMouseButton(window, button) == GLFW_PRESS;
        };

        // Input.GetMousePosition() -> x, y
        input["GetMousePosition"] = []() -> std::tuple<float, float> {
            GLFWwindow* window = static_cast<GLFWwindow*>(
                Application::Get().GetWindow().GetNativeWindow());
            if (!window) return { 0, 0 };
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            return { (float)x, (float)y };
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
    }

}
