#pragma once

#include <string>
#include <pmp/surface_mesh.h>

namespace obj2blocks {
    class MeshProcessor {
    public:
        MeshProcessor();

        ~MeshProcessor();

        bool loadOBJ(const std::string&filename);

        void scaleMesh(double scale_factor);

        void autoScale(double target_size);

        void centerMesh();

        void getBoundingBox(pmp::Point&min_point, pmp::Point&max_point);

        double getMaxDimension();

        const pmp::SurfaceMesh& getMesh() const { return mesh_; }

        pmp::SurfaceMesh& getMesh() { return mesh_; }

    private:
        pmp::SurfaceMesh mesh_;
    };
}
