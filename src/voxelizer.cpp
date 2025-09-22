#include "voxelizer.h"
#include <queue>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>

namespace obj2blocks {
    Voxelizer::Voxelizer(double voxel_size) : voxel_size_(voxel_size) {
    }

    Voxelizer::~Voxelizer() {
    }

    Vec3i Voxelizer::pointToVoxel(const pmp::Point&point) const {
        return Vec3i(
            static_cast<int>(std::floor(point[0] / voxel_size_)),
            static_cast<int>(std::floor(point[1] / voxel_size_)),
            static_cast<int>(std::floor(point[2] / voxel_size_))
        );
    }

    std::set<Vec3i> Voxelizer::voxelize(pmp::SurfaceMesh&mesh, bool solid) {
        std::cout << "Starting voxelization with voxel size: " << voxel_size_ << std::endl;

        std::set<Vec3i> surface_voxels = voxelizeSurface(mesh);
        std::cout << "Surface voxels: " << surface_voxels.size() << std::endl;

        if (solid) {
            std::set<Vec3i> filled_voxels = fillInterior(surface_voxels);
            std::cout << "Total voxels after filling: " << filled_voxels.size() << std::endl;
            return filled_voxels;
        }

        return surface_voxels;
    }

    std::set<Vec3i> Voxelizer::voxelizeSurface(pmp::SurfaceMesh&mesh) {
        std::set<Vec3i> voxels;

        auto points = mesh.vertex_property<pmp::Point>("v:point");

        for (auto f: mesh.faces()) {
            std::vector<pmp::Point> vertices;
            for (auto v: mesh.vertices(f)) {
                vertices.push_back(points[v]);
            }

            if (vertices.size() == 3) {
                rasterizeTriangle(vertices[0], vertices[1], vertices[2], voxels);
            }
        }

        return voxels;
    }

    void Voxelizer::rasterizeTriangle(const pmp::Point&v0, const pmp::Point&v1,
                                      const pmp::Point&v2, std::set<Vec3i>&voxels) {
        Vec3i voxel0 = pointToVoxel(v0);
        Vec3i voxel1 = pointToVoxel(v1);
        Vec3i voxel2 = pointToVoxel(v2);

        int min_x = std::min({voxel0.x, voxel1.x, voxel2.x});
        int max_x = std::max({voxel0.x, voxel1.x, voxel2.x});
        int min_y = std::min({voxel0.y, voxel1.y, voxel2.y});
        int max_y = std::max({voxel0.y, voxel1.y, voxel2.y});
        int min_z = std::min({voxel0.z, voxel1.z, voxel2.z});
        int max_z = std::max({voxel0.z, voxel1.z, voxel2.z});

        pmp::Point edge1 = v1 - v0;
        pmp::Point edge2 = v2 - v0;
        pmp::Point normal = pmp::cross(edge1, edge2);

        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int z = min_z; z <= max_z; ++z) {
                    pmp::Point voxel_center(
                        (x + 0.5) * voxel_size_,
                        (y + 0.5) * voxel_size_,
                        (z + 0.5) * voxel_size_
                    );

                    pmp::Point v0_to_p = voxel_center - v0;
                    pmp::Point v1_to_p = voxel_center - v1;
                    pmp::Point v2_to_p = voxel_center - v2;

                    pmp::Point cross0 = pmp::cross(v1 - v0, v0_to_p);
                    pmp::Point cross1 = pmp::cross(v2 - v1, v1_to_p);
                    pmp::Point cross2 = pmp::cross(v0 - v2, v2_to_p);

                    if (pmp::dot(cross0, normal) >= 0 &&
                        pmp::dot(cross1, normal) >= 0 &&
                        pmp::dot(cross2, normal) >= 0) {
                        voxels.insert(Vec3i(x, y, z));
                    }
                }
            }
        }

        voxels.insert(voxel0);
        voxels.insert(voxel1);
        voxels.insert(voxel2);
    }

    std::set<Vec3i> Voxelizer::fillInterior(const std::set<Vec3i>&surface_voxels) {
        if (surface_voxels.empty()) return surface_voxels;

        Box3i bbox = getBoundingBox(surface_voxels);

        std::set<Vec3i> filled_voxels = surface_voxels;

        for (int x = bbox.min.x; x <= bbox.max.x; ++x) {
            for (int z = bbox.min.z; z <= bbox.max.z; ++z) {
                bool inside = false;
                int transition_count = 0;

                for (int y = bbox.min.y; y <= bbox.max.y; ++y) {
                    Vec3i current(x, y, z);
                    bool is_surface = surface_voxels.find(current) != surface_voxels.end();

                    if (is_surface) {
                        if (!inside) {
                            inside = true;
                            transition_count++;
                        }
                    }
                    else if (inside && transition_count % 2 == 1) {
                        filled_voxels.insert(current);
                    }

                    if (is_surface && y + 1 <= bbox.max.y) {
                        Vec3i next(x, y + 1, z);
                        bool next_is_surface = surface_voxels.find(next) != surface_voxels.end();
                        if (!next_is_surface && inside) {
                            inside = false;
                        }
                    }
                }
            }
        }

        return filled_voxels;
    }

    Box3i Voxelizer::getBoundingBox(const std::set<Vec3i>&voxels) const {
        if (voxels.empty()) return Box3i();

        Vec3i min_point(INT_MAX, INT_MAX, INT_MAX);
        Vec3i max_point(INT_MIN, INT_MIN, INT_MIN);

        for (const auto&v: voxels) {
            min_point.x = std::min(min_point.x, v.x);
            min_point.y = std::min(min_point.y, v.y);
            min_point.z = std::min(min_point.z, v.z);

            max_point.x = std::max(max_point.x, v.x);
            max_point.y = std::max(max_point.y, v.y);
            max_point.z = std::max(max_point.z, v.z);
        }

        return Box3i(min_point, max_point);
    }
    
    Box3i Voxelizer::getBoundingBox(const std::set<VoxelData>&voxels) const {
        if (voxels.empty()) return Box3i();

        Vec3i min_point(INT_MAX, INT_MAX, INT_MAX);
        Vec3i max_point(INT_MIN, INT_MIN, INT_MIN);

        for (const auto&vd: voxels) {
            const auto& v = vd.position;
            min_point.x = std::min(min_point.x, v.x);
            min_point.y = std::min(min_point.y, v.y);
            min_point.z = std::min(min_point.z, v.z);

            max_point.x = std::max(max_point.x, v.x);
            max_point.y = std::max(max_point.y, v.y);
            max_point.z = std::max(max_point.z, v.z);
        }

        return Box3i(min_point, max_point);
    }
    
    std::set<VoxelData> Voxelizer::voxelizeWithMaterials(MeshProcessor& processor, bool solid) {
        std::cout << "Starting voxelization with materials, voxel size: " << voxel_size_ << std::endl;
        
        std::set<VoxelData> surface_voxels = voxelizeSurfaceWithMaterials(processor);
        std::cout << "Surface voxels with materials: " << surface_voxels.size() << std::endl;

        if (solid) {
            std::set<VoxelData> filled_voxels = fillInteriorWithColors(surface_voxels);
            std::cout << "Total voxels after filling: " << filled_voxels.size() << std::endl;
            // Ensure unique positions after filling
            return dedupeByPositionAverage(filled_voxels);
        }
        
        // Ensure unique positions in surface-only mode as well
        return dedupeByPositionAverage(surface_voxels);
    }
    
    std::set<VoxelData> Voxelizer::voxelizeSurfaceWithMaterials(MeshProcessor& processor) {
        std::set<VoxelData> voxels;
        
        if (!processor.hasObjLoader()) {
            // Fall back to no materials
            std::cout << "No material information available, using default color" << std::endl;
            auto simple_voxels = voxelizeSurface(processor.getMesh());
            for (const auto& v : simple_voxels) {
                voxels.insert(VoxelData(v, Color4()));
            }
            return voxels;
        }
        
        auto& mesh = processor.getMesh();
        auto& obj_loader = processor.getObjLoader();
        auto points = mesh.vertex_property<pmp::Point>("v:point");
        
        size_t face_idx = 0;
        for (auto f : mesh.faces()) {
            std::vector<pmp::Point> vertices;
            std::vector<Vec2f> uvs;
            
            size_t vert_idx = 0;
            for (auto v : mesh.vertices(f)) {
                vertices.push_back(points[v]);
                uvs.push_back(obj_loader.getUVForFaceVertex(face_idx, vert_idx));
                vert_idx++;
            }
            
            if (vertices.size() == 3) {
                Material* material = obj_loader.getMaterialForFace(face_idx);
                rasterizeTriangleWithMaterial(vertices[0], vertices[1], vertices[2],
                                             uvs[0], uvs[1], uvs[2], material,
                                             obj_loader.getMaterialLoader(), voxels);
            }
            face_idx++;
        }
        
        // Deduplicate by position across all triangles by averaging colors
        return dedupeByPositionAverage(voxels);
    }
    
    void Voxelizer::rasterizeTriangleWithMaterial(const pmp::Point&v0, const pmp::Point&v1,
                                                 const pmp::Point&v2, const Vec2f& uv0, 
                                                 const Vec2f& uv1, const Vec2f& uv2,
                                                 Material* material, const MaterialLoader& mat_loader,
                                                 std::set<VoxelData>&voxels) {
        Vec3i voxel0 = pointToVoxel(v0);
        Vec3i voxel1 = pointToVoxel(v1);
        Vec3i voxel2 = pointToVoxel(v2);
        
        int min_x = std::min({voxel0.x, voxel1.x, voxel2.x});
        int max_x = std::max({voxel0.x, voxel1.x, voxel2.x});
        int min_y = std::min({voxel0.y, voxel1.y, voxel2.y});
        int max_y = std::max({voxel0.y, voxel1.y, voxel2.y});
        int min_z = std::min({voxel0.z, voxel1.z, voxel2.z});
        int max_z = std::max({voxel0.z, voxel1.z, voxel2.z});
        
        // 使用map来累积同一位置的颜色
        struct ColorAccum { double r=0, g=0, b=0, a=0; int count=0; };
        std::map<Vec3i, ColorAccum> color_accumulator;

        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int z = min_z; z <= max_z; ++z) {
                    pmp::Point voxel_center(
                        (x + 0.5) * voxel_size_,
                        (y + 0.5) * voxel_size_,
                        (z + 0.5) * voxel_size_
                    );
                    
                    float w0, w1, w2;
                    if (computeBarycentricCoordinates(voxel_center, v0, v1, v2, w0, w1, w2)) {
                        // Interpolate UV coordinates
                        float u = w0 * uv0.u + w1 * uv1.u + w2 * uv2.u;
                        float v = w0 * uv0.v + w1 * uv1.v + w2 * uv2.v;
                        
                        // Get color from material
                        Color4 color;
                        if (material) {
                            color = mat_loader.calculateFinalColor(*material, u, v);
                        } else {
                            color = Color4();  // Default white
                        }
                        
                        // 累积颜色而不是直接插入
                        Vec3i pos(x, y, z);
                        auto& accum = color_accumulator[pos];
                        accum.r += color.r;
                        accum.g += color.g;
                        accum.b += color.b;
                        accum.a += color.a;
                        accum.count++;
                    }
                }
            }
        }
        
        // 确保三个顶点被包含
        Color4 default_color = material ?
            Color4(material->diffuse[0] * 255, material->diffuse[1] * 255, 
                  material->diffuse[2] * 255, material->opacity * 255) : Color4();

        // 将顶点颜色也累积进去
        for (const Vec3i& vertex_pos : {voxel0, voxel1, voxel2}) {
            auto& accum = color_accumulator[vertex_pos];
            accum.r += default_color.r;
            accum.g += default_color.g;
            accum.b += default_color.b;
            accum.a += default_color.a;
            accum.count++;
        }

        // 计算平均颜色并插入最终结果
        for (const auto& kv : color_accumulator) {
            const Vec3i& pos = kv.first;
            const ColorAccum& accum = kv.second;
            Color4 avg_color(
                static_cast<uint8_t>(std::round(accum.r / std::max(1, accum.count))),
                static_cast<uint8_t>(std::round(accum.g / std::max(1, accum.count))),
                static_cast<uint8_t>(std::round(accum.b / std::max(1, accum.count))),
                static_cast<uint8_t>(std::round(accum.a / std::max(1, accum.count)))
            );
            voxels.insert(VoxelData(pos, avg_color));
        }
    }
    
    bool Voxelizer::computeBarycentricCoordinates(const pmp::Point& p, const pmp::Point& v0,
                                                 const pmp::Point& v1, const pmp::Point& v2,
                                                 float& w0, float& w1, float& w2) const {
        pmp::Point edge1 = v1 - v0;
        pmp::Point edge2 = v2 - v0;
        pmp::Point normal = pmp::cross(edge1, edge2);
        
        // Check if point is close to the triangle plane
        pmp::Point v0_to_p = p - v0;
        float dist_to_plane = std::abs(pmp::dot(v0_to_p, normal) / pmp::norm(normal));
        
        if (dist_to_plane > voxel_size_) {
            return false;
        }
        
        // Project point onto triangle plane
        float t = pmp::dot(normal, v0 - p) / pmp::dot(normal, normal);
        pmp::Point projected = p + normal * t;
        
        // Compute barycentric coordinates for projected point
        pmp::Point v0_to_proj = projected - v0;
        
        float d00 = pmp::dot(edge1, edge1);
        float d01 = pmp::dot(edge1, edge2);
        float d11 = pmp::dot(edge2, edge2);
        float d20 = pmp::dot(v0_to_proj, edge1);
        float d21 = pmp::dot(v0_to_proj, edge2);
        
        float denom = d00 * d11 - d01 * d01;
        if (std::abs(denom) < 1e-10) return false;
        
        w1 = (d11 * d20 - d01 * d21) / denom;
        w2 = (d00 * d21 - d01 * d20) / denom;
        w0 = 1.0f - w1 - w2;
        
        // Check if point is inside triangle
        return w0 >= 0 && w1 >= 0 && w2 >= 0 && w0 <= 1 && w1 <= 1 && w2 <= 1;
    }
    
    std::set<VoxelData> Voxelizer::fillInteriorWithColors(const std::set<VoxelData>&surface_voxels) {
        if (surface_voxels.empty()) return surface_voxels;
        
        Box3i bbox = getBoundingBox(surface_voxels);
        std::set<VoxelData> filled_voxels = surface_voxels;
        
        // Create a map for quick lookup of surface voxels and their colors
        std::map<Vec3i, Color4> surface_map;
        for (const auto& vd : surface_voxels) {
            surface_map[vd.position] = vd.color;
        }
        
        for (int x = bbox.min.x; x <= bbox.max.x; ++x) {
            for (int z = bbox.min.z; z <= bbox.max.z; ++z) {
                bool inside = false;
                Color4 last_surface_color;
                
                for (int y = bbox.min.y; y <= bbox.max.y; ++y) {
                    Vec3i current(x, y, z);
                    auto it = surface_map.find(current);
                    bool is_surface = (it != surface_map.end());
                    
                    if (is_surface) {
                        last_surface_color = it->second;
                        inside = !inside;
                    } else if (inside) {
                        // Use the color of the last surface voxel encountered
                        filled_voxels.insert(VoxelData(current, last_surface_color));
                    }
                }
            }
        }
        
        return filled_voxels;
    }

    // Helper to deduplicate by position with averaged color
    std::set<VoxelData> Voxelizer::dedupeByPositionAverage(const std::set<VoxelData>& voxels) const {
        if (voxels.empty()) return voxels;
        struct ColorAccum { double r=0, g=0, b=0, a=0; int count=0; };
        std::map<Vec3i, ColorAccum> accum;
        for (const auto& vd : voxels) {
            auto& ac = accum[vd.position];
            ac.r += vd.color.r;
            ac.g += vd.color.g;
            ac.b += vd.color.b;
            ac.a += vd.color.a;
            ac.count += 1;
        }
        std::set<VoxelData> deduped;
        for (const auto& kv : accum) {
            const Vec3i& pos = kv.first;
            const ColorAccum& ac = kv.second;
            Color4 avg(
                static_cast<uint8_t>(std::round(ac.r / std::max(1, ac.count))),
                static_cast<uint8_t>(std::round(ac.g / std::max(1, ac.count))),
                static_cast<uint8_t>(std::round(ac.b / std::max(1, ac.count))),
                static_cast<uint8_t>(std::round(ac.a / std::max(1, ac.count)))
            );
            deduped.insert(VoxelData(pos, avg));
        }
        return deduped;
    }
}
