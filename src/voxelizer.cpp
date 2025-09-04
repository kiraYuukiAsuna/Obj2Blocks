#include "voxelizer.h"
#include <queue>
#include <algorithm>
#include <iostream>

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
}
