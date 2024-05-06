#include "GameObject.hpp"
#include "components/ComponentFactory.hpp"
#include "components/TransformComponent.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/gameObjects/GameObjectCreateEvent.hpp"
#include "events/gameObjects/GameObjectDestroyEvent.hpp"

#include <utility>
#include <iostream>

namespace TechEngine {
    GameObject::GameObject(std::string name, EventDispatcher& eventDispatcher) : eventDispatcher(eventDispatcher), name(std::move(name)) {
        addComponent<TransformComponent>();
        eventDispatcher.dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::GameObject(std::string name, const std::string& tag, EventDispatcher& eventDispatcher) : eventDispatcher(eventDispatcher), name(std::move(name)), tag(tag) {
        addComponent<TransformComponent>();
        eventDispatcher.dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::GameObject(GameObject* gameObject, const std::string& tag, EventDispatcher& eventDispatcher): eventDispatcher(eventDispatcher), tag(tag) {
        name = gameObject->name + "(copy)";
        for (auto& element: gameObject->components) {
            components[element.first] = element.second->copy(this, element.second);
        }
        eventDispatcher.dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::~GameObject() {
        if (parent != nullptr) {
            parent->removeChild(tag);
        }
        deleteChildren();
        eventDispatcher.dispatch(new GameObjectDestroyEvent(tag));
    }

    void GameObject::fixUpdate() {
        for (auto element: components) {
            element.second->fixedUpdate();
        }
    }

    void GameObject::update() {
        for (auto element: components) {
            element.second->update();
        }
    }

    void GameObject::makeParent(GameObject* parent) {
        this->parent = parent;
    }

    void GameObject::addChild(GameObject* child) {
        child->makeParent(this);
        children.insert(std::make_pair(child->getTag(), child));
    }

    void GameObject::removeParent() {
        removeChild(parent->getTag());
    }

    void GameObject::removeChild(const std::string& tag) {
        if (children.contains(tag)) {
            GameObject* child = children.at(tag);
            child->parent = nullptr;
            children.erase(tag);
        }
    }

    glm::mat4 GameObject::getModelMatrix() {
        return getTransform().getModelMatrix();
    }

    glm::mat4 GameObject::getLocalModelMatrix() {
        return getTransform().getLocalModelMatrix();
    }

    void GameObject::setName(const std::string& name) {
        this->name = name;
    }

    std::string GameObject::getName() {
        return name;
    }

    std::string GameObject::getTag() {
        return tag;
    }

    GameObject* GameObject::getParent() {
        return parent;
    }

    TransformComponent& GameObject::getTransform() {
        return *getComponent<TransformComponent>();
    }

    std::unordered_map<std::string, Component*>& GameObject::getComponents() {
        return components;
    }

    std::unordered_map<std::string, GameObject*>& GameObject::getChildren() {
        return children;
    }

    void GameObject::deleteChildren() {
        while (!children.empty()) {
            GameObject* child = children.begin()->second;
            children.erase(child->tag);
            delete child;
        }
    }

    void GameObject::setTag(std::string tag) {
        this->tag = tag;
    }

    bool GameObject::hasParent() {
        return parent != nullptr;
    }

    bool GameObject::hasChildren() {
        return !children.empty();
    }

    void GameObject::setEditorOnly(bool editorOnly) {
        this->editorOnly = editorOnly;
    }

    bool GameObject::isEditorOnly() const {
        return editorOnly;
    }

    void GameObject::Serialize(StreamWriter* stream, const GameObject& gameObject) {
        stream->writeString(gameObject.name);
        stream->writeString(gameObject.tag);
        stream->writeRaw<int>(gameObject.components.size());
        for (auto& element: gameObject.components) {
            stream->writeString(element.first);
            element.second->Serialize(stream);
        }
    }

    void GameObject::Deserialize(StreamReader* stream, GameObject& gameObject) {
        stream->readString(gameObject.name);
        stream->readString(gameObject.tag);
        int componentsSize = 0;
        stream->readRaw<int>(componentsSize);
        for (int i = 0; i < componentsSize; i++) {
            std::string componentName;
            stream->readString(componentName);
            Component* component = ComponentFactory::createComponent(componentName, &gameObject, gameObject.eventDispatcher);
            component->Deserialize(stream);
            gameObject.components[componentName] = component;
        }
    }
}
