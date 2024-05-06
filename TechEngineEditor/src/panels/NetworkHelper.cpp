#include "NetworkHelper.hpp"


namespace TechEngine {
    NetworkHelper::NetworkHelper(NetworkEngine& networkEngine, EventDispatcher& eventDispatcher) : networkEngine(networkEngine), Panel("Network Helper", eventDispatcher) {
    }

    NetworkHelper::~NetworkHelper() {
    }

    void NetworkHelper::onUpdate() {
        ImGui::Text("Network Helper");
        ImGui::Separator();
        ImGui::Text("IP: ");
        std::string hintIP = "localhost";
        std::string hintPort = "localhost";
        static std::string serverAddress = "";
        static std::string serverPort = "";
        ImGui::InputTextWithHint("##", hintIP.c_str(), &serverAddress, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Text("Port: ");
        ImGui::InputTextWithHint("###", hintPort.c_str(), &serverPort, ImGuiInputTextFlags_EnterReturnsTrue);
        if (serverAddress.empty()) {
            serverAddress = "localhost";
        }
        if (serverPort.empty()) {
            serverPort = "25565";
        }
        if (networkEngine.getConnectionStatus() == NetworkEngine::ConnectionStatus::Connected) {
            if (ImGui::Button("Disconnect")) {
                networkEngine.disconnectServer();
            }
        } else {
            if (ImGui::Button("Connect")) {
                networkEngine.connectServer("localhost", "25565");
            }
        }

        ImGui::Text("Connection status: %s", networkEngine.getConnectionStatus() == NetworkEngine::ConnectionStatus::Connected ? "Connected" : "Disconnected");
        ImGui::Text("Debug message: %s", networkEngine.getConnectionDebugMessage().c_str());
    }
}
