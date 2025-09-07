#include "mesh_processor.h"
#include <pmp/io/io.h>
#include <iostream>
#include <limits>

namespace obj2blocks {
    MeshProcessor::MeshProcessor() {
    }

    MeshProcessor::~MeshProcessor() {
    }

    bool MeshProcessor::loadOBJ(const std::string&filename) {
        // First try to load with our custom loader for material support
        obj_loader_ = std::make_unique<ObjLoader>();
        if (obj_loader_->load(filename)) {
            // Build the surface mesh from loaded data
            if (obj_loader_->buildSurfaceMesh(mesh_)) {
                std::cout << "Loaded mesh with materials: " << mesh_.n_vertices() << " vertices and "
                        << mesh_.n_faces() << " faces" << std::endl;
                return true;
            }
        }
        
        // Fallback to pmp loader if custom loader fails
        obj_loader_.reset();
        try {
            pmp::read(mesh_, filename);
            if (mesh_.n_vertices() == 0) {
                std::cerr << "Error: Loaded mesh has no vertices" << std::endl;
                return false;
            }
            std::cout << "Loaded mesh without materials: " << mesh_.n_vertices() << " vertices and "
                    << mesh_.n_faces() << " faces" << std::endl;
            return true;
        }
        catch (const std::exception&e) {
            std::cerr << "Error loading OBJ file: " << e.what() << std::endl;
            return false;
        }
    }

    void MeshProcessor::scaleMesh(double scale_factor) {
        auto points = mesh_.vertex_property<pmp::Point>("v:point");
        for (auto v: mesh_.vertices()) {
            points[v] *= scale_factor;
        }
    }

    void MeshProcessor::autoScale(double target_size) {
        double max_dim = getMaxDimension();
        if (max_dim > 0) {
            double scale_factor = target_size / max_dim;
            scaleMesh(scale_factor);
            std::cout << "Auto-scaled mesh by factor: " << scale_factor << std::endl;
        }
    }

    void MeshProcessor::centerMesh() {
        pmp::Point min_point, max_point;
        getBoundingBox(min_point, max_point);

        pmp::Point center = (min_point + max_point) * 0.5;

        auto points = mesh_.vertex_property<pmp::Point>("v:point");
        for (auto v: mesh_.vertices()) {
            points[v] -= center;
        }
    }

    void MeshProcessor::getBoundingBox(pmp::Point&min_point, pmp::Point&max_point) {
        min_point = pmp::Point(std::numeric_limits<float>::max());
        max_point = pmp::Point(std::numeric_limits<float>::lowest());

        auto points = mesh_.vertex_property<pmp::Point>("v:point");
        for (auto v: mesh_.vertices()) {
            const auto&p = points[v];
            for (int i = 0; i < 3; ++i) {
                min_point[i] = std::min(min_point[i], p[i]);
                max_point[i] = std::max(max_point[i], p[i]);
            }
        }
    }

    double MeshProcessor::getMaxDimension() {
        pmp::Point min_point, max_point;
        getBoundingBox(min_point, max_point);

        pmp::Point dimensions = max_point - min_point;
        return std::max({dimensions[0], dimensions[1], dimensions[2]});
    }
}
