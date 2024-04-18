#pragma once


#include "scene/Scene.hpp"
#include "serialization/Buffer.hpp"
#include "serialization/BufferStream.hpp"
#include "components/network/NetworkSync.hpp"

namespace TechEngine::SceneSynchronizer {
    inline Buffer serializeGameObject(GameObject& gameObject) {
        Buffer buffer;
        buffer.allocate(sizeof(PacketType));
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
        GameObject* gameObject;
        if (scene.getGameObject(name) == nullptr) {
            gameObject = &scene.registerGameObject<GameObject>(name, tag);
        } else {
            gameObject = scene.getGameObject(name);
        }
        stream.readObject(gameObject);
        return *gameObject;
    }

    inline void deserializeScene(BufferStreamReader& stream, Scene& scene) {
        size_t size;
        stream.readRaw<size_t>(size);
        for (int i = 0; i < size; i++) {
            deserializeGameObject(stream, scene);
        }
    }

    inline Buffer serializeGameState(Scene& scene) {
        Buffer buffer;
        buffer.allocate(sizeof(PacketType) + sizeof(size_t));
        BufferStreamWriter stream(buffer);
        stream.writeRaw(PacketType::SyncGameState);
        size_t size = 0;

        std::vector<GameObject*> gameObjects;
        for (auto& gameObject: scene.getGameObjects()) {
            if (gameObject->hasComponent<NetworkSync>()) {
                size++;
                gameObjects.push_back(gameObject);
            }
        }
        stream.writeRaw<size_t>(size);
        for (auto& gameObject: gameObjects) {
            stream.writeString(gameObject->getName());
            stream.writeString(gameObject->getTag());
            stream.writeObject(gameObject);
        }
        return buffer;
    }
}
