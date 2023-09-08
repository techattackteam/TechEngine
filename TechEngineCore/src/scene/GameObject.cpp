#include "GameObject.hpp"

#include <utility>
#include "event/EventDispatcher.hpp"
#include "components/TransformComponent.hpp"
#include "Scene.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"
#include <iostream>

namespace TechEngine {
    GameObject::GameObject(std::string name) : name(std::move(name)) {
        addComponent<TransformComponent>();
        TechEngine::EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
        Scene::getInstance().registerGameObject(this);
    }

    GameObject::GameObject(std::string name, const std::string &tag) : name(std::move(name)) {
        addComponent<TransformComponent>();
        TechEngine::EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
        Scene::getInstance().registerGameObject(this, tag);
    }

    GameObject::GameObject(std::string name, GameObject *pObject) : name(std::move(name)) {
        addComponent<TransformComponent>();
        TechEngine::EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
        Scene::getInstance().registerGameObject(this);
    }

    GameObject::~GameObject() {
        if (parent == nullptr) {
            Scene::getInstance().unregisterGameObject(this);
        } else {
            parent->removeChild(tag);
        }
        deleteChildren();
        TechEngine::EventDispatcher::getInstance().dispatch(new GameObjectDestroyEvent(tag));
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

    void GameObject::makeParent(GameObject *parent) {
        this->parent = parent;
    }

    void GameObject::addChild(GameObject *child) {
        child->makeParent(this);
        children.insert(std::make_pair(child->getTag(), child));
    }

    void GameObject::removeParent() {
        removeChild(parent->getTag());
    }

    void GameObject::removeChild(const std::string &tag) {
        if (children.contains(tag)) {
            GameObject *child = children.at(tag);
            child->parent = nullptr;
            children.erase(tag);
        }
    }

    glm::mat4 GameObject::getModelMatrix() {
        return getTransform().getModelMatrix();
    }

    std::string GameObject::getName() {
        return name;
    }

    std::string GameObject::getTag() {
        return tag;
    }

    GameObject *GameObject::getParent() {
        return parent;
    }

    TransformComponent &GameObject::getTransform() {
        return *getComponent<TransformComponent>();
    }

    std::unordered_map<std::string, Component *> &GameObject::getComponents() {
        return components;
    }

    std::unordered_map<std::string, GameObject *> &GameObject::getChildren() {
        return children;
    }

    void GameObject::deleteChildren() {
        while (!children.empty()) {
            GameObject *child = children.begin()->second;
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

}