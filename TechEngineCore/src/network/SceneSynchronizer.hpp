#pragma once


#include "serialization/Buffer.hpp"
#include "serialization/BufferStream.hpp"
#include "components/network/NetworkSync.hpp"
#include "network/PacketType.hpp"

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
            gameObject = &scene.registerGameObject(name, tag);
        } else {
            gameObject = scene.getGameObject(name);
        }
        stream.readObject(gameObject);
        return *gameObject;
    }

    inline void deserializeScene(BufferStreamReader& stream, SceneManager& sceneManager) {
        size_t size;
        sceneManager.getScene().clear();
        stream.readRaw<size_t>(size);
        for (int i = 0; i < size; i++) {
            deserializeGameObject(stream, sceneManager.getScene());
        }
    }

    inline Buffer serializeGameState(SceneManager& sceneManager) {
        Buffer buffer;
        buffer.allocate(sizeof(PacketType) + sizeof(size_t));
        BufferStreamWriter stream(buffer);
        stream.writeRaw(PacketType::SyncGameState);
        size_t size = 0;
        std::vector<GameObject*> gameObjects;
        for (auto& gameObject: sceneManager.getScene().getGameObjects()) {
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
