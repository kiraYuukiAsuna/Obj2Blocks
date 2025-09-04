#pragma once

#include <set>
#include <pmp/surface_mesh.h>
#include "types.h"

namespace obj2blocks {
    class Voxelizer {
    public:
        Voxelizer(double voxel_size = 1.0);

        ~Voxelizer();

        std::set<Vec3i> voxelize(pmp::SurfaceMesh&mesh, bool solid = true);

        void setVoxelSize(double size) { voxel_size_ = size; }

        double getVoxelSize() const { return voxel_size_; }

    private:
        double voxel_size_;

        Vec3i pointToVoxel(const pmp::Point&point) const;

        bool isInsideMesh(const pmp::Point&point, const pmp::SurfaceMesh&mesh) const;

        std::set<Vec3i> voxelizeSurface(pmp::SurfaceMesh&mesh);

        std::set<Vec3i> fillInterior(const std::set<Vec3i>&surface_voxels);

        void rasterizeTriangle(const pmp::Point&v0, const pmp::Point&v1,
                               const pmp::Point&v2, std::set<Vec3i>&voxels);

        Box3i getBoundingBox(const std::set<Vec3i>&voxels) const;
    };
}
