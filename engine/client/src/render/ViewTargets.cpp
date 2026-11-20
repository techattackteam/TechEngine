#include <render/ViewTargets.hpp>
#include <render/device/RenderDevice.hpp>

#include <cstddef>

namespace TechEngine {
    void ViewTargets::initialize(RenderDevice& device) {
        m_device = &device;
    }

    std::error_code ViewTargets::update(const std::span<const ViewRequest> requests) {
        for (std::size_t index = 0; index < m_targets.size();) {
            const ViewTarget& existing = m_targets[index];
            bool keep = false;
            for (const ViewRequest& request: requests) {
                if (request.token == existing.token && request.token != 0 && request.width > 0 && request.height > 0) {
                    keep = true;
                    break;
                }
            }
            if (keep) {
                index++;
                continue;
            }
            destroyTarget(m_targets[index]);
            m_targets.erase(m_targets.begin() + static_cast<std::ptrdiff_t>(index));
        }

        std::error_code firstError;
        for (const ViewRequest& request: requests) {
            if (request.token == 0 || request.width <= 0 || request.height <= 0) {
                continue;
            }
            const RenderExtent extent{request.width, request.height};
            std::size_t targetIndex = m_targets.size();
            for (std::size_t index = 0; index < m_targets.size(); index++) {
                ViewTarget& existing = m_targets[index];
                if (existing.token == request.token) {
                    targetIndex = index;
                    break;
                }
            }

            if (targetIndex == m_targets.size()) {
                ViewTarget target;
                const std::error_code createError = createTarget(request.token, extent, target);
                if (createError) {
                    if (!firstError) {
                        firstError = createError;
                    }
                    continue;
                }
                m_targets.push_back(target);
            } else {
                ViewTarget& target = m_targets[targetIndex];
                if (target.extent.height == extent.height && target.extent.width == extent.width) {
                    continue;
                }
                destroyTarget(target);
                ViewTarget resized;
                const std::error_code resizeError = createTarget(request.token, extent, resized);
                if (resizeError) {
                    m_targets.erase(m_targets.begin() + static_cast<std::ptrdiff_t>(targetIndex));
                    if (!firstError) {
                        firstError = resizeError;
                    }
                    continue;
                }
                target = resized;
            }
        }

        return firstError;
    }

    const ViewTarget* ViewTargets::findTarget(const std::uint64_t token) const {
        for (const ViewTarget& target: m_targets) {
            if (target.token == token) {
                return &target;
            }
        }
        return nullptr;
    }

    void ViewTargets::collectImGuiTextures(std::vector<ImGuiViewTexture>& out) const {
        out.clear();
        if (m_device == nullptr) {
            return;
        }
        for (const ViewTarget& target: m_targets) {
            out.push_back(ImGuiViewTexture{
                .token = target.token,
                .nativeTexture = m_device->getNativeTexture(target.color),
            });
        }
    }

    void ViewTargets::shutdown() {
        if (m_device == nullptr) {
            return;
        }
        for (ViewTarget& target: m_targets) {
            destroyTarget(target);
        }
        m_targets.clear();
        m_device = nullptr;
    }

    std::error_code ViewTargets::createTarget(const std::uint64_t token, const RenderExtent& extent, ViewTarget& out) const {
        out = ViewTarget{};
        const std::error_code colorError = m_device->createTexture(TextureDescription{extent.width, extent.height, 1, GpuTextureFormat::Rgba8}, out.color);
        if (colorError) {
            return colorError;
        }
        const std::error_code depthError = m_device->createTexture(TextureDescription{extent.width, extent.height, 1, GpuTextureFormat::Depth32F}, out.depth);
        if (depthError) {
            destroyTarget(out);
            return depthError;
        }
        const std::error_code targetError = m_device->createRenderTarget(RenderTargetDescription{out.color, out.depth}, out.target);
        if (targetError) {
            destroyTarget(out);
            return targetError;
        }
        out.token = token;
        out.extent = extent;
        return {};
    }

    void ViewTargets::destroyTarget(ViewTarget& target) const {
        if (target.target.valid()) {
            m_device->destroyRenderTarget(target.target);
        }
        if (target.depth.valid()) {
            m_device->destroyTexture(target.depth);
        }
        if (target.color.valid()) {
            m_device->destroyTexture(target.color);
        }
        target = ViewTarget{};
    }
}
