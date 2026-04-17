// AudioEngine.cpp — the ONLY translation unit that defines miniaudio.
// Including the implementation here keeps ma_* symbols out of all other .cpp files.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "Audio/AudioEngine.h"
#include "ECS/Scene.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Orion {

    // ================================================================
    // PIMPL — all miniaudio state lives here
    // ================================================================

    struct AudioEngine::Impl {
        ma_engine engine;

        // One ma_sound per AudioSource entity (loaded at Init time)
        std::unordered_map<EntityID, ma_sound*> sounds;

        // Fire-and-forget sounds: allocated on Play, freed when ma_sound_at_end() is true
        std::vector<ma_sound*> oneShots;
    };


    // ================================================================
    // Constructor / Destructor
    // ================================================================

    AudioEngine::AudioEngine()
        : m_Impl(std::make_unique<Impl>())
    {
    }

    AudioEngine::~AudioEngine()
    {
        if (m_Initialized)
            Shutdown();
    }


    // ================================================================
    // Lifecycle
    // ================================================================

    void AudioEngine::Init(std::shared_ptr<Scene> scene, const std::string& assetsPath)
    {
        m_Scene     = scene;
        m_AssetsPath = assetsPath;

        // Initialise the miniaudio engine (uses default device / sample rate)
        ma_result result = ma_engine_init(nullptr, &m_Impl->engine);
        if (result != MA_SUCCESS) {
            std::cout << "[AudioEngine] Failed to initialise ma_engine (error " << result << "). "
                         "Audio will be silent.\n";
            return;
        }

        // Load an ma_sound for every entity that has an AudioSourceComponent
        for (EntityID entity : scene->GetEntities()) {
            AudioSourceComponent* asc = scene->GetAudioSourceComponent(entity);
            if (!asc || asc->clipPath.empty())
                continue;
            CreateSoundForEntity(entity, *asc);
        }

        m_Initialized = true;
        std::cout << "[AudioEngine] Initialised with " << m_Impl->sounds.size() << " source(s).\n";
    }

    void AudioEngine::OnStart()
    {
        if (!m_Initialized || !m_Scene)
            return;

        for (EntityID entity : m_Scene->GetEntities()) {
            AudioSourceComponent* asc = m_Scene->GetAudioSourceComponent(entity);
            if (asc && asc->playOnStart)
                Play(entity);
        }
    }

    void AudioEngine::Update(float /*dt*/)
    {
        if (!m_Initialized || !m_Scene)
            return;

        // --- Update listener pose from the AudioListenerComponent entity ---
        for (EntityID entity : m_Scene->GetEntities()) {
            if (!m_Scene->HasAudioListenerComponent(entity))
                continue;

            TransformComponent* tc = m_Scene->GetTransformComponent(entity);
            if (!tc) break;

            glm::vec3 pos = tc->position;

            // Build forward direction from Euler rotation (same order as renderer)
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            glm::mat4 rot(1.0f);
            if (tc->rotation.x != 0.0f) rot = glm::rotate(rot, tc->rotation.x, glm::vec3(1, 0, 0));
            if (tc->rotation.y != 0.0f) rot = glm::rotate(rot, tc->rotation.y, glm::vec3(0, 1, 0));
            if (tc->rotation.z != 0.0f) rot = glm::rotate(rot, tc->rotation.z, glm::vec3(0, 0, 1));
            forward = glm::vec3(rot * glm::vec4(forward, 0.0f));
            up      = glm::vec3(rot * glm::vec4(up, 0.0f));

            ma_engine_listener_set_position (&m_Impl->engine, 0, pos.x, pos.y, pos.z);
            ma_engine_listener_set_direction(&m_Impl->engine, 0, forward.x, forward.y, forward.z);
            ma_engine_listener_set_world_up (&m_Impl->engine, 0, up.x, up.y, up.z);
            break;  // Only the first listener is used
        }

        // --- Update 3D source positions for spatial sounds ---
        for (auto& [entity, sound] : m_Impl->sounds) {
            if (!sound) continue;

            AudioSourceComponent* asc = m_Scene->GetAudioSourceComponent(entity);
            if (!asc || !asc->spatial) continue;

            TransformComponent* tc = m_Scene->GetTransformComponent(entity);
            if (!tc) continue;

            ma_sound_set_position(sound, tc->position.x, tc->position.y, tc->position.z);
        }

        // --- Sweep finished one-shots ---
        auto it = m_Impl->oneShots.begin();
        while (it != m_Impl->oneShots.end()) {
            ma_sound* s = *it;
            if (ma_sound_at_end(s)) {
                ma_sound_uninit(s);
                delete s;
                it = m_Impl->oneShots.erase(it);
            } else {
                ++it;
            }
        }
    }

    void AudioEngine::Shutdown()
    {
        if (!m_Initialized)
            return;

        // Stop and free per-entity sounds
        for (auto& [entity, sound] : m_Impl->sounds) {
            if (sound) {
                ma_sound_stop(sound);
                ma_sound_uninit(sound);
                delete sound;
            }
        }
        m_Impl->sounds.clear();

        // Free one-shots
        for (ma_sound* s : m_Impl->oneShots) {
            ma_sound_stop(s);
            ma_sound_uninit(s);
            delete s;
        }
        m_Impl->oneShots.clear();

        ma_engine_uninit(&m_Impl->engine);

        m_Scene.reset();
        m_Initialized = false;

        std::cout << "[AudioEngine] Shutdown.\n";
    }


    // ================================================================
    // Per-entity control
    // ================================================================

    void AudioEngine::Play(EntityID entity)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;

        // Rewind to the beginning before playing so repeated calls work as expected
        ma_sound_seek_to_pcm_frame(it->second, 0);
        ma_sound_start(it->second);
    }

    void AudioEngine::Stop(EntityID entity)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;
        ma_sound_stop(it->second);
        ma_sound_seek_to_pcm_frame(it->second, 0);
    }

    void AudioEngine::Pause(EntityID entity)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;
        ma_sound_stop(it->second);  // In miniaudio stop without seek = pause
    }

    void AudioEngine::Resume(EntityID entity)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;
        ma_sound_start(it->second);
    }

    void AudioEngine::SetVolume(EntityID entity, float volume)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;
        ma_sound_set_volume(it->second, volume);
    }

    void AudioEngine::SetPitch(EntityID entity, float pitch)
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return;
        ma_sound_set_pitch(it->second, pitch);
    }

    bool AudioEngine::IsPlaying(EntityID entity) const
    {
        auto it = m_Impl->sounds.find(entity);
        if (it == m_Impl->sounds.end() || !it->second)
            return false;
        return ma_sound_is_playing(it->second) == MA_TRUE;
    }


    // ================================================================
    // Fire-and-forget
    // ================================================================

    void AudioEngine::PlayOneShot(const std::string& clipPath, float x, float y, float z, float volume)
    {
        if (!m_Initialized) return;

        std::string fullPath = ResolveClipPath(clipPath);

        ma_sound* sound = new ma_sound();
        ma_uint32 flags = MA_SOUND_FLAG_ASYNC;
        ma_result result = ma_sound_init_from_file(&m_Impl->engine, fullPath.c_str(), flags, nullptr, nullptr, sound);
        if (result != MA_SUCCESS) {
            std::cout << "[AudioEngine] PlayOneShot failed: " << fullPath << " (error " << result << ")\n";
            delete sound;
            return;
        }

        ma_sound_set_volume(sound, volume);
        ma_sound_set_spatialization_enabled(sound, MA_TRUE);
        ma_sound_set_position(sound, x, y, z);
        ma_sound_start(sound);
        m_Impl->oneShots.push_back(sound);
    }

    void AudioEngine::PlayOneShot2D(const std::string& clipPath, float volume)
    {
        if (!m_Initialized) return;

        std::string fullPath = ResolveClipPath(clipPath);

        ma_sound* sound = new ma_sound();
        ma_uint32 flags = MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION;
        ma_result result = ma_sound_init_from_file(&m_Impl->engine, fullPath.c_str(), flags, nullptr, nullptr, sound);
        if (result != MA_SUCCESS) {
            std::cout << "[AudioEngine] PlayOneShot2D failed: " << fullPath << " (error " << result << ")\n";
            delete sound;
            return;
        }

        ma_sound_set_volume(sound, volume);
        ma_sound_start(sound);
        m_Impl->oneShots.push_back(sound);
    }

    void AudioEngine::StopAll()
    {
        if (!m_Initialized) return;
        ma_engine_stop(&m_Impl->engine);
    }


    // ================================================================
    // Private helpers
    // ================================================================

    void AudioEngine::CreateSoundForEntity(EntityID entity, const AudioSourceComponent& asc)
    {
        std::string fullPath = ResolveClipPath(asc.clipPath);

        ma_sound* sound = new ma_sound();

        ma_uint32 flags = 0;
        if (!asc.spatial)
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        ma_result result = ma_sound_init_from_file(&m_Impl->engine, fullPath.c_str(), flags, nullptr, nullptr, sound);
        if (result != MA_SUCCESS) {
            std::cout << "[AudioEngine] Failed to load clip for entity " << entity
                      << ": " << fullPath << " (error " << result << ")\n";
            delete sound;
            return;
        }

        // Apply component settings
        ma_sound_set_volume(sound, asc.volume);
        ma_sound_set_pitch(sound, asc.pitch);
        ma_sound_set_looping(sound, asc.loop ? MA_TRUE : MA_FALSE);

        if (asc.spatial) {
            ma_sound_set_spatialization_enabled(sound, MA_TRUE);
            ma_sound_set_min_distance(sound, asc.minDistance);
            ma_sound_set_max_distance(sound, asc.maxDistance);

            // Set initial position from transform (if available)
            if (m_Scene) {
                TransformComponent* tc = m_Scene->GetTransformComponent(entity);
                if (tc)
                    ma_sound_set_position(sound, tc->position.x, tc->position.y, tc->position.z);
            }
        } else {
            ma_sound_set_spatialization_enabled(sound, MA_FALSE);
        }

        m_Impl->sounds[entity] = sound;

        std::cout << "[AudioEngine] Loaded sound for entity " << entity
                  << ": " << asc.clipPath << "\n";
    }

    std::string AudioEngine::ResolveClipPath(const std::string& clipPath) const
    {
        // If the path looks absolute (has a drive letter on Windows or leading slash), use as-is.
        if (clipPath.size() >= 2 && clipPath[1] == ':')
            return clipPath;
        if (!clipPath.empty() && (clipPath[0] == '/' || clipPath[0] == '\\'))
            return clipPath;

        // Otherwise treat as relative to the assets folder.
        return m_AssetsPath + clipPath;
    }

}
