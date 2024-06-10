#pragma once
#include <steam/steamnetworkingtypes.h>
#include <string>

namespace TechEngine {
    using ClientID = HSteamNetConnection;

    struct ClientInfo {
        ClientID ID;
        std::string ConnectionDesc;
    };
}
