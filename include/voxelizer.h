#pragma once

#include <set>
#include <pmp/surface_mesh.h>
#include "types.h"
#include "mesh_processor.h"

namespace obj2blocks {
    class Voxelizer {
    public:
        Voxelizer(double voxel_size = 1.0);

        ~Voxelizer();

        std::set<Vec3i> voxelize(pmp::SurfaceMesh&mesh, bool solid = true);
        
        // New method with material support
        std::set<VoxelData> voxelizeWithMaterials(MeshProcessor& processor, bool solid = true);

        void setVoxelSize(double size) { voxel_size_ = size; }

        double getVoxelSize() const { return voxel_size_; }

    private:
        double voxel_size_;

        Vec3i pointToVoxel(const pmp::Point&point) const;

        bool isInsideMesh(const pmp::Point&point, const pmp::SurfaceMesh&mesh) const;

        std::set<Vec3i> voxelizeSurface(pmp::SurfaceMesh&mesh);
        
        std::set<VoxelData> voxelizeSurfaceWithMaterials(MeshProcessor& processor);

        std::set<Vec3i> fillInterior(const std::set<Vec3i>&surface_voxels);
        
        std::set<VoxelData> fillInteriorWithColors(const std::set<VoxelData>&surface_voxels);

        void rasterizeTriangle(const pmp::Point&v0, const pmp::Point&v1,
                               const pmp::Point&v2, std::set<Vec3i>&voxels);
        
        void rasterizeTriangleWithMaterial(const pmp::Point&v0, const pmp::Point&v1,
                                          const pmp::Point&v2, const Vec2f& uv0, const Vec2f& uv1,
                                          const Vec2f& uv2, Material* material,
                                          const MaterialLoader& mat_loader,
                                          std::set<VoxelData>&voxels);

        Box3i getBoundingBox(const std::set<Vec3i>&voxels) const;
        
        Box3i getBoundingBox(const std::set<VoxelData>&voxels) const;
        
        // Helper to compute barycentric coordinates
        bool computeBarycentricCoordinates(const pmp::Point& p, const pmp::Point& v0,
                                          const pmp::Point& v1, const pmp::Point& v2,
                                          float& w0, float& w1, float& w2) const;
    };
}
