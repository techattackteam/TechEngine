#include "Light.hpp"

Light::Light() : GameObject("Light", false) {
    addComponent<TechEngine::DirectionalLightComponent>(this);
}
