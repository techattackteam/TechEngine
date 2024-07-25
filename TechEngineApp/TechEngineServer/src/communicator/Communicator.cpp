#include "Communicator.hpp"

#include <utility>

#include "core/Logger.hpp"
#include "core/Server.hpp"
#include "network/NetworkObjectsManager.hpp"
#include "network/SceneSynchronizer.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"

namespace TechEngine {
    Communicator::Communicator(Server& server) : server(server) {
    }

    std::string resultToString(EResult result) {
        switch (result) {
            case k_EResultNone: return "None";
            case k_EResultOK: return "OK";
            case k_EResultFail: return "Generic failure";
            case k_EResultNoConnection: return "No/failed network connection";
            case k_EResultInvalidPassword: return "Password/ticket is invalid";
            case k_EResultLoggedInElsewhere: return "Same user logged in elsewhere";
            case k_EResultInvalidProtocolVer: return "Protocol version is incorrect";
            case k_EResultInvalidParam: return "A parameter is incorrect";
            case k_EResultFileNotFound: return "File was not found";
            case k_EResultBusy: return "Called method busy - action not taken";
            case k_EResultInvalidState: return "Called object was in an invalid state";
            case k_EResultInvalidName: return "Name is invalid";
            case k_EResultInvalidEmail: return "Email is invalid";
            case k_EResultDuplicateName: return "Name is not unique";
            case k_EResultAccessDenied: return "Access is denied";
            case k_EResultTimeout: return "Operation timed out";
            case k_EResultBanned: return "VAC2 banned";
            case k_EResultAccountNotFound: return "Account not found";
            case k_EResultInvalidSteamID: return "SteamID is invalid";
            case k_EResultServiceUnavailable: return "The requested service is currently unavailable";
            case k_EResultNotLoggedOn: return "The user is not logged on";
            case k_EResultPending: return "Request is pending (may be in process, or waiting on third party)";
            case k_EResultEncryptionFailure: return "Encryption or Decryption failed";
            case k_EResultInsufficientPrivilege: return "Insufficient privilege";
            case k_EResultLimitExceeded: return "Too much of a good thing";
            case k_EResultRevoked: return "Access has been revoked (used for revoked guest passes)";
            case k_EResultExpired: return "License/Guest pass the user is trying to access is expired";
            case k_EResultAlreadyRedeemed: return "Guest pass has already been redeemed by account, cannot be acked again";
            case k_EResultDuplicateRequest: return "The request is a duplicate and the action has already occurred in the past, ignored this time";
            case k_EResultAlreadyOwned: return "All the games in this guest pass redemption request are already owned by the user";
            case k_EResultIPNotFound: return "IP address not found";
            case k_EResultPersistFailed: return "Failed to write change to the data store";
            case k_EResultLockingFailed: return "Failed to acquire access lock for this operation";
            case k_EResultLogonSessionReplaced: return "Logon session replaced";
            case k_EResultConnectFailed: return "Connect failed";
            case k_EResultHandshakeFailed: return "Handshake failed";
            case k_EResultIOFailure: return "IO failure";
            case k_EResultRemoteDisconnect: return "Remote disconnect";
            case k_EResultShoppingCartNotFound: return "Failed to find the shopping cart requested";
            case k_EResultBlocked: return "A user didn't allow it";
            case k_EResultIgnored: return "Target is ignoring sender";
            case k_EResultNoMatch: return "Nothing matching the request found";
            case k_EResultAccountDisabled: return "Account disabled";
            case k_EResultServiceReadOnly: return "This service is not accepting content changes right now";
            case k_EResultAccountNotFeatured: return "Account doesn't have value, so this feature isn't available";
            case k_EResultAdministratorOK: return "Allowed to take this action, but only because requester is admin";
            case k_EResultContentVersion: return "A Version mismatch in content transmitted within the Steam protocol";
            case k_EResultTryAnotherCM: return "The current CM can't service the user making a request, user should try another";
            case k_EResultPasswordRequiredToKickSession: return "You are already logged in elsewhere, this cached credential login has failed";
            case k_EResultAlreadyLoggedInElsewhere: return "You are already logged in elsewhere, you must wait";
            case k_EResultSuspended: return "Long running operation (content download) suspended/paused";
            case k_EResultCancelled: return "Operation canceled (typically by user: content download)";
            case k_EResultDataCorruption: return "Operation canceled because data is ill formed or unrecoverable";
            case k_EResultDiskFull: return "Operation canceled - not enough disk space";
            case k_EResultRemoteCallFailed: return "An remote call or IPC call failed";
            case k_EResultPasswordUnset: return "Password could not be verified as it's unset server side";
            case k_EResultExternalAccountUnlinked: return "External account (PSN, Facebook...) is not linked to a Steam account";
            case k_EResultPSNTicketInvalid: return "PSN ticket was invalid";
            case k_EResultExternalAccountAlreadyLinked: return "External account (PSN, Facebook...) is already linked to some other account, must explicitly request to replace/delete the link first";
            case k_EResultRemoteFileConflict: return "The sync cannot resume due to a conflict between the local and remote files";
            case k_EResultIllegalPassword: return "The requested new password is not legal";
            case k_EResultSameAsPreviousValue: return "New value is the same as the old one ( secret question and answer )";
            case k_EResultAccountLogonDenied: return "Account login denied due to 2nd factor authentication failure";
            case k_EResultCannotUseOldPassword: return "The requested new password is not legal";
            case k_EResultInvalidLoginAuthCode: return "Account login denied due to auth code invalid";
            case k_EResultAccountLogonDeniedNoMail: return "Account login denied due to 2nd factor auth failure - and no mail has been sent - partner site specific";
            case k_EResultHardwareNotCapableOfIPT: return "Hardware not capable of IPT";
            case k_EResultIPTInitError: return "IPT failed to initialize";
            case k_EResultParentalControlRestricted: return "Operation failed due to parental control restrictions for current user";
            case k_EResultFacebookQueryError: return "Facebook query returned an error";
            case k_EResultExpiredLoginAuthCode: return "Account login denied due to auth code expired";
            case k_EResultIPLoginRestrictionFailed: return "IP login restriction failed";
            case k_EResultAccountLockedDown: return "Account locked down";
            case k_EResultAccountLogonDeniedVerifiedEmailRequired: return "Account logon denied.  Verified email required.";
            case k_EResultNoMatchingURL: return "No matching URL";
            case k_EResultBadResponse: return "Parse failure, missing field, etc.";
            case k_EResultRequirePasswordReEntry: return "The user cannot complete the action until they re-enter their password";
            case k_EResultValueOutOfRange: return "The value entered is outside the acceptable range";
            case k_EResultUnexpectedError: return "Something happened that we didn't expect to ever happen";
            case k_EResultDisabled: return "The requested service has been configured to be unavailable";
            case k_EResultInvalidCEGSubmission: return "The set of files submitted to the CEG server are not valid !";
            case k_EResultRestrictedDevice: return "The device being used is not allowed to perform this action";
            case k_EResultRegionLocked: return "The action could not be complete because it is region restricted";
            case k_EResultRateLimitExceeded: return "Temporary rate limit exceeded, try again later, different from k_EResultLimitExceeded which may be permanent";
            case k_EResultAccountLoginDeniedNeedTwoFactor: return "Need two-factor code to login";
            case k_EResultItemDeleted: return "The thing we're trying to access has been deleted";
            case k_EResultAccountLoginDeniedThrottle: return "login attempt failed, try to throttle response to possible attacker";
            case k_EResultTwoFactorCodeMismatch: return "two factor code mismatch";
            case k_EResultTwoFactorActivationCodeMismatch: return "activation code for two-factor didn't match";
            case k_EResultAccountAssociatedToMultiplePartners: return "account has been associated with multiple partners";
            case k_EResultNotModified: return "data not modified";
            case k_EResultNoMobileDevice: return "the account does not have a mobile device associated with it";
            case k_EResultTimeNotSynced: return "the time presented is out of range or tolerance";
            case k_EResultSmsCodeFailed: return "SMS code failure (no match, none pending, etc.)";
            case k_EResultAccountLimitExceeded: return "Too many accounts access this resource";
            case k_EResultAccountActivityLimitExceeded: return "Too many changes to this account";
            case k_EResultPhoneActivityLimitExceeded: return "Too many changes to this phone";
            case k_EResultRefundToWallet: return "Cannot refund to payment method, must use wallet";
            case k_EResultEmailSendFailure: return "Cannot send an email";
            case k_EResultNotSettled: return "Can't perform operation till payment has settled";
            case k_EResultNeedCaptcha: return "Needs to provide a valid captcha";
            case k_EResultGSLTDenied: return "a game server login token owned by this token's owner has been banned";
            case k_EResultGSOwnerDenied: return "game server owner is denied for other reason (account lock, community ban, vac ban, missing phone)";
            case k_EResultInvalidItemType: return "the type of thing we were requested to act on is invalid";
            case k_EResultIPBanned: return "the ip address has been banned from taking this action";
            case k_EResultGSLTExpired: return "this token has expired from disuse; can be reset for use";
            case k_EResultInsufficientFunds: return "user doesn't have enough wallet funds to complete the action";
            case k_EResultTooManyPending: return "There are too many of this thing pending already";
            case k_EResultNoSiteLicensesFound: return "No site licenses found";
            case k_EResultWGNetworkSendExceeded: return "the WG couldn't send a response because we exceeded max network send size";
            case k_EResultAccountNotFriends: return "the user is not mutually friends";
            case k_EResultLimitedUserAccount: return "the user is limited";
            case k_EResultCantRemoveItem: return "item can't be removed";
            case k_EResultAccountDeleted: return "account has been deleted";
            case k_EResultExistingUserCancelledLicense: return "A license for this already exists, but cancelled";
            case k_EResultCommunityCooldown: return "access is denied because of a community cooldown (probably from support profile data resets)";
            case k_EResultNoLauncherSpecified: return "No launcher was specified, but a launcher was needed to choose correct realm for operation.";
            case k_EResultMustAgreeToSSA: return "User must agree to china SSA or global SSA before login";
            case k_EResultLauncherMigrated: return "The specified launcher type is no longer supported; the user should be directed elsewhere";
            case k_EResultSteamRealmMismatch: return "The user's realm does not match the realm of the requested resource";
            case k_EResultInvalidSignature: return "signature check did not match";
            case k_EResultParseFailure: return "Failed to parse input";
            case k_EResultNoVerifiedPhone: return "account does not have a verified phone number";
            case k_EResultInsufficientBattery: return "user device doesn't have enough battery charge currently to complete the action";
            case k_EResultChargerRequired: return "The operation requires a charger to be plugged in, which wasn't present";
            case k_EResultCachedCredentialInvalid: return "Cached credential was invalid - user must reauthenticate";
            case K_EResultPhoneNumberIsVOIP: return "The phone number provided is a Voice Over IP number";
            case k_EResultNotSupported: return "The data being accessed is not supported by this API";
            default: return "Unknown";
        }
    }

    void Communicator::sendBufferToClient(const ClientInfo& clientInfo, Buffer buffer, bool reliable) {
        EResult result = server.m_interface->SendMessageToConnection((HSteamNetConnection)clientInfo.internalID, buffer.data, (ClientID)buffer.size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
        if (result != k_EResultOK) {
            TE_LOGGER_ERROR("Failed to send message to client: \n\tInternal ID {0}\n\tNetwork ID {1} \n\tError {2}\n\tError Desc: {3}",
                            clientInfo.internalID,
                            clientInfo.networkID,
                            static_cast<int>(result),
                            resultToString(result));
        }
    }

    void Communicator::broadcastBuffer(Buffer buffer, bool reliable, std::vector<ClientInfo> excludeClientsInfos) {
        for (const auto& [internalClientID, clientInfo]: server.m_connectedClients) {
            if (std::find(excludeClientsInfos.begin(), excludeClientsInfos.end(), clientInfo) == excludeClientsInfos.end()) {
                sendBufferToClient(clientInfo, buffer, reliable);
            }
        }
    }

    void Communicator::sendNetworkID(const ClientInfo& clientInfo) {
        Buffer buffer;
        buffer.allocate(sizeof(PacketType::NetworkID) + sizeof(int));
        BufferStreamWriter stream(buffer);
        stream.writeRaw<PacketType>(PacketType::NetworkID);
        stream.writeRaw<int>(clientInfo.networkID);
        sendBufferToClient(clientInfo, buffer, true);
    }

    Buffer Communicator::createNetworkObjectBuffer(const std::string& objectType, int owner, const std::string& networkUUID) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CreateNetworkObject) + sizeof(int) + sizeof(networkUUID));
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CreateNetworkObject);
        stream.writeString(objectType);
        stream.writeRaw<int>(owner);
        stream.writeString(networkUUID);
        return scratchBuffer;
    }

    Buffer Communicator::createNetworkVariableBuffer(unsigned int owner, const std::string& uuid, const std::string& name, int value) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CreateNetworkVariable) + sizeof(unsigned int) + (sizeof(char) * (name.size() + uuid.size())) + sizeof(int));
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CreateNetworkVariable);
        stream.writeRaw<int>(owner);
        stream.writeString(uuid);
        stream.writeString(name);
        stream.writeRaw<int>(value);
        TE_LOGGER_INFO("Created network variable buffer with uuid: {0}, name: {1}, value: {2}", uuid, name, value);
        return scratchBuffer;
    }

    void Communicator::syncGameState(const ClientInfo& clientInfo) {
        Buffer buffer = SceneSynchronizer::serializeGameState(server.systemsRegistry.getSystem<SceneManager>());
        server.m_communicator.sendBufferToClient(clientInfo, buffer, true);
    }

    void Communicator::sendNetworkObject(const ClientInfo& clientInfo, NetworkObject* networkObject) {
        Buffer scratchBuffer = createNetworkObjectBuffer(typeid(*networkObject).name(), networkObject->getOwner(), networkObject->getNetworkUUID());
        sendBufferToClient(clientInfo, scratchBuffer, true);
    }

    void Communicator::broadcastNetworkObject(NetworkObject* networkObject) {
        Buffer scratchBuffer = createNetworkObjectBuffer(typeid(*networkObject).name(), networkObject->getOwner(), networkObject->getNetworkUUID());
        broadcastBuffer(scratchBuffer, true, {});
    }

    void Communicator::syncNetworkObjects(const ClientInfo& clientInfo) {
        for (NetworkObject* networObject: server.systemsRegistry.getSystem<NetworkObjectsManager>().getNetworkObjects()) {
            sendNetworkObject(clientInfo, networObject);
        }
    }

    void Communicator::sendCustomPacket(const ClientInfo& clientInfo, const std::string& packetType, Buffer buffer, bool reliable) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CustomPacket) + buffer.size);
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CustomPacket);
        stream.writeString(packetType);
        stream.writeBuffer(buffer);
        sendBufferToClient(clientInfo, stream.getBuffer(), reliable);
    }

    void Communicator::broadcastNetworkObjectDeleted(const std::string& uuid, std::vector<ClientInfo> excludedClientInfos) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::DeleteNetworkObject) + uuid.size());
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::DeleteNetworkObject);
        stream.writeString(uuid);
        broadcastBuffer(scratchBuffer, true, excludedClientInfos);
    }

    void Communicator::deleteAllNetworkObjectsFromClient(const ClientInfo& clientInfo) {
        for (NetworkObject* networkObject: server.systemsRegistry.getSystem<NetworkObjectsManager>().getNetworkObjects()) {
            if (networkObject->getOwner() == clientInfo.networkID) {
                broadcastNetworkObjectDeleted(networkObject->getNetworkUUID());
                server.systemsRegistry.getSystem<NetworkObjectsManager>().deleteNetworkObject(networkObject->getNetworkUUID());
            }
        }
    }

    void Communicator::broadcastNetworkVariable(unsigned int owner, const std::string& uuid, const std::string& string, int value) {
        Buffer buffer = createNetworkVariableBuffer(owner, uuid, string, value);
        broadcastBuffer(buffer, true);
    }
}
