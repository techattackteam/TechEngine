#include "GameObject.hpp"

#include <utility>
#include "eventSystem/EventDispatcher.hpp"
#include "components/TransformComponent.hpp"
#include "Scene.hpp"
#include <iostream>

#include "events/gameObjects/GameObjectCreateEvent.hpp"
#include "events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    GameObject::GameObject(std::string name) : name(std::move(name)) {
        addComponent<TransformComponent>();
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::GameObject(std::string name, const std::string& tag) : name(std::move(name)), tag(tag) {
        addComponent<TransformComponent>();
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::GameObject(GameObject* gameObject, const std::string& tag): tag(tag) {
        name = gameObject->name + "(copy)";
        for (auto& element: gameObject->components) {
            components[element.first] = element.second->copy(this, element.second);
        }
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
    }

    GameObject::~GameObject() {
        if (parent != nullptr) {
            parent->removeChild(tag);
        }
        deleteChildren();
        EventDispatcher::getInstance().dispatch(new GameObjectDestroyEvent(tag));
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
        stream->writeMap(gameObject.components);
        //stream->writeMap(gameObject.children);
    }

    void GameObject::Deserialize(StreamReader* stream, GameObject& gameObject) {
        stream->readString(gameObject.name);
        stream->readString(gameObject.tag);
        stream->readMap(gameObject.components);
        //stream->readMap(gameObject.children);
    }
}
