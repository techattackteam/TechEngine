#pragma once

#include <vector>
#include <stdexcept>

namespace TechEngine {

class IComponentStorage {
public:
    virtual ~IComponentStorage() = default;

    virtual void pop_back() = 0;

    virtual void clear() = 0;

    virtual bool empty() const = 0;

    virtual size_t size() const = 0;

    virtual void* getData() = 0;

    virtual const void* getData() const = 0;

    virtual void resize(size_t new_size) = 0;

    virtual void reserve(size_t new_cap) = 0;

    virtual void remove(size_t index, size_t entityCount) = 0;

    virtual void copyFrom(size_t sourceIndex, const IComponentStorage& sourceStorage) = 0;

    virtual void push_back_default() = 0;

    virtual void set_from(size_t index, const IComponentStorage& sourceStorage, size_t sourceIndex) = 0;
};

template<typename T>
class ComponentStorage : public IComponentStorage {
private:
    std::vector<T> m_data;

public:
    ~ComponentStorage() override = default;

    T& get(size_t index) {
        return m_data[index];
    }

    void add(const T& component) {
        m_data.push_back(component);
    }

    void pop_back() override {
        m_data.pop_back();
    }

    size_t size() const override {
        return m_data.size();
    }

    void clear() override {
        m_data.clear();
    }

    bool empty() const override {
        return m_data.empty();
    }

    void resize(size_t new_size) override {
        m_data.resize(new_size);
    }

    void* getData() override {
        return m_data.data();
    }

    const void* getData() const override {
        return m_data.data();
    }

    void reserve(size_t new_cap) override {
        m_data.reserve(new_cap);
    }

    void remove(size_t index, size_t entity_count) override {
        if (index < entity_count - 1) {
            m_data[index] = m_data.back();
        }
        pop_back();
    }

    void copyFrom(size_t sourceIndex, const IComponentStorage& sourceStorage) override {
        const auto& typedSource = static_cast<const ComponentStorage<T>&>(sourceStorage);
        m_data.push_back(typedSource.m_data[sourceIndex]);
    }

    void push_back_default() override {
        m_data.emplace_back(); // Creates a T()
    }

    void set_from(size_t index, const IComponentStorage& sourceStorage, size_t sourceIndex) override {
        const auto& typedSource = static_cast<const ComponentStorage<T>&>(sourceStorage);
        m_data[index] = typedSource.m_data[sourceIndex];
    }

    // We also need a way to set the new component's value directly
    void set(size_t index, const T& value) {
        // This is the safety check.
        if (index >= m_data.size()) {
            // If this happens, it means an entity was added to an archetype
            // without its component data arrays being correctly resized.
            throw std::runtime_error("FATAL: Component data size is out of sync with entity index.");
        }
        m_data[index] = value;
    }
};
}
