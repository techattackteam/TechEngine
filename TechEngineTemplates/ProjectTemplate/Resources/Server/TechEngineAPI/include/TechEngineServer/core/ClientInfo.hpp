#pragma once
#include <steam/steamnetworkingtypes.h>
#include <string>

namespace TechEngine {
    using ClientID = HSteamNetConnection;

    struct ClientInfo {
        ClientID internalID;
        int networkID;
        std::string ConnectionDesc;
    };

    inline bool operator==(const ClientInfo& lhs, const ClientInfo& rhs) {
        return lhs.internalID == rhs.internalID;
    }

    inline bool operator!=(const ClientInfo& lhs, const ClientInfo& rhs) {
        return !(lhs == rhs);
    }
}
