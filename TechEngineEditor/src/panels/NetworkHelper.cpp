#include "NetworkHelper.hpp"


namespace TechEngine {
    NetworkHelper::NetworkHelper(NetworkEngine& networkEngine) : networkEngine(networkEngine), Panel("Network Helper") {
    }

    NetworkHelper::~NetworkHelper() {
    }

    void NetworkHelper::onUpdate() {
        ImGui::Text("Network Helper");
        ImGui::Separator();
        ImGui::Text("IP: ");
        std::string hint = "localhost:25565";
        std::string serverAddress = networkEngine.getServerAddress();
        ImGui::InputTextWithHint("##", hint.c_str(), &serverAddress);
        if (serverAddress.empty()) {
            serverAddress = "localhost:25565";
            networkEngine.setServerAddress(serverAddress);
        } else if (serverAddress != networkEngine.getServerAddress()) {
            networkEngine.setServerAddress(serverAddress);
        }
        if (networkEngine.getConnectionStatus() == NetworkEngine::ConnectionStatus::Connected) {
            if (ImGui::Button("Disconnect")) {
                networkEngine.disconnectServer();
            }
        } else {
            if (ImGui::Button("Connect")) {
                networkEngine.connectServer();
            }
        }

        ImGui::Text("Connection status: %s", networkEngine.getConnectionStatus() == NetworkEngine::ConnectionStatus::Connected ? "Connected" : "Disconnected");
        ImGui::Text("Debug message: %s", networkEngine.getConnectionDebugMessage().c_str());
    }
}
