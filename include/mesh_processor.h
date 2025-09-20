#pragma once

#include <string>
#include <memory>
#include <pmp/surface_mesh.h>
#include "obj_loader.h"

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
        
        const ObjLoader& getObjLoader() const { return *obj_loader_; }
        
        ObjLoader& getObjLoader() { return *obj_loader_; }
        
        bool hasObjLoader() const { return obj_loader_ != nullptr; }

    private:
        pmp::SurfaceMesh mesh_;
        std::unique_ptr<ObjLoader> obj_loader_;
    };
}
