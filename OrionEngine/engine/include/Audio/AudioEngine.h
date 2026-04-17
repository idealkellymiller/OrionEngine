#pragma once
#include "EngineCore.h"
#include "ECS/Scene.h"

#include <string>
#include <memory>

namespace Orion {

    // AudioEngine wraps miniaudio for runtime audio playback.
    //
    // Lifecycle (called by RuntimeLayer):
    //   Init()     -- start ma_engine, load clips for every AudioSourceComponent
    //   OnStart()  -- play any sources flagged playOnStart
    //   Update(dt) -- update listener pose, sweep finished one-shots
    //   Shutdown() -- destroy all sounds and the engine
    //
    // miniaudio types are hidden behind PIMPL so this header stays clean.

    class ORION_API AudioEngine {
    public:
        AudioEngine();
        ~AudioEngine();

        void Init(std::shared_ptr<Scene> scene, const std::string& assetsPath);
        void OnStart();
        void Update(float dt);
        void Shutdown();

        bool IsInitialized() const { return m_Initialized; }

        // Per-entity audio control (entity must have an AudioSourceComponent)
        void Play(EntityID entity);
        void Stop(EntityID entity);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void SetVolume(EntityID entity, float volume);
        void SetPitch(EntityID entity, float pitch);
        bool IsPlaying(EntityID entity) const;

        // Fire-and-forget: 3D positional sound at a world position
        void PlayOneShot(const std::string& clipPath, float x, float y, float z, float volume = 1.0f);

        // Fire-and-forget: non-positional (2D) sound
        void PlayOneShot2D(const std::string& clipPath, float volume = 1.0f);

        // Immediately stop all active sounds
        void StopAll();

    private:
        // Creates and registers an ma_sound for one entity.
        // Called during Init for each entity with a valid AudioSourceComponent.
        void CreateSoundForEntity(EntityID entity, const struct AudioSourceComponent& asc);

        // Builds an absolute path from a clip path that may be assets-relative or absolute.
        std::string ResolveClipPath(const std::string& clipPath) const;

    private:
        struct Impl;                           // defined in AudioEngine.cpp — holds ma_engine + maps
        std::unique_ptr<Impl> m_Impl;

        std::shared_ptr<Scene> m_Scene;
        std::string m_AssetsPath;
        bool m_Initialized = false;
    };

}
