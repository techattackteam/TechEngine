#include "Test.hpp"

#include "scriptingAPI/scene/SceneManagerAPI.hpp"
#include "scriptingAPI/scene/SceneAPI.hpp"
#include "components/network/NetworkSync.hpp"

void Test::onStart() {
    auto gameObject = TechEngine::SceneManagerAPI::getScene()->createGameObject("TestObject");
    gameObject->addComponent<TechEngine::NetworkSync>();
}
