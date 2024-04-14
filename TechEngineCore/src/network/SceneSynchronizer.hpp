#pragma once


#include "scene/Scene.hpp"
#include "serialization/Buffer.hpp"
#include "serialization/BufferStream.hpp"

namespace TechEngine::SceneSynchronizer {
    inline Buffer serializeGameObject(GameObject& gameObject) {
        Buffer buffer;
        buffer.allocate(sizeof(GameObject));
        BufferStreamWriter stream(buffer);
        stream.writeRaw(PacketType::SyncGameObject);
        stream.writeString(gameObject.getName());
        stream.writeString(gameObject.getTag());
        stream.writeObject(gameObject);
        return buffer;
    }

    inline GameObject& deserializeGameObject(BufferStreamReader& stream, Scene& scene) {
        std::string name;
        std::string tag;
        stream.readString(name);
        stream.readString(tag);
        GameObject& gameObject = scene.registerGameObject<GameObject>(name, tag);
        stream.readObject(gameObject);
        for (auto& component: gameObject.getComponents()) {
            component.second->setGameObject(gameObject);
        }
        return gameObject;
    }

    inline std::vector<Buffer> serializeScene(Scene& scene) {
        std::vector<Buffer> buffers;
        for (auto element: scene.getGameObjects()) {
            buffers.push_back(serializeGameObject(*element));
        }
    }

    inline void deserializeScene(BufferStreamReader& stream, Scene& scene) {
        deserializeGameObject(stream, scene);
    }
}
