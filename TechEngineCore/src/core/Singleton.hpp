#pragma once


namespace TechEngine {
    template<typename T>
    class Singleton {
    public:
        static T& getInstance() {
            static T instance;
            return instance;
        }

        void operator=(const Singleton&) = delete;

        Singleton(const Singleton&) = delete;

    protected:
        Singleton() = default;

        virtual ~Singleton() = default;
    };
}
