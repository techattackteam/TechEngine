#include <engine/components/DirectionalLightComponent.hpp>
#include "Light.hpp"

Light::Light() : GameObject("Light", false) {
    addComponent<Engine::DirectionalLightComponent>(this);
}
