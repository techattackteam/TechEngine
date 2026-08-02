#include <TechEngine/base/Math.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <concepts>

static_assert(std::same_as<TechEngine::Vec2, glm::vec2>);
static_assert(std::same_as<TechEngine::Vec3, glm::vec3>);
static_assert(std::same_as<TechEngine::Vec4, glm::vec4>);

static_assert(std::same_as<TechEngine::IVec2, glm::ivec2>);
static_assert(std::same_as<TechEngine::IVec3, glm::ivec3>);
static_assert(std::same_as<TechEngine::IVec4, glm::ivec4>);

static_assert(std::same_as<TechEngine::UVec2, glm::uvec2>);
static_assert(std::same_as<TechEngine::UVec3, glm::uvec3>);
static_assert(std::same_as<TechEngine::UVec4, glm::uvec4>);

static_assert(std::same_as<TechEngine::Mat3, glm::mat3>);
static_assert(std::same_as<TechEngine::Mat4, glm::mat4>);

static_assert(std::same_as<TechEngine::Quat, glm::quat>);

static_assert(std::same_as<TechEngine::Vec3::value_type, float>);
