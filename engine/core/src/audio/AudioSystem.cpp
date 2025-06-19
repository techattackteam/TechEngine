#include "AudioSystem.hpp"
#include "core/Logger.hpp"


#define MA_ENABLE_VORBIS
#define MA_ENABLE_FLAC
#define MA_ENABLE_MP3
#define MA_ENABLE_WAV

#define MA_IMPLEMENTATION
#include "miniaudio.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    AudioSystem::AudioSystem(SystemsRegistry& systemsRegistry): m_systemsRegistry(systemsRegistry) {
        m_audioEngine = new ma_engine();

        ma_result result = ma_engine_init(nullptr, m_audioEngine);
        if (result != MA_SUCCESS) {
            TE_LOGGER_ERROR("Failed to initialize audio engine. Code: {0}", translateErrorCodeToString(result));
            delete m_audioEngine;
            m_audioEngine = nullptr;
            return;
        }
        TE_LOGGER_INFO("Audio engine initialized successfully.");
        playSound("C:\\Users\\Miguel Faria\\Downloads\\UI Soundpack\\UI Soundpack\\MP3\\Wood Block1.mp3");
        //playSound("C:\\Users\\Miguel Faria\\Downloads\\UI Soundpack\\UI Soundpack\\OGG\\Wood Block1.ogg");
    }

    AudioSystem::~AudioSystem() {
    }

    void AudioSystem::init() {
        System::init();
    }

    void AudioSystem::onStart() {
        System::onStart();
    }

    void AudioSystem::onUpdate() {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();

        scene.runSystem<Tag, Transform, AudioListenerComponent>([this, &scene](Tag& tag, Transform& transform, AudioListenerComponent& listener) {
            Entity entity = scene.getEntityByTag(tag);
            setListenerPosition(
                entity,
                transform.m_position,
                transform.m_forward,
                transform.m_up
            );
        });
    }

    void AudioSystem::onFixedUpdate() {
        System::onFixedUpdate();
    }

    void AudioSystem::onStop() {
        System::onStop();
    }

    void AudioSystem::shutdown() {
        if (m_audioEngine) {
            ma_engine_uninit(m_audioEngine);
            delete m_audioEngine;
            m_audioEngine = nullptr;
        }
    }

    void AudioSystem::playSound(const std::string& soundPath) {
        if (!m_audioEngine) return;
        ma_result result = ma_engine_play_sound(m_audioEngine, soundPath.c_str(), nullptr);
        if (result != MA_SUCCESS) {
            TE_LOGGER_ERROR("Failed to play sound: {0} ({1})", (int)result, translateErrorCodeToString(result));
        }
    }

    void AudioSystem::playSound3D(const std::string& soundPath, glm::vec3 position) {
        if (!m_audioEngine) return;
        // For 3D sound, it's better to create a ma_sound object to control it.
        // However, for a simple fire-and-forget 3D sound, this is how you do it:
        ma_sound sound;

        // Initialize the sound but don't start it yet.
        // The MA_SOUND_FLAG_NO_PITCH flag is a good default. We don't want spatialization affecting pitch.
        ma_result result = ma_sound_init_from_file(m_audioEngine, soundPath.c_str(), MA_SOUND_FLAG_NO_PITCH, NULL, NULL, &sound);
        if (result != MA_SUCCESS) {
            TE_LOGGER_ERROR("Failed to load 3D sound: {0}", soundPath);
            return;
        }

        // Set its position in 3D space before playing.
        ma_sound_set_position(&sound, position.x, position.y, position.z);
        ma_sound_start(&sound);

        // IMPORTANT: miniaudio automatically handles the memory for this `ma_sound`
        // because it's not set to loop. It will be cleaned up once it finishes playing.
        // This is an extremely convenient feature for fire-and-forget effects.
    }

    void AudioSystem::setListenerPosition(Entity entity, glm::vec3 position, glm::vec3 forward, glm::vec3 up) {
        if (!m_audioEngine)
            return;

        ma_engine_listener_set_direction(
            m_audioEngine,
            entity,
            forward.x, forward.y, forward.z
        );
        ma_engine_listener_set_position(
            m_audioEngine,
            entity,
            position.x, position.y, position.z
        );
        ma_engine_listener_set_world_up(
            m_audioEngine,
            entity,
            up.x, up.y, up.z
        );
    }

    void AudioSystem::registerListener(Entity listener) {
        if (!m_audioEngine) {
            TE_LOGGER_ERROR("Audio engine is not initialized. Cannot register listener.");
            return;
        }
        if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end()) {
            TE_LOGGER_WARN("Listener already registered: {0}", listener);
            return;
        }
        listeners.push_back(listener);
    }

    void AudioSystem::registerEmitter(Entity emitter) {
        if (!m_audioEngine) {
            TE_LOGGER_ERROR("Audio engine is not initialized. Cannot register emitter.");
            return;
        }
        if (std::find(emitters.begin(), emitters.end(), emitter) != emitters.end()) {
            TE_LOGGER_WARN("Emitter already registered: {0}", emitter);
            return;
        }
        emitters.push_back(emitter);
    }

    void AudioSystem::unregisterListener(Entity listener) {
        if (!m_audioEngine) {
            TE_LOGGER_ERROR("Audio engine is not initialized. Cannot unregister listener.");
            return;
        }
        auto it = std::find(listeners.begin(), listeners.end(), listener);
        if (it != listeners.end()) {
            listeners.erase(it);
        } else {
            TE_LOGGER_WARN("Listener not found: {0}", listener);
        }
    }

    void AudioSystem::unregisterEmitter(Entity emitter) {
        if (!m_audioEngine) {
            TE_LOGGER_ERROR("Audio engine is not initialized. Cannot unregister emitter.");
            return;
        }
        auto it = std::find(emitters.begin(), emitters.end(), emitter);
        if (it != emitters.end()) {
            emitters.erase(it);
        } else {
            TE_LOGGER_WARN("Emitter not found: {0}", emitter);
        }
    }

    std::string AudioSystem::translateErrorCodeToString(int errorCode) {
        ma_result result = static_cast<ma_result>(errorCode);

        switch (result) {
            // Standard Results
            case MA_SUCCESS: return "Success";
            case MA_ERROR: return "A generic error.";
            case MA_INVALID_ARGS: return "Invalid arguments.";
            case MA_INVALID_OPERATION: return "Invalid operation.";
            case MA_OUT_OF_MEMORY: return "Out of memory.";
            case MA_OUT_OF_RANGE: return "Out of range.";
            case MA_ACCESS_DENIED: return "Access denied.";
            case MA_DOES_NOT_EXIST: return "The file or resource does not exist.";
            case MA_ALREADY_EXISTS: return "The resource already exists.";
            case MA_TOO_MANY_OPEN_FILES: return "Too many open files.";
            case MA_INVALID_FILE: return "Invalid file. (Is the format enabled and the file not corrupt?)";
            case MA_TOO_BIG: return "The file or resource is too big.";
            case MA_PATH_TOO_LONG: return "The specified path is too long.";
            case MA_NAME_TOO_LONG: return "The specified name is too long.";
            case MA_NOT_DIRECTORY: return "The path is not a directory.";
            case MA_IS_DIRECTORY: return "The path is a directory.";
            case MA_DIRECTORY_NOT_EMPTY: return "The directory is not empty.";
            case MA_AT_END: return "At the end of the stream or file.";
            case MA_NO_SPACE: return "No space left on device.";
            case MA_BUSY: return "The resource is busy.";
            case MA_IO_ERROR: return "A low-level I/O error occurred.";
            case MA_INTERRUPT: return "The operation was interrupted.";
            case MA_UNAVAILABLE: return "The resource is unavailable.";
            case MA_ALREADY_IN_USE: return "The resource is already in use.";
            case MA_BAD_ADDRESS: return "Bad address.";
            case MA_BAD_SEEK: return "Bad seek.";
            case MA_BAD_PIPE: return "Bad pipe.";
            case MA_DEADLOCK: return "Deadlock detected.";
            case MA_TOO_MANY_LINKS: return "Too many links.";
            case MA_NOT_IMPLEMENTED: return "Not implemented.";
            case MA_NO_MESSAGE: return "No message.";
            case MA_BAD_MESSAGE: return "Bad message.";
            case MA_NO_DATA_AVAILABLE: return "No data available.";
            case MA_INVALID_DATA: return "Invalid data.";
            case MA_TIMEOUT: return "The operation timed out.";
            case MA_NO_NETWORK: return "No network.";
            case MA_NOT_UNIQUE: return "Not unique.";
            case MA_NOT_SOCKET: return "Not a socket.";
            case MA_NO_ADDRESS: return "No address.";
            case MA_BAD_PROTOCOL: return "Bad protocol.";
            case MA_PROTOCOL_UNAVAILABLE: return "Protocol unavailable.";
            case MA_PROTOCOL_NOT_SUPPORTED: return "Protocol not supported.";
            case MA_PROTOCOL_FAMILY_NOT_SUPPORTED: return "Protocol family not supported.";
            case MA_ADDRESS_FAMILY_NOT_SUPPORTED: return "Address family not supported.";
            case MA_SOCKET_NOT_SUPPORTED: return "Socket not supported.";
            case MA_CONNECTION_RESET: return "Connection reset.";
            case MA_ALREADY_CONNECTED: return "Already connected.";
            case MA_NOT_CONNECTED: return "Not connected.";
            case MA_CONNECTION_REFUSED: return "Connection refused.";
            case MA_NO_HOST: return "No host.";
            case MA_IN_PROGRESS: return "Operation in progress.";
            case MA_CANCELLED: return "Operation cancelled.";
            case MA_MEMORY_ALREADY_MAPPED: return "Memory is already mapped.";

            // General non-standard errors.
            case MA_CRC_MISMATCH: return "CRC mismatch.";

            // General miniaudio-specific errors.
            case MA_FORMAT_NOT_SUPPORTED: return "Format not supported.";
            case MA_DEVICE_TYPE_NOT_SUPPORTED: return "Device type not supported.";
            case MA_SHARE_MODE_NOT_SUPPORTED: return "Share mode not supported.";
            case MA_NO_BACKEND: return "No audio backend available.";
            case MA_NO_DEVICE: return "No audio device available.";
            case MA_API_NOT_FOUND: return "A required API was not found.";
            case MA_INVALID_DEVICE_CONFIG: return "Invalid device configuration.";
            case MA_LOOP: return "Looping error.";
            case MA_BACKEND_NOT_ENABLED: return "The selected backend was not enabled at compile time.";

            // State errors.
            case MA_DEVICE_NOT_INITIALIZED: return "Device not initialized.";
            case MA_DEVICE_ALREADY_INITIALIZED: return "Device already initialized.";
            case MA_DEVICE_NOT_STARTED: return "Device not started.";
            case MA_DEVICE_NOT_STOPPED: return "Device not stopped.";

            // Operation errors.
            case MA_FAILED_TO_INIT_BACKEND: return "Failed to initialize backend.";
            case MA_FAILED_TO_OPEN_BACKEND_DEVICE: return "Failed to open backend device.";
            case MA_FAILED_TO_START_BACKEND_DEVICE: return "Failed to start backend device.";
            case MA_FAILED_TO_STOP_BACKEND_DEVICE: return "Failed to stop backend device.";

            // Default case for any unknown codes
            default:
                return "Unknown or unhandled error code: " + std::to_string(errorCode);
        }
    }
}
