#include <imgui_internal.h>
#include <iostream>
#include "NetworkHandlerComponent.hpp"

namespace TechEngine {

    NetworkHandlerComponent::NetworkHandlerComponent(GameObject *gameObject) :
            networkHandler(std::string("localhost"), 25565, port), Component(gameObject, "NetworkHandler") {
    }

    NetworkHandlerComponent::~NetworkHandlerComponent() {
    }


    void NetworkHandlerComponent::connectionWithServer() {
        networkHandler.connectWithServer();
    }

    void NetworkHandlerComponent::update() {
        networkHandler.update();
    }

    void NetworkHandlerComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {
            ImGui::PushID(name.c_str());
            ImGui::PushMultiItemsWidths(1, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

            ImGui::InputInt("Port: ", &port);
            networkHandler.setPort(port);
            ImGui::NewLine();
            if (ImGui::Button("Connect", ImVec2(200.0f, 50.0f))) {
                std::cout << "Connecting with server" << std::endl;
                connectionWithServer();
            }

            ImGui::PopItemWidth();
            ImGui::PopStyleVar();
            ImGui::PopID();
        }
    }
}
