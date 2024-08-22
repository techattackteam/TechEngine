#pragma once


#include "systems/System.hpp"

#include "glm/gtx/string_cast.hpp"
#include "spdlog/spdlog.h"
#include <spdlog/sinks/dist_sink.h>

namespace TechEngine {
    class CORE_DLL Logger : public System {
    private:
        inline static std::shared_ptr<spdlog::logger> logger;
        inline static bool initialized = false;
        std::string name;
        std::shared_ptr<spdlog::sinks::dist_sink_mt> m_distSinks;

    public:
        explicit Logger(const std::string& name);

        void init() override;

        void shutdown() override;

        void addLogSink(const std::shared_ptr<spdlog::sinks::sink>& sink);

        static std::shared_ptr<spdlog::logger>& getLogger();
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
#define TE_LOGGER_TRACE(...) TechEngine::Logger::getLogger()->trace(__VA_ARGS__)
#define TE_LOGGER_INFO(...) TechEngine::Logger::getLogger()->info(__VA_ARGS__)
#define TE_LOGGER_WARN(...) TechEngine::Logger::getLogger()->warn(__VA_ARGS__)
#define TE_LOGGER_ERROR(...) TechEngine::Logger::getLogger()->error(__VA_ARGS__)
#define TE_LOGGER_CRITICAL(...) {TechEngine::Logger::getLogger()->critical(__VA_ARGS__); __debugbreak();}
