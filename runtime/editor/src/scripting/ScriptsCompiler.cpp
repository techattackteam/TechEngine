#include "ScriptsCompiler.hpp"
#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"

#include <cstdio>
#include <cassert>

namespace TechEngine::ScriptsCompiler {
    std::string execCommand(const std::string& cmd) {
        assert(!cmd.empty());
        //Pre-allocation of the string
        std::string result;
        result.reserve(1024);
        // Open pipe to file
#ifdef _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif

        if (!pipe) {
            TE_LOGGER_INFO("popen() failed!");
            return "";
        }

        // Read the output a character at a time
        char ch;
        while ((ch = fgetc(pipe.get())) != EOF) {
            result += ch;
        }

        return result;
    }


    void compileUserScripts(ProjectManager& projectManager, CompileMode compileMode, ProjectType projectType) {
        assert(projectType == ProjectType::Client || projectType == ProjectType::Server);
        std::string cmakeBuildPath = projectManager.getCmakeBuildPath(compileMode).string();
        std::string cmakeListPath = projectManager.getCmakeListPath().string();

        std::string cm;
        std::string buildType;
        if (compileMode == CompileMode::Release) {
            cm = "Release";
            buildType = " -DCMAKE_BUILD_TYPE=Release";
        } else if (compileMode == CompileMode::Debug) {
            cm = "Debug";
            buildType = " -DCMAKE_BUILD_TYPE=Debug";
        } else if (compileMode == CompileMode::RelWithDebInfo) {
            cm = "RelWithDebInfo";
            buildType = " -DCMAKE_BUILD_TYPE=RelWithDebInfo";
        } else {
            TE_LOGGER_ERROR("Unknown compile mode: " + std::to_string(static_cast<int>(compileMode)));
            return;
        }

        std::string command = "\"" + projectManager.getCmakePath().string() +
                              " -G \"Visual Studio 17 2022\""
                              " -S " + "\"" + cmakeListPath + "\"" +
                              " -B " + "\"" + cmakeBuildPath + "\"" +
                              buildType + "\"";

        TE_LOGGER_INFO(execCommand(command));
        command = "\"" + projectManager.getCmakePath().string() +
                  " --build " + "\"" + cmakeBuildPath + "\"" +
                  " --target " + (projectType == ProjectType::Client
                                      ? "ClientScripts"
                                      : "ServerScripts") +
                  " --config " + cm + "\"";
        TE_LOGGER_INFO(execCommand(command));
    }
}
