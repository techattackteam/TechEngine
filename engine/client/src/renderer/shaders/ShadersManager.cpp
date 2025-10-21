#include "ShadersManager.hpp"
#include "files/FileUtils.hpp"
#include "core/Logger.hpp"
#include "project/Project.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ShadersManager::ShadersManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), activeShader(nullptr) {
    }

    ShadersManager::~ShadersManager() {
    }

    void ShadersManager::init() {
        /*shaders = std::unordered_map<std::string, Shader*>();
        std::string clientPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string();
        shaders["line"] = new Shader("line", (clientPath + "/assets/shaders/lineVertex.glsl").c_str(),
                                     (clientPath + "/assets/shaders/lineFragment.glsl").c_str());
        //shaders["geometry"] = new Shader("geometry", (clientPath + "/assets/shaders/geometryVertex.glsl").c_str(),
        //                                 (clientPath + "/assets/shaders/geometryFragment.glsl").c_str());
        //shaders["geometryOld"] = new Shader("line", (clientPath + "/assets/shaders/geometryOldVertex.glsl").c_str(),
        //                             (clientPath + "/assets/shaders/geometryOldFragment.glsl").c_str());
        //shaders["depthPrePass"] = new Shader("depthPrePass", (clientPath + "/assets/shaders/depthPrePassVertex.glsl").c_str(),
        //                                     (clientPath + "/assets/shaders/depthPrePassFragment.glsl").c_str());
        //shaders["lightCull"] = new Shader("lightCull", (clientPath + "/assets/shaders/lightCullingCompute.glsl").c_str());

        shaders["geometry"] = new Shader("geometry", ("C:/dev/TechEngine/templates/project/resources/client/assets/shaders/geometryVertex.glsl"),
                                         ("C:/dev/TechEngine/templates/project/resources/client/assets/shaders/geometryFragment.glsl"));
        shaders["depthPrePass"] = new Shader("depthPrePass", ("C:/dev/TechEngine/templates/project/resources/client/assets/shaders/depthPrePassVertex.glsl"),
                                             ("C:/dev/TechEngine/templates/project/resources/client/assets/shaders/depthPrePassFragment.glsl"));
        shaders["lightCulling"] = new Shader("lightCulling", ("C:/dev/TechEngine/templates/project/resources/client/assets/shaders/lightCullingCompute.glsl"));*/

        shaders.clear();
        std::string shaderBasePath = "C:/dev/TechEngine/templates/project/resources/client/assets/shaders/";

        std::vector<ShaderInfo> shadersToBuild = {
            {
                "geometry", {
                    {ShaderType::Vertex, shaderBasePath + "geometryVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "geometryFragment.glsl"}
                }
            },
            {
                "gBufferPass", {
                    {ShaderType::Vertex, shaderBasePath + "gBuffer/gBufferVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "gBuffer/gBufferFragment.glsl"}
                }
            },
            {
                "AOCompute", {
                    {ShaderType::Compute, shaderBasePath + "ambientOcclusion/aoCompute.glsl"}
                }
            },
            {
                "lightCulling", {
                    {ShaderType::Compute, shaderBasePath + "lightCullingCompute.glsl"}
                }
            },
            {
                "omniShadowMap", {
                    {ShaderType::Vertex, shaderBasePath + "shadow/shadowCubeVertex.glsl"},
                    {ShaderType::Geometry, shaderBasePath + "shadow/shadowCubeGeometry.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "shadow/shadowCubeFragment.glsl"}
                }
            },
            {
                "depthShadowMap", {
                    {ShaderType::Vertex, shaderBasePath + "shadow/shadowDepthVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "shadow/shadowDepthFragment.glsl"}
                }
            },
            {
                "equirectangular", {
                    {ShaderType::Vertex, shaderBasePath + "IBL/equirectangularVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "IBL/equirectangularFragment.glsl"}
                }
            },
            {
                "skybox", {
                    {ShaderType::Vertex, shaderBasePath + "IBL/skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "IBL/skyboxFragment.glsl"}
                }
            },
            {
                "irradiance", {
                    {ShaderType::Vertex, shaderBasePath + "IBL/skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "IBL/irradianceFragment.glsl"}
                }
            },
            {
                "prefilter", {
                    {ShaderType::Vertex, shaderBasePath + "IBL/skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "IBL/prefilterFragment.glsl"}
                }
            },
            {
                "brdf", {
                    {ShaderType::Vertex, shaderBasePath + "IBL/brdfVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "IBL/brdfFragment.glsl"}
                }
            },

            {
                "histogram", {
                    {ShaderType::Compute, shaderBasePath + "IBL/histogramCompute.glsl"}
                }
            },
            {
                "postProcess", {
                    {ShaderType::Vertex, shaderBasePath + "postProcess/postProcessVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "postProcess/postProcessFragment.glsl"}
                }
            },
            {
                "bloomPrefilter", {
                    {ShaderType::Compute, shaderBasePath + "postProcess/effects/bloomPrefilterCompute.glsl"}
                }
            },
            {
                "bloomDownSample", {
                    {ShaderType::Compute, shaderBasePath + "postProcess/effects/bloomDownSampleCompute.glsl"}
                }
            },
            {
                "bloomUpSample", {
                    {ShaderType::Compute, shaderBasePath + "postProcess/effects/bloomUpSampleCompute.glsl"}
                }
            },
            {
                "line", {
                    {ShaderType::Vertex, shaderBasePath + "lineVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "lineFragment.glsl"}
                }
            }
        };

        for (const auto& info: shadersToBuild) {
            Shader* shader = new Shader(info.name);
            for (const auto& stage: info.stages) {
                shader->attachSourceFile(stage.m_type, stage.m_path.string());
            }

            if (shader->link()) {
                shaders[info.name] = shader;
            } else {
                TE_LOGGER_ERROR("Failed to build shader '{0}'. It will be unavailable.", info.name);
                delete shader;
            }
        }
    }

    void ShadersManager::changeActiveShader(const std::string& name) {
        if (activeShader != nullptr) {
            activeShader->unBind();
        }
        activeShader = shaders[name];
        activeShader->bind();
    }

    void ShadersManager::unBindShader() {
        activeShader->unBind();
    }

    Shader* ShadersManager::getActiveShader() {
        return activeShader;
    }

    void ShadersManager::exportShaderFiles(const std::string& path) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
        for (auto& element: shaders) {
            std::string newPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string() + "/shaders/" + element.first + "Vertex.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Vertex.glsl", newPath, e.what());
            }
            newPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string() + "/shaders/" + element.first + "Fragment.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Fragment.glsl", newPath, e.what());
            }
        }
    }
}
