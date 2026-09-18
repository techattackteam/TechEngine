#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace TechEngine {
    template<typename T>
    concept ComponentValue = std::default_initializable<T> && std::copyable<T> && std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

    class IComponentStorage {
    public:
        virtual ~IComponentStorage() = default;

        virtual std::size_t size() const = 0;

        virtual void reserve(std::size_t capacity) = 0;

        virtual void appendDefault() = 0;

        virtual void setCopy(std::size_t destination, const IComponentStorage& source, std::size_t sourceIndex) = 0;

        virtual void* element(std::size_t index) = 0;

        virtual const void* element(std::size_t index) const = 0;

        virtual void popBack() noexcept = 0;

        virtual void eraseSwap(std::size_t index) = 0;
    };

    template<ComponentValue T>
    class ComponentStorage final : public IComponentStorage {
    private:
        std::vector<T> m_values;

    public:
        std::size_t size() const override {
            return m_values.size();
        }

        void reserve(const std::size_t capacity) override {
            m_values.reserve(capacity);
        }

        void appendDefault() override {
            m_values.emplace_back();
        }

        void setCopy(const std::size_t destination, const IComponentStorage& source, const std::size_t sourceIndex) override {
            const auto& typedSource = static_cast<const ComponentStorage<T>&>(source);
            m_values[destination] = typedSource.m_values[sourceIndex];
        }

        void* element(const std::size_t index) override {
            return &m_values.at(index);
        }

        const void* element(const std::size_t index) const override {
            return &m_values.at(index);
        }

        void popBack() noexcept override {
            m_values.pop_back();
        }

        void eraseSwap(const std::size_t index) override {
            if (index + 1U < m_values.size()) {
                m_values[index] = std::move(m_values.back());
            }
            m_values.pop_back();
        }

        std::span<T> values() {
            return m_values;
        }

        std::span<const T> values() const {
            return m_values;
        }

        void set(const std::size_t index, const T& value) {
            m_values[index] = value;
        }
    };

    template<ComponentValue T>
    std::unique_ptr<IComponentStorage> createComponentStorage() {
        return std::make_unique<ComponentStorage<T>>();
    }
}
