#pragma once
#include "events/network/SyncNetworkInt.hpp"
#include "NetworkObject.hpp"

namespace TechEngine {
    template<typename T>
    class CORE_DLL NetworkVariable {
    private:
        T value;
        std::string name;

    public:
        explicit NetworkVariable(NetworkObject* networkObject, std::string name, T value);

        T getValue() const;

        void setValue(T value);

    private:
        void sync(SyncNetworkInt* event);
    };
}


template class TechEngine::NetworkVariable<int>;
template class TechEngine::NetworkVariable<float>;
template class TechEngine::NetworkVariable<double>;
template class TechEngine::NetworkVariable<bool>;
template class TechEngine::NetworkVariable<char>;
template class TechEngine::NetworkVariable<unsigned char>;
template class TechEngine::NetworkVariable<short>;
template class TechEngine::NetworkVariable<unsigned short>;
template class TechEngine::NetworkVariable<long>;
template class TechEngine::NetworkVariable<unsigned long>;
template class TechEngine::NetworkVariable<long long>;
template class TechEngine::NetworkVariable<unsigned long long>;

#define NetworkVar(NetworkObject, type, name, value) TechEngine::NetworkVariable<type> name{NetworkObject, #name, value}
