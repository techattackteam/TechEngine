#pragma once
#include "events/network/SyncNetworkInt.hpp"
#include "NetworkObject.hpp"
#include <functional>

namespace TechEngine {
    class CORE_DLL NetworkVariable {
        friend class NetworkObject;

    private:
        NetworkObject* networkObject;
        std::string name;
        int value;
        std::function<void(int, int)> valueChangedCallback;

    public:
        NetworkVariable(NetworkObject* networkObject, const std::string& name, int value);

        ~NetworkVariable();

        int getValue() const;

        void setValue(int value);

        void onValueChanged(std::function<void(int, int)> callback);

    private:
        void sync(SyncNetworkInt* event);
    };
}


/*
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
*/

#define NetworkVar(name, value) TechEngine::NetworkVariable name = TechEngine::NetworkVariable(this, #name, value)
