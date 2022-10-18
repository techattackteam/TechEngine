#include <string>
#include <vector>

#pragma once
namespace TechEngineCore {
    using StateName = std::string;

    class State {
    protected:
        StateName stateName;

        std::vector<StateName> transitions;

    public:

        explicit State(const StateName &stateName);

        virtual ~State() = default;

        virtual void enter() = 0;

        virtual void leave() = 0;

        StateName getName();

        std::vector<StateName> getTransitions();
    };
}
