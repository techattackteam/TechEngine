#include "MeshLoader.hpp"

#include "serialization/BufferStream.hpp"
#include "MaterialLoader.hpp"
#include "TextureLoader.hpp"
#include "core/Logger.hpp"

#include <queue>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace TechEngine {
    // ============================================================================
    //  Helpers
    // ============================================================================

    const char* MeshLoader::atlasTypeName(AtlasType type) {
        static const char* names[] = {"Albedo", "Normal", "Metallic", "Roughness", "AO", "Emission"};
        return names[static_cast<int>(type)];
    }

    UUID MeshLoader::getTexUUID(const MaterialResource& mat, AtlasType type) {
        switch (type) {
            case ATLAS_ALBEDO: return mat.getAlbedoMapUUID();
            case ATLAS_NORMAL: return mat.getNormalMapUUID();
            case ATLAS_METALLIC: return mat.getMetallicMapUUID();
            case ATLAS_ROUGHNESS: return mat.getRoughnessMapUUID();
            case ATLAS_AO: return mat.getAmbientOcclusionMapUUID();
            case ATLAS_EMISSION: return mat.getEmissionMapUUID();
            default: return UUID(-1);
        }
    }

    std::shared_ptr<TextureResource> MeshLoader::findTexture(
        const std::unordered_map<std::string, std::shared_ptr<TextureResource>>& textures,
        const UUID& uuid) const {
        if (uuid == UUID(-1)) return nullptr;
        for (const auto& [name, tex]: textures)
            if (tex && tex->getUUID() == uuid) return tex;
        return m_resourcesSystem.getTextureResource(uuid);
    }

    void MeshLoader::flattenNodeTree(const AssimpLoader::Node& node,
                                     std::vector<FlatSubMesh>& out, const glm::mat4& parentWorld) {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), node.position);
        glm::mat4 R = glm::mat4_cast(glm::quat(glm::radians(node.rotation)));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), node.scale);
        glm::mat4 world = parentWorld * T * R * S;
        for (const auto& mesh: node.meshes)
            out.push_back({&mesh, world});
        for (const auto& child: node.children)
            flattenNodeTree(child, out, world);
    }

    static int nextPow2(int v) {
        if (v <= 1) return 1;
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }

    static uint64_t edgeKey(int a, int b) {
        int lo = std::min(a, b), hi = std::max(a, b);
        return (uint64_t(lo) << 32) | uint64_t(hi);
    }

    static glm::vec3 faceNormal(const std::vector<Vertex>& v, const std::vector<int>& idx, int face) {
        const glm::vec3& p0 = v[idx[face * 3 + 0]].position;
        const glm::vec3& p1 = v[idx[face * 3 + 1]].position;
        const glm::vec3& p2 = v[idx[face * 3 + 2]].position;
        glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        float len = glm::length(n);
        return len > 1e-8f ? n / len : glm::vec3(0, 1, 0);
    }

    static float triangleArea3D(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        return 0.5f * glm::length(glm::cross(b - a, c - a));
    }

    static float signedArea2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
    }

    // Barycentric coordinates of point p in triangle (a,b,c).
    // Returns false if degenerate.
    static bool barycentric(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b,
                            const glm::vec2& c, float& u, float& v, float& w) {
        glm::vec2 v0 = b - a, v1 = c - a, v2 = p - a;
        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;
        if (std::abs(denom) < 1e-12f) return false;
        float inv = 1.0f / denom;
        v = (d11 * d20 - d01 * d21) * inv;
        w = (d00 * d21 - d01 * d20) * inv;
        u = 1.0f - v - w;
        return u >= -1e-4f && v >= -1e-4f && w >= -1e-4f;
    }

    static void sampleTextureCached(const std::vector<unsigned char>& px,
                                    int texW, int texH, int texCh,
                                    float su, float sv, unsigned char* out) {
        su = su - std::floor(su);
        sv = sv - std::floor(sv);
        int x = std::clamp(int(su * texW), 0, texW - 1);
        int y = std::clamp(int(sv * texH), 0, texH - 1);
        const int idx = (y * texW + x) * texCh;
        const unsigned char* src = px.data() + idx;
        switch (texCh) {
            case 4: out[0] = src[0];
                out[1] = src[1];
                out[2] = src[2];
                out[3] = src[3];
                return;
            case 3: out[0] = src[0];
                out[1] = src[1];
                out[2] = src[2];
                out[3] = 255;
                return;
            case 1: out[0] = src[0];
                out[1] = src[0];
                out[2] = src[0];
                out[3] = 255;
                return;
            default:
                for (int c = 0; c < std::min(texCh, 4); c++) out[c] = src[c];
                for (int c = texCh; c < 4; c++) out[c] = 255;
        }
    }

    // ============================================================================
    //  Phase 1: Chart Generation  (region growing with dihedral angle & material)
    // ============================================================================

    static glm::ivec2 faceUVOffset(const std::vector<Vertex>& verts,
                                   const std::vector<int>& indices, int f) {
        glm::vec2 sum(0.0f);
        for (int k = 0; k < 3; k++)
            sum += verts[indices[f * 3 + k]].texCoords;
        return glm::ivec2(glm::floor(sum / 3.0f));
    }

    std::vector<MeshLoader::Chart> MeshLoader::generateCharts(
        const std::vector<Vertex>& verts, const std::vector<int>& indices,
        const std::vector<int>& faceMats, float angleDeg) const {
        const int numFaces = static_cast<int>(indices.size()) / 3;
        const float cosThreshold = std::cos(glm::radians(angleDeg));

        std::unordered_map<uint64_t, std::pair<int, int>> edgeFaces;
        edgeFaces.reserve(numFaces * 3);
        for (int f = 0; f < numFaces; f++) {
            for (int e = 0; e < 3; e++) {
                int v0 = indices[f * 3 + e];
                int v1 = indices[f * 3 + ((e + 1) % 3)];
                uint64_t key = edgeKey(v0, v1);
                auto it = edgeFaces.find(key);
                if (it == edgeFaces.end()) edgeFaces[key] = {f, -1};
                else it->second.second = f;
            }
        }

        std::vector<std::vector<int>> faceAdj(numFaces);
        for (const auto& [key, pair]: edgeFaces) {
            if (pair.second >= 0) {
                faceAdj[pair.first].push_back(pair.second);
                faceAdj[pair.second].push_back(pair.first);
            }
        }

        // Pre-compute per-face UV integer offset (tile cell)
        std::vector<glm::ivec2> faceUVOff(numFaces);
        for (int f = 0; f < numFaces; f++)
            faceUVOff[f] = faceUVOffset(verts, indices, f);

        std::vector<bool> visited(numFaces, false);
        std::vector<Chart> charts;
        charts.reserve(numFaces / 8);

        for (int seed = 0; seed < numFaces; seed++) {
            if (visited[seed]) continue;

            Chart chart;
            chart.materialIdx = faceMats[seed];
            chart.uvOffset = faceUVOff[seed]; // NEW: store canonical tile offset
            std::queue<int> q;
            q.push(seed);
            visited[seed] = true;

            while (!q.empty()) {
                int f = q.front();
                q.pop();
                chart.faces.push_back(f);
                glm::vec3 nf = faceNormal(verts, indices, f);

                for (int adj: faceAdj[f]) {
                    if (visited[adj]) continue;
                    if (faceMats[adj] != chart.materialIdx) continue;
                    // KEY FIX 1: same UV tile cell required
                    if (faceUVOff[adj] != chart.uvOffset) continue;
                    float cosA = glm::dot(nf, faceNormal(verts, indices, adj));
                    if (cosA < cosThreshold) continue;
                    visited[adj] = true;
                    q.push(adj);
                }
            }

            for (int f: chart.faces) {
                for (int k = 0; k < 3; k++) {
                    int gv = indices[f * 3 + k];
                    if (chart.globalToLocal.find(gv) == chart.globalToLocal.end()) {
                        int lv = static_cast<int>(chart.localToGlobal.size());
                        chart.globalToLocal[gv] = lv;
                        chart.localToGlobal.push_back(gv);
                    }
                }
            }
            charts.push_back(std::move(chart));
        }

        TE_LOGGER_INFO("MeshLoader: generated {0} charts from {1} faces",
                       charts.size(), numFaces);
        return charts;
    }

    // ============================================================================
    //  Phase 2a: Parameterization  (PCA-based projection to best-fit 2D plane)
    // ============================================================================

    void MeshLoader::parameterizeChart(Chart& chart,
                                       const std::vector<Vertex>& verts, const std::vector<int>& indices) {
        const int nv = static_cast<int>(chart.localToGlobal.size());
        chart.uvs.resize(nv);
        if (nv == 0) return;

        // Compute area-weighted average normal and centroid
        glm::vec3 centroid(0.0f);
        glm::vec3 avgNormal(0.0f);
        float totalArea = 0.0f;

        for (int f: chart.faces) {
            const glm::vec3& p0 = verts[indices[f * 3 + 0]].position;
            const glm::vec3& p1 = verts[indices[f * 3 + 1]].position;
            const glm::vec3& p2 = verts[indices[f * 3 + 2]].position;
            float a = triangleArea3D(p0, p1, p2);
            glm::vec3 fn = glm::cross(p1 - p0, p2 - p0);
            avgNormal += fn;
            centroid += (p0 + p1 + p2) * (a / 3.0f);
            totalArea += a;
        }
        if (totalArea > 1e-10f)
            centroid /= totalArea;
        float nl = glm::length(avgNormal);
        glm::vec3 N = nl > 1e-8f ? avgNormal / nl : glm::vec3(0, 1, 0);

        // Build tangent frame perpendicular to N
        glm::vec3 up = std::abs(glm::dot(N, glm::vec3(0, 1, 0))) < 0.95f
                           ? glm::vec3(0, 1, 0)
                           : glm::vec3(1, 0, 0);
        glm::vec3 T = glm::normalize(glm::cross(up, N));
        glm::vec3 B = glm::cross(N, T);

        // Project each vertex onto the 2D plane
        for (int lv = 0; lv < nv; lv++) {
            const glm::vec3& pos = verts[chart.localToGlobal[lv]].position;
            glm::vec3 d = pos - centroid;
            chart.uvs[lv] = glm::vec2(glm::dot(d, T), glm::dot(d, B));
        }
    }

    // ============================================================================
    //  Phase 2b: Relaxation  (Laplacian smoothing with flip prevention)
    // ============================================================================

    void MeshLoader::relaxChart(Chart& chart,
                                const std::vector<Vertex>& verts, const std::vector<int>& indices, int iters) {
        const int nv = static_cast<int>(chart.uvs.size());
        if (nv < 3 || chart.faces.size() < 2) return;

        // Build local adjacency: for each local vertex, set of neighbor local vertices
        std::vector<std::unordered_set<int>> adj(nv);
        std::unordered_map<uint64_t, int> localEdgeCount;

        for (int f: chart.faces) {
            int lv[3];
            for (int k = 0; k < 3; k++)
                lv[k] = chart.globalToLocal.at(indices[f * 3 + k]);
            for (int e = 0; e < 3; e++) {
                int a = lv[e], b = lv[(e + 1) % 3];
                adj[a].insert(b);
                adj[b].insert(a);
                localEdgeCount[edgeKey(a, b)]++;
            }
        }

        // Boundary vertices: have at least one edge shared by only 1 face
        std::vector<bool> isBoundary(nv, false);
        for (const auto& [key, count]: localEdgeCount) {
            if (count == 1) {
                isBoundary[int(key >> 32)] = true;
                isBoundary[int(key & 0xFFFFFFFF)] = true;
            }
        }

        // Iterative Laplacian relaxation for interior vertices only
        std::vector<glm::vec2> newUVs(nv);
        for (int iter = 0; iter < iters; iter++) {
            newUVs = chart.uvs;
            for (int v = 0; v < nv; v++) {
                if (isBoundary[v] || adj[v].empty()) continue;
                glm::vec2 avg(0.0f);
                float tw = 0.0f;
                for (int nb: adj[v]) {
                    float w = 1.0f / std::max(1e-6f,
                                              glm::distance(verts[chart.localToGlobal[v]].position,
                                                            verts[chart.localToGlobal[nb]].position));
                    avg += w * chart.uvs[nb];
                    tw += w;
                }
                if (tw > 1e-8f) avg /= tw;
                newUVs[v] = glm::mix(chart.uvs[v], avg, 0.5f);
            }

            // Check for triangle flips; if any, reject this iteration
            bool anyFlip = false;
            for (int f: chart.faces) {
                int l0 = chart.globalToLocal.at(indices[f * 3 + 0]);
                int l1 = chart.globalToLocal.at(indices[f * 3 + 1]);
                int l2 = chart.globalToLocal.at(indices[f * 3 + 2]);
                if (signedArea2D(newUVs[l0], newUVs[l1], newUVs[l2]) <= 0.0f) {
                    anyFlip = true;
                    break;
                }
            }
            if (!anyFlip)
                chart.uvs = newUVs;
        }

        // Ensure consistent winding (all positive signed area)
        float totalSA = 0.0f;
        for (int f: chart.faces) {
            int l0 = chart.globalToLocal.at(indices[f * 3 + 0]);
            int l1 = chart.globalToLocal.at(indices[f * 3 + 1]);
            int l2 = chart.globalToLocal.at(indices[f * 3 + 2]);
            totalSA += signedArea2D(chart.uvs[l0], chart.uvs[l1], chart.uvs[l2]);
        }
        if (totalSA < 0.0f) {
            for (auto& uv: chart.uvs) uv.x = -uv.x;
        }
    }

    // ============================================================================
    //  Chart sizing  (match original texture's texel density)
    // ============================================================================

    void MeshLoader::computeChartSizes(std::vector<Chart>& charts,
                                       const std::vector<Vertex>& verts, const std::vector<int>& indices,
                                       const std::vector<int>& faceMats, const std::vector<std::string>& matOrder,
                                       const AssimpLoader::SceneData& scene) const {
        for (auto& chart: charts) {
            // Compute bounding box
            chart.bbMin = chart.uvs[0];
            chart.bbMax = chart.uvs[0];
            for (const auto& uv: chart.uvs) {
                chart.bbMin = glm::min(chart.bbMin, uv);
                chart.bbMax = glm::max(chart.bbMax, uv);
            }

            // Estimate texel density from original texture/UV mapping
            float density = 256.0f;
            if (!chart.faces.empty() && chart.materialIdx < (int)matOrder.size()) {
                auto matIt = scene.materials.find(matOrder[chart.materialIdx]);
                if (matIt != scene.materials.end()) {
                    int texW = 1024;
                    for (int t = 0; t < ATLAS_TYPE_COUNT; t++) {
                        auto tex = findTexture(scene.textures, getTexUUID(*matIt->second, static_cast<AtlasType>(t)));
                        if (tex) {
                            texW = std::max(texW, tex->getWidth());
                            break;
                        }
                    }

                    float sumRatio = 0.0f;
                    int counted = 0;
                    for (int f: chart.faces) {
                        int i0 = indices[f * 3 + 0], i1 = indices[f * 3 + 1];
                        float len3D = glm::distance(verts[i0].position, verts[i1].position);
                        glm::vec2 tc0 = verts[i0].texCoords - glm::vec2(chart.uvOffset); // KEY FIX 3
                        glm::vec2 tc1 = verts[i1].texCoords - glm::vec2(chart.uvOffset);
                        float lenUV = glm::distance(tc0, tc1);
                        if (len3D > 1e-6f && lenUV > 1e-6f) {
                            sumRatio += (lenUV * texW) / len3D;
                            counted++;
                        }
                        if (counted >= 8) break;
                    }
                    if (counted > 0) density = sumRatio / counted;
                }
            }
            chart.texelDensity = density;

            glm::vec2 bbSize = chart.bbMax - chart.bbMin;
            chart.pixW = std::clamp(int(std::ceil(bbSize.x * density)) + 2 * ATLAS_PADDING, 4, 1024);
            chart.pixH = std::clamp(int(std::ceil(bbSize.y * density)) + 2 * ATLAS_PADDING, 4, 1024);
        }

        // Global density scaling: if total chart area exceeds atlas budget, scale all
        // charts down uniformly so they fit.  Target 75% fill to leave room for
        // packing waste.
        int64_t totalArea = 0;
        for (const auto& c: charts)
            totalArea += int64_t(c.pixW) * c.pixH;

        int64_t budget = int64_t(MAX_ATLAS_SIZE) * MAX_ATLAS_SIZE;
        float fillTarget = 0.75f;
        if (totalArea > int64_t(budget * fillTarget)) {
            float scale = std::sqrt(float(budget * fillTarget) / float(totalArea));
            TE_LOGGER_INFO("MeshLoader: scaling chart density by {0:.2f} to fit atlas", scale);
            for (auto& c: charts) {
                c.pixW = std::max(4, int(c.pixW * scale));
                c.pixH = std::max(4, int(c.pixH * scale));
            }
        }
    }

    // ============================================================================
    //  Phase 3: Chart Packing  (MaxRects Best-Short-Side-Fit with 90° rotation)
    // ============================================================================

    bool MeshLoader::packCharts(std::vector<Chart>& charts,
                                int maxSize, int padding, int& outW, int& outH) {
        if (charts.empty()) {
            outW = outH = 1;
            return true;
        }

        // Sort largest max-side first — do this ONCE, outside the retry loop.
        std::vector<size_t> order(charts.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            return std::max(charts[a].pixW, charts[a].pixH) >
                   std::max(charts[b].pixW, charts[b].pixH);
        });

        struct FreeRect {
            int x, y, w, h;
        };

        // Compute the minimum viable atlas size from the largest single chart.
        int maxChart = 0;
        for (const auto& c: charts)
            maxChart = std::max(maxChart, std::max(c.pixW, c.pixH));

        // Also compute a lower bound from total area so we skip obviously-too-small sizes.
        int64_t totalPixels = 0;
        for (const auto& c: charts)
            totalPixels += int64_t(c.pixW) * c.pixH;
        int areaLB = nextPow2(int(std::ceil(std::sqrt(double(totalPixels)))));

        int startSize = std::max(nextPow2(maxChart), areaLB);
        startSize = std::min(startSize, maxSize);

        // Snapshot original orientations so retries start clean.
        struct Snapshot {
            int pixW, pixH;
            std::vector<glm::vec2> uvs;
            glm::vec2 bbMin, bbMax;
        };
        std::vector<Snapshot> snapshots(charts.size());
        for (size_t i = 0; i < charts.size(); i++)
            snapshots[i] = {charts[i].pixW, charts[i].pixH, charts[i].uvs, charts[i].bbMin, charts[i].bbMax};

        auto restoreSnapshots = [&]() {
            for (size_t i = 0; i < charts.size(); i++) {
                charts[i].pixW = snapshots[i].pixW;
                charts[i].pixH = snapshots[i].pixH;
                charts[i].uvs = snapshots[i].uvs;
                charts[i].bbMin = snapshots[i].bbMin;
                charts[i].bbMax = snapshots[i].bbMax;
            }
        };

        // Prune rects fully contained within another — O(N) with sorted sweep.
        // After splitting we typically have <200 rects; sort by area descending
        // and do a single forward pass: a smaller rect can only be contained in a
        // larger one that appeared earlier in the sorted list.
        auto pruneContained = [](std::vector<FreeRect>& rects) {
            // Sort largest-area first so containment only needs to check predecessors.
            std::sort(rects.begin(), rects.end(), [](const FreeRect& a, const FreeRect& b) {
                return (a.w * a.h) > (b.w * b.h);
            });
            std::vector<FreeRect> out;
            out.reserve(rects.size());
            for (size_t i = 0; i < rects.size(); i++) {
                bool contained = false;
                for (size_t j = 0; j < i; j++) { // only check larger rects
                    if (rects[i].x >= rects[j].x && rects[i].y >= rects[j].y &&
                        rects[i].x + rects[i].w <= rects[j].x + rects[j].w &&
                        rects[i].y + rects[i].h <= rects[j].y + rects[j].h) {
                        contained = true;
                        break;
                    }
                }
                if (!contained) out.push_back(rects[i]);
            }
            rects = std::move(out);
        };

        for (int atlasSize = startSize; atlasSize <= maxSize; atlasSize *= 2) {
            restoreSnapshots();

            std::vector<FreeRect> freeRects;
            freeRects.reserve(charts.size() * 4); // MaxRects produces at most 4 per placement
            freeRects.push_back({0, 0, atlasSize, atlasSize});

            bool allFit = true;

            for (size_t ci: order) {
                auto& chart = charts[ci];
                int rw = chart.pixW, rh = chart.pixH;

                int bestIdx = -1, bestSS = INT_MAX, bestLS = INT_MAX;
                bool bestRotated = false;

                // BSSF over both orientations — unchanged logic, same O(freeRects) cost,
                // but freeRects stays small because we prune aggressively after each place.
                for (int rot = 0; rot < 2; rot++) {
                    // Skip duplicate orientation check when chart is square.
                    if (rot == 1 && rw == rh) break;
                    int cw = (rot == 0) ? rw : rh;
                    int ch = (rot == 0) ? rh : rw;
                    for (int i = 0; i < (int)freeRects.size(); i++) {
                        const auto& fr = freeRects[i];
                        if (cw <= fr.w && ch <= fr.h) {
                            int ss = std::min(fr.w - cw, fr.h - ch);
                            int ls = std::max(fr.w - cw, fr.h - ch);
                            if (ss < bestSS || (ss == bestSS && ls < bestLS)) {
                                bestIdx = i;
                                bestSS = ss;
                                bestLS = ls;
                                bestRotated = (rot == 1);
                            }
                        }
                    }
                }

                if (bestIdx < 0) {
                    allFit = false;
                    break;
                }

                int cw = bestRotated ? rh : rw;
                int ch = bestRotated ? rw : rh;
                chart.packX = freeRects[bestIdx].x;
                chart.packY = freeRects[bestIdx].y;

                if (bestRotated) {
                    std::swap(chart.pixW, chart.pixH);
                    for (auto& uv: chart.uvs) uv = glm::vec2(uv.y, uv.x);
                    std::swap(chart.bbMin.x, chart.bbMin.y);
                    std::swap(chart.bbMax.x, chart.bbMax.y);
                }

                FreeRect placed = {chart.packX, chart.packY, cw, ch};

                // Split all overlapping free rects, collecting new candidates.
                std::vector<FreeRect> newFree;
                newFree.reserve(freeRects.size() * 2);

                for (const auto& fr: freeRects) {
                    // No overlap — keep as-is.
                    if (placed.x >= fr.x + fr.w || placed.x + placed.w <= fr.x ||
                        placed.y >= fr.y + fr.h || placed.y + placed.h <= fr.y) {
                        newFree.push_back(fr);
                        continue;
                    }
                    // Four candidate sub-rects from the split.
                    if (placed.x > fr.x)
                        newFree.push_back({fr.x, fr.y, placed.x - fr.x, fr.h});
                    if (placed.x + placed.w < fr.x + fr.w)
                        newFree.push_back({
                            placed.x + placed.w, fr.y,
                            (fr.x + fr.w) - (placed.x + placed.w), fr.h
                        });
                    if (placed.y > fr.y)
                        newFree.push_back({fr.x, fr.y, fr.w, placed.y - fr.y});
                    if (placed.y + placed.h < fr.y + fr.h)
                        newFree.push_back({
                            fr.x, placed.y + placed.h,
                            fr.w, (fr.y + fr.h) - (placed.y + placed.h)
                        });
                }

                pruneContained(newFree); // O(N log N + N²) but N stays small
                freeRects = std::move(newFree);
            }

            if (allFit) {
                int usedH = 0;
                for (const auto& c: charts)
                    usedH = std::max(usedH, c.packY + c.pixH);
                outW = atlasSize;
                outH = nextPow2(usedH);
                return true;
            }
        }

        restoreSnapshots();
        outW = outH = maxSize;
        TE_LOGGER_WARN("MeshLoader: charts don't fit in {0}x{0} atlas", maxSize);
        return false;
    }

    // ============================================================================
    //  Phase 4: Atlas Rasterization  (software triangle rasterizer + texture transfer)
    // ============================================================================

    void MeshLoader::rasterizeAtlas(
        std::vector<unsigned char>& pixels, int atlasW, int atlasH,
        const std::vector<Chart>& charts,
        const std::vector<Vertex>& verts, const std::vector<int>& indices,
        const std::vector<std::string>& matOrder,
        const AssimpLoader::SceneData& scene, AtlasType type) const {
        pixels.assign(static_cast<size_t>(atlasW) * atlasH * ATLAS_CHANNELS, 0);

        unsigned char defCol[4];
        switch (type) {
            case ATLAS_ALBEDO: defCol[0] = 255;
                defCol[1] = 255;
                defCol[2] = 255;
                defCol[3] = 255;
                break;
            case ATLAS_NORMAL: defCol[0] = 128;
                defCol[1] = 128;
                defCol[2] = 255;
                defCol[3] = 255;
                break;
            case ATLAS_METALLIC: defCol[0] = 0;
                defCol[1] = 0;
                defCol[2] = 0;
                defCol[3] = 255;
                break;
            case ATLAS_ROUGHNESS: defCol[0] = 128;
                defCol[1] = 128;
                defCol[2] = 128;
                defCol[3] = 255;
                break;
            case ATLAS_AO: defCol[0] = 255;
                defCol[1] = 255;
                defCol[2] = 255;
                defCol[3] = 255;
                break;
            case ATLAS_EMISSION: defCol[0] = 0;
                defCol[1] = 0;
                defCol[2] = 0;
                defCol[3] = 255;
                break;
            default: defCol[0] = 255;
                defCol[1] = 255;
                defCol[2] = 255;
                defCol[3] = 255;
                break;
        }

        // Per-material texture cache: copy each texture's pixels only once
        struct CachedTex {
            std::vector<unsigned char> px;
            int w = 0, h = 0, ch = 0;
            bool valid = false;
        };
        std::unordered_map<int, CachedTex> texCache;
        for (const auto& chart: charts) {
            if (texCache.count(chart.materialIdx)) continue;
            CachedTex ct;
            if (chart.materialIdx < (int)matOrder.size()) {
                auto matIt = scene.materials.find(matOrder[chart.materialIdx]);
                if (matIt != scene.materials.end()) {
                    auto tex = findTexture(scene.textures, getTexUUID(*matIt->second, type));
                    if (tex && tex->hasPixelData()) {
                        ct.px = tex->getPixelsChar();
                        ct.w = tex->getWidth();
                        ct.h = tex->getHeight();
                        ct.ch = tex->getChannels();
                        ct.valid = !ct.px.empty();
                    }
                }
            }
            texCache[chart.materialIdx] = std::move(ct);
        }

        unsigned char* atlasPtr = pixels.data();

        for (const auto& chart: charts) {
            const CachedTex& ct = texCache[chart.materialIdx];

            glm::vec2 bbSize = chart.bbMax - chart.bbMin;
            float innerW = std::max(1.0f, float(chart.pixW - 2 * ATLAS_PADDING));
            float innerH = std::max(1.0f, float(chart.pixH - 2 * ATLAS_PADDING));
            float invBBx = (bbSize.x > 1e-8f) ? 1.0f / bbSize.x : 0.0f;
            float invBBy = (bbSize.y > 1e-8f) ? 1.0f / bbSize.y : 0.0f;
            float offX = float(chart.packX + ATLAS_PADDING);
            float offY = float(chart.packY + ATLAS_PADDING);

            for (int f: chart.faces) {
                int gv0 = indices[f * 3 + 0], gv1 = indices[f * 3 + 1], gv2 = indices[f * 3 + 2];
                int lv0 = chart.globalToLocal.at(gv0);
                int lv1 = chart.globalToLocal.at(gv1);
                int lv2 = chart.globalToLocal.at(gv2);

                // Original UVs for texture sampling
                glm::vec2 tileOff = glm::vec2(chart.uvOffset); // KEY FIX 2
                glm::vec2 ouv0 = verts[gv0].texCoords - tileOff;
                glm::vec2 ouv1 = verts[gv1].texCoords - tileOff;
                glm::vec2 ouv2 = verts[gv2].texCoords - tileOff;

                // Atlas-space triangle coordinates (in pixels)
                float nu0 = (chart.uvs[lv0].x - chart.bbMin.x) * invBBx;
                float nv0 = (chart.uvs[lv0].y - chart.bbMin.y) * invBBy;
                float nu1 = (chart.uvs[lv1].x - chart.bbMin.x) * invBBx;
                float nv1 = (chart.uvs[lv1].y - chart.bbMin.y) * invBBy;
                float nu2 = (chart.uvs[lv2].x - chart.bbMin.x) * invBBx;
                float nv2 = (chart.uvs[lv2].y - chart.bbMin.y) * invBBy;

                float ax = offX + nu0 * innerW, ay = offY + nv0 * innerH;
                float bx = offX + nu1 * innerW, by = offY + nv1 * innerH;
                float cx = offX + nu2 * innerW, cy = offY + nv2 * innerH;

                // Pre-compute 2x2 inverse for barycentric interpolation
                float dx0 = bx - ax, dy0 = by - ay;
                float dx1 = cx - ax, dy1 = cy - ay;
                float det = dx0 * dy1 - dx1 * dy0;
                if (std::abs(det) < 1e-6f) continue;
                float invDet = 1.0f / det;

                int x0 = std::max(0, int(std::floor(std::min({ax, bx, cx}))));
                int x1 = std::min(atlasW - 1, int(std::ceil(std::max({ax, bx, cx}))));
                int y0 = std::max(0, int(std::floor(std::min({ay, by, cy}))));
                int y1 = std::min(atlasH - 1, int(std::ceil(std::max({ay, by, cy}))));

                for (int py = y0; py <= y1; py++) {
                    float fy = py + 0.5f - ay;
                    int rowOff = py * atlasW;
                    for (int px = x0; px <= x1; px++) {
                        float fx = px + 0.5f - ax;

                        // Inline barycentric via pre-computed inverse
                        float bv = (dy1 * fx - dx1 * fy) * invDet;
                        float bw = (-dy0 * fx + dx0 * fy) * invDet;
                        float bu = 1.0f - bv - bw;
                        if (bu < -1e-4f || bv < -1e-4f || bw < -1e-4f) continue;

                        int di = (rowOff + px) * ATLAS_CHANNELS;
                        if (ct.valid) {
                            float su = bu * ouv0.x + bv * ouv1.x + bw * ouv2.x;
                            float sv = bu * ouv0.y + bv * ouv1.y + bw * ouv2.y;
                            sampleTextureCached(ct.px, ct.w, ct.h, ct.ch, su, sv,
                                                atlasPtr + di);
                        } else {
                            atlasPtr[di + 0] = defCol[0];
                            atlasPtr[di + 1] = defCol[1];
                            atlasPtr[di + 2] = defCol[2];
                            atlasPtr[di + 3] = defCol[3];
                        }
                    }
                }
            }
        }
    }

    // ============================================================================
    //  Constructor & public interface
    // ============================================================================

    MeshLoader::MeshLoader(ResourceSystem& resourceSystem,
                           FileSystem& fileSystem,
                           TextureLoader& textureLoader,
                           MaterialLoader& materialLoader)
        : m_fileSystem(fileSystem),
          m_assimpLoader(resourceSystem, fileSystem, textureLoader),
          m_resourcesSystem(resourceSystem),
          m_materialLoader(materialLoader),
          m_textureLoader(textureLoader) {
    }

    void MeshLoader::importMeshFile(const std::filesystem::path& virtualPath) {
        const std::string modelName = m_fileSystem.status(virtualPath).name.string();
        const std::filesystem::path parent = m_fileSystem.getParentPath(virtualPath);

        if (m_resourcesSystem.isMeshRegistered(modelName)) return;

        auto [meshResource, mergedMaterial, atlasTextures] =
                loadMeshFromSourceFile(virtualPath);

        if (!meshResource) return;

        for (const auto& atlas: atlasTextures) {
            if (!atlas) continue;
            m_resourcesSystem.registerTextureResource(atlas);
            Buffer b;
            m_textureLoader.serializeTextureResource(b, atlas);
            m_fileSystem.write(
                parent / (atlas->getName() + m_textureLoader.resourceExtension()), b);
        }
        m_resourcesSystem.registerMaterialResource(mergedMaterial);
        Buffer buffer;
        m_materialLoader.serializeMaterialResource(buffer, mergedMaterial);
        m_fileSystem.write(
            parent / (modelName + m_materialLoader.resourceExtension()), buffer);
        m_resourcesSystem.registerMeshResource(meshResource);
        buffer.release();
        serializeMeshResource(buffer, meshResource);
        m_fileSystem.write(parent / (modelName + resourceExtension()), buffer);
    }

    // ============================================================================
    //  Main pipeline
    // ============================================================================

    std::tuple<std::shared_ptr<MeshResource>,
        std::shared_ptr<MaterialResource>,
        std::vector<std::shared_ptr<TextureResource>>>
    MeshLoader::loadMeshFromSourceFile(const std::filesystem::path& virtualPath) {
        AssimpLoader::SceneData sceneData = m_assimpLoader.loadSourceFile(virtualPath);
        const std::string modelName = m_fileSystem.status(virtualPath).name.string();

        // --- 1. Flatten node tree & merge all sub-meshes ---
        std::vector<FlatSubMesh> flatMeshes;
        flattenNodeTree(sceneData.rootNode, flatMeshes);
        if (flatMeshes.empty()) {
            TE_LOGGER_WARN("MeshLoader: no meshes in '{0}'", virtualPath.string());
            return {nullptr, nullptr, {}};
        }

        std::vector<Vertex> allVerts;
        std::vector<int> allIndices;
        std::vector<int> faceMaterials;
        std::vector<std::string> materialOrder;
        std::unordered_map<std::string, size_t> matIndex;

        for (const auto& sm: flatMeshes) {
            if (matIndex.find(sm.data->material) == matIndex.end()) {
                matIndex[sm.data->material] = materialOrder.size();
                materialOrder.push_back(sm.data->material);
            }
            int mi = static_cast<int>(matIndex[sm.data->material]);
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(sm.worldTransform)));
            int baseV = static_cast<int>(allVerts.size());

            for (const auto& v: sm.data->vertices) {
                Vertex xv;
                xv.position = glm::vec3(sm.worldTransform * glm::vec4(v.position, 1.0f));
                xv.normal = glm::normalize(normalMatrix * v.normal);
                xv.texCoords = v.texCoords;
                allVerts.push_back(xv);
            }
            for (size_t i = 0; i < sm.data->indices.size(); i += 3) {
                allIndices.push_back(sm.data->indices[i] + baseV);
                allIndices.push_back(sm.data->indices[i + 1] + baseV);
                allIndices.push_back(sm.data->indices[i + 2] + baseV);
                faceMaterials.push_back(mi);
            }
        }

        TE_LOGGER_INFO("MeshLoader: merged input {0} verts, {1} tris, {2} materials",
                       allVerts.size(), faceMaterials.size(), materialOrder.size());

        // --- 2. Phase 1: Chart generation ---
        std::vector<Chart> charts = generateCharts(allVerts, allIndices, faceMaterials, CHART_ANGLE_THRESHOLD);

        // --- 3. Phase 2: Parameterize & relax each chart ---
        for (auto& chart: charts) {
            parameterizeChart(chart, allVerts, allIndices);
            relaxChart(chart, allVerts, allIndices, RELAX_ITERATIONS);
        }

        // --- 4. Compute chart pixel sizes based on original texture density ---
        computeChartSizes(charts, allVerts, allIndices, faceMaterials, materialOrder, sceneData);

        // --- 5. Phase 3: Pack charts into atlas ---
        int atlasW = 1, atlasH = 1;
        packCharts(charts, MAX_ATLAS_SIZE, ATLAS_PADDING, atlasW, atlasH);

        TE_LOGGER_INFO("MeshLoader: atlas {0}x{1}, {2} charts for '{3}'",
                       atlasW, atlasH, charts.size(), modelName);

        // --- 6. Build final mesh with atlas UVs ---
        std::vector<Vertex> finalVerts;
        std::vector<int> finalIndices;

        float invW = 1.0f / static_cast<float>(atlasW);
        float invH = 1.0f / static_cast<float>(atlasH);

        for (const auto& chart: charts) {
            glm::vec2 bbSize = chart.bbMax - chart.bbMin;
            float innerW = static_cast<float>(chart.pixW - 2 * ATLAS_PADDING);
            float innerH = static_cast<float>(chart.pixH - 2 * ATLAS_PADDING);
            if (innerW < 1.0f) innerW = 1.0f;
            if (innerH < 1.0f) innerH = 1.0f;

            std::unordered_map<int, int> localToFinal;

            for (int f: chart.faces) {
                int tri[3];
                for (int k = 0; k < 3; k++) {
                    int gv = allIndices[f * 3 + k];
                    int lv = chart.globalToLocal.at(gv);

                    auto it = localToFinal.find(lv);
                    if (it == localToFinal.end()) {
                        Vertex fv;
                        fv.position = allVerts[gv].position;
                        fv.normal = allVerts[gv].normal;

                        float nu = (bbSize.x > 1e-8f) ? (chart.uvs[lv].x - chart.bbMin.x) / bbSize.x : 0.5f;
                        float nv = (bbSize.y > 1e-8f) ? (chart.uvs[lv].y - chart.bbMin.y) / bbSize.y : 0.5f;
                        fv.texCoords.x = (chart.packX + ATLAS_PADDING + nu * innerW) * invW;
                        fv.texCoords.y = (chart.packY + ATLAS_PADDING + nv * innerH) * invH;

                        int fi = static_cast<int>(finalVerts.size());
                        localToFinal[lv] = fi;
                        finalVerts.push_back(fv);
                        tri[k] = fi;
                    } else {
                        tri[k] = it->second;
                    }
                }
                finalIndices.push_back(tri[0]);
                finalIndices.push_back(tri[1]);
                finalIndices.push_back(tri[2]);
            }
        }

        // --- 7. Phase 4: Rasterize atlas textures ---
        std::vector<std::shared_ptr<TextureResource>> atlasTextures;
        auto mergedMaterial = std::make_shared<MaterialResource>(modelName);

        // Detect which atlas types have real textures
        std::array<bool, ATLAS_TYPE_COUNT> hasTexType{};
        for (const auto& [name, mat]: sceneData.materials) {
            for (int t = 0; t < ATLAS_TYPE_COUNT; t++) {
                if (findTexture(sceneData.textures, getTexUUID(*mat, static_cast<AtlasType>(t))))
                    hasTexType[t] = true;
            }
        }

        for (int t = 0; t < ATLAS_TYPE_COUNT; t++) {
            if (!hasTexType[t]) continue;

            AtlasType atlasType = static_cast<AtlasType>(t);
            std::string atlasName = modelName + "_" + atlasTypeName(atlasType);

            std::vector<unsigned char> pixels;
            rasterizeAtlas(pixels, atlasW, atlasH, charts, allVerts, allIndices,
                           materialOrder, sceneData, atlasType);

            auto atlasRes = std::make_shared<TextureResource>(
                atlasName, atlasW, atlasH, ATLAS_CHANNELS,
                PNG, CLAMP_TO_EDGE, 1.0f, 1.0f, std::move(pixels));

            switch (atlasType) {
                case ATLAS_ALBEDO: mergedMaterial->setAlbedoMapID(atlasRes->getUUID());
                    break;
                case ATLAS_NORMAL: mergedMaterial->setNormalMapID(atlasRes->getUUID());
                    break;
                case ATLAS_METALLIC: mergedMaterial->setMetallicMapID(atlasRes->getUUID());
                    break;
                case ATLAS_ROUGHNESS: mergedMaterial->setRoughnessMapID(atlasRes->getUUID());
                    break;
                case ATLAS_AO: mergedMaterial->setAmbientOcclusionMapID(atlasRes->getUUID());
                    break;
                case ATLAS_EMISSION: mergedMaterial->setEmissionMapID(atlasRes->getUUID());
                    break;
                default: break;
            }
            atlasTextures.push_back(std::move(atlasRes));
        }

        // Average scalar material properties
        {
            glm::vec4 albedoSum(0.0f);
            float metallicSum = 0.0f, roughnessSum = 0.0f;
            int counted = 0;
            for (const auto& [name, mat]: sceneData.materials) {
                if (matIndex.find(name) == matIndex.end()) continue;
                albedoSum += mat->getAlbedo();
                metallicSum += mat->getMetallic();
                roughnessSum += mat->getRoughness();
                counted++;
            }
            if (counted > 0) {
                float inv = 1.0f / static_cast<float>(counted);
                mergedMaterial->setAlbedo(albedoSum * inv);
                mergedMaterial->setMetallic(metallicSum * inv);
                mergedMaterial->setRoughness(roughnessSum * inv);
            }
        }

        auto meshResource = std::make_shared<MeshResource>(
            modelName, std::move(finalVerts), std::move(finalIndices));

        TE_LOGGER_INFO("MeshLoader: '{0}' -> {1} verts, {2} indices, {3} atlases ({4}x{5})",
                       modelName, meshResource->getVertices().size(),
                       meshResource->getIndices().size(), atlasTextures.size(), atlasW, atlasH);

        return {meshResource, mergedMaterial, atlasTextures};
    }

    // ============================================================================
    //  Serialization (unchanged)
    // ============================================================================

    void MeshLoader::serializeMeshResource(Buffer& buffer, const std::string& meshName) {
        serializeMeshResource(buffer, m_resourcesSystem.getMeshResource(meshName));
    }

    void MeshLoader::serializeMeshResource(Buffer& buffer, const std::shared_ptr<MeshResource>& mesh) {
        if (mesh) {
            BufferStreamWriter sw(buffer, 0);
            MeshResource::serialize(&sw, *mesh);
        }
    }

    std::shared_ptr<MeshResource> MeshLoader::deserializeMeshResource(Buffer& buffer) const {
        BufferStreamReader sr(buffer, 0);
        std::shared_ptr<MeshResource> mesh = MeshResource::deserialize(&sr);
        m_resourcesSystem.registerMeshResource(mesh);
        return mesh;
    }

    std::vector<std::string> MeshLoader::sourceExtensions() const {
        return {".obj", ".fbx", ".gltf", ".glb"};
    }

    std::string MeshLoader::resourceExtension() const {
        return ".temesh";
    }
} // namespace TechEngine
