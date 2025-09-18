#include "WidgetsSerializer.hpp"


#include "WidgetsRegistry.hpp"
#include "events/scene/SceneLoadEvent.hpp"
#include "events/scene/SceneSaveEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "utils/YAMLUtils.hpp"

#include <fstream>

namespace TechEngine {
    void WidgetsSerializer::init(SystemsRegistry& systemsRegistry) {
        EventDispatcher& eventDispatcher = systemsRegistry.getSystem<EventDispatcher>();
        eventDispatcher.subscribe<SceneLoadEvent>([&](const std::shared_ptr<Event>& event) {
            const auto* sceneLoadEvent = dynamic_cast<SceneLoadEvent*>(event.get());
            if (sceneLoadEvent) {
                deserializeUI(sceneLoadEvent->getSceneName(), systemsRegistry);
            }
        });
        eventDispatcher.subscribe<SceneSaveEvent>([&](const std::shared_ptr<Event>& event) {
            TE_LOGGER_INFO("SceneSaveEvent received in WidgetsSerializer");
            const auto* sceneSaveEvent = dynamic_cast<SceneSaveEvent*>(event.get());
            if (sceneSaveEvent) {
                serializeUI(sceneSaveEvent->getSceneName(), systemsRegistry);
            }
        });
    }

    void WidgetsSerializer::serializeUI(const std::string& sceneName, SystemsRegistry& systemsRegistry) {
        std::filesystem::path path = systemsRegistry.getSystem<ScenesManager>().m_scenesBank[sceneName];
        YAML::Node node = YAML::LoadFile(path.string());
        std::ofstream file(path);
        if (!file.is_open()) {
            TE_LOGGER_ERROR("Failed to open scene file for writing: {}", path.string());
            return;
        }
        YAML::Emitter out;
        out << node;
        out << YAML::BeginMap;
        out << YAML::Key << "Widgets" << YAML::Value << YAML::BeginSeq;
        for (const auto& widget: systemsRegistry.getSystem<WidgetsRegistry>().getRootWidgets()) {
            out << YAML::BeginMap;
            if (widget) {
                widget->serialize(out);
            }
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        file << out.c_str();
    }

    void WidgetsSerializer::deserializeUI(const std::string& sceneName, SystemsRegistry& systemsRegistry) {
        std::filesystem::path path = systemsRegistry.getSystem<ScenesManager>().m_scenesBank[sceneName];
        std::vector<YAML::Node> docs = YAML::LoadAllFromFile(path.string());
        if (docs.size() < 2) {
            TE_LOGGER_WARN("No Widgets section found in scene: {}", sceneName);
            return;
        }
        YAML::Node node = docs[1];

        for (const YAML::Node& widgetNode: node["Widgets"]) {
            if (widgetNode["Name"].IsDefined()) {
                std::string widgetName = widgetNode["Name"].as<std::string>();
                std::string widgetType = widgetNode["Type"].as<std::string>();
                auto widget = systemsRegistry.getSystem<WidgetsRegistry>().createWidget(nullptr, widgetName, widgetType, false);
                widget->deserialize(widgetNode, systemsRegistry.getSystem<WidgetsRegistry>());
                TE_LOGGER_INFO("Deserialized widget: {0}", widgetName);
            }
        }
    }
}
