#pragma once


#include "glm/gtx/string_cast.hpp"
#include "spdlog/spdlog.h"

namespace TechEngine {
    class Logger {
    private:
        static std::shared_ptr<spdlog::logger> engineLogger;
        static std::shared_ptr<spdlog::logger> gameLogger;

    public:
        static void init();

        static std::shared_ptr<spdlog::logger>& getEngineLogger();

        static std::shared_ptr<spdlog::logger>& getGameLogger();
    };
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector) {
    return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix) {
    return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion) {
    return os << glm::to_string(quaternion);
}

// Core log macros
#define TE_LOGGER_TRACE(...) TechEngine::Logger::getEngineLogger()->trace(__VA_ARGS__)
#define TE_LOGGER_INFO(...) TechEngine::Logger::getEngineLogger()->info(__VA_ARGS__)
#define TE_LOGGER_WARN(...) TechEngine::Logger::getEngineLogger()->warn(__VA_ARGS__)
#define TE_LOGGER_ERROR(...) TechEngine::Logger::getEngineLogger()->error(__VA_ARGS__)
#define TE_LOGGER_CRITICAL(...) {TechEngine::Logger::getEngineLogger()->critical(__VA_ARGS__); __debugbreak();}

// Client log macros
#define GAME_LOGGER_TRACE(...) TechEngine::Logger::getGameLogger()->trace(__VA_ARGS__)
#define GAME_LOGGER_INFO(...) TechEngine::Logger::getGameLogger()->info(__VA_ARGS__)
#define GAME_LOGGER_WARN(...) TechEngine::Logger::getGameLogger()->warn(__VA_ARGS__)
#define GAME_LOGGER_ERROR(...) TechEngine::Logger::getGameLogger()->error(__VA_ARGS__)
#define GAME_LOGGER_CRITICAL(...) TechEngine::Logger::getGameLogger()->critical(__VA_ARGS__)
