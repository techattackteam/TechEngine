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
                "depthPrePass", {
                    {ShaderType::Vertex, shaderBasePath + "depthPrePassVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "depthPrePassFragment.glsl"}
                }
            },
            {
                "depthToNormal", {
                    {ShaderType::Compute, shaderBasePath + "ambientOcclusion/depthToNormalCompute.glsl"}
                }
            },
            {
                "gtaoHorizon", {
                    {ShaderType::Compute, shaderBasePath + "ambientOcclusion/gtaoHorizonCompute.glsl"}
                }
            },
            {
                "gtaoDenoise", {
                    {ShaderType::Compute, shaderBasePath + "gtaoDenoiseCompute.glsl"}
                }
            },
            {
                "motionVector", {
                    {ShaderType::Compute, shaderBasePath + "ambientOcclusion/motionVectorCompute.glsl"}
                }
            },
            {
                "lightCulling", {
                    {ShaderType::Compute, shaderBasePath + "lightCullingCompute.glsl"}
                }
            },
            {
                "omniShadowMap", {
                    {ShaderType::Vertex, shaderBasePath + "shadowCubeVertex.glsl"},
                    {ShaderType::Geometry, shaderBasePath + "shadowCubeGeometry.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "shadowCubeFragment.glsl"}
                }
            },
            {
                "depthShadowMap", {
                    {ShaderType::Vertex, shaderBasePath + "shadowDepthVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "shadowDepthFragment.glsl"}
                }
            },
            {
                "equirectangular", {
                    {ShaderType::Vertex, shaderBasePath + "equirectangularVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "equirectangularFragment.glsl"}
                }
            },
            {
                "skybox", {
                    {ShaderType::Vertex, shaderBasePath + "skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "skyboxFragment.glsl"}
                }
            },
            {
                "irradiance", {
                    {ShaderType::Vertex, shaderBasePath + "skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "irradianceFragment.glsl"}
                }
            },
            {
                "prefilter", {
                    {ShaderType::Vertex, shaderBasePath + "skyboxVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "prefilterFragment.glsl"}
                }
            },
            {
                "brdf", {
                    {ShaderType::Vertex, shaderBasePath + "brdfVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "brdfFragment.glsl"}
                }
            },

            {
                "histogram", {
                    {ShaderType::Compute, shaderBasePath + "histogramCompute.glsl"}
                }
            },
            {
                "postProcess", {
                    {ShaderType::Vertex, shaderBasePath + "postProcessVertex.glsl"},
                    {ShaderType::Fragment, shaderBasePath + "postProcessFragment.glsl"}
                }
            },
            {
                "bloomPrefilter", {
                    {ShaderType::Compute, shaderBasePath + "bloomPrefilterCompute.glsl"}
                }
            },
            {
                "bloomDownSample", {
                    {ShaderType::Compute, shaderBasePath + "bloomDownSampleCompute.glsl"}
                }
            },
            {
                "bloomUpSample", {
                    {ShaderType::Compute, shaderBasePath + "bloomUpSampleCompute.glsl"}
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
