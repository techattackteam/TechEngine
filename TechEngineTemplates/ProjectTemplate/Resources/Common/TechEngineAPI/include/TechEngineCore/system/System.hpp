#pragma once

class System {
protected:
    System() = default;

    friend class SystemsRegistry;

public:
    System(const System&) = delete;

    System& operator=(const System&) = delete;

    virtual ~System() = default;
};
