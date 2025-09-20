#include "obj_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace obj2blocks {

ObjLoader::ObjLoader() {}

ObjLoader::~ObjLoader() {}

bool ObjLoader::load(const std::string& obj_path) {
    std::ifstream file(obj_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << obj_path << std::endl;
        return false;
    }
    
    std::string base_path = std::filesystem::path(obj_path).parent_path().string();
    
    std::string line;
    while (std::getline(file, line)) {
        // Handle MTL lib first
        if (line.rfind("mtllib", 0) == 0) {
            parseMTLLib(line, base_path);
        } else {
            parseLine(line);
        }
    }
    
    file.close();
    
    std::cout << "Loaded OBJ with:" << std::endl;
    std::cout << "  Vertices: " << vertices_.size() << std::endl;
    std::cout << "  UVs: " << uvs_.size() << std::endl;
    std::cout << "  Normals: " << normals_.size() << std::endl;
    std::cout << "  Faces: " << faces_.size() << std::endl;
    
    return !vertices_.empty() && !faces_.empty();
}

void ObjLoader::parseLine(const std::string& line) {
    if (line.empty() || line[0] == '#') return;
    
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;
    
    if (prefix == "v") {
        parseVertex(line);
    } else if (prefix == "vt") {
        parseUV(line);
    } else if (prefix == "vn") {
        parseNormal(line);
    } else if (prefix == "f") {
        parseFace(line);
    } else if (prefix == "usemtl") {
        parseMaterial(line);
    }
}

void ObjLoader::parseVertex(const std::string& line) {
    std::istringstream iss(line);
    std::string prefix;
    float x, y, z;
    iss >> prefix >> x >> y >> z;
    vertices_.emplace_back(x, y, z);
}

void ObjLoader::parseUV(const std::string& line) {
    std::istringstream iss(line);
    std::string prefix;
    float u, v;
    iss >> prefix >> u >> v;
    uvs_.emplace_back(u, v);
}

void ObjLoader::parseNormal(const std::string& line) {
    std::istringstream iss(line);
    std::string prefix;
    float x, y, z;
    iss >> prefix >> x >> y >> z;
    normals_.emplace_back(x, y, z);
}

void ObjLoader::parseFace(const std::string& line) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;
    
    FaceData face;
    face.material_name = current_material_;
    
    std::string vertex_str;
    while (iss >> vertex_str) {
        // Parse vertex/uv/normal indices
        std::istringstream vss(vertex_str);
        std::string index_str;
        
        // Vertex index
        if (std::getline(vss, index_str, '/')) {
            if (!index_str.empty()) {
                int idx = std::stoi(index_str);
                // OBJ indices are 1-based, convert to 0-based
                face.vertex_indices.push_back(idx > 0 ? idx - 1 : vertices_.size() + idx);
            }
        }
        
        // UV index
        if (std::getline(vss, index_str, '/')) {
            if (!index_str.empty()) {
                int idx = std::stoi(index_str);
                face.uv_indices.push_back(idx > 0 ? idx - 1 : uvs_.size() + idx);
            } else {
                face.uv_indices.push_back(-1);  // No UV
            }
        } else {
            face.uv_indices.push_back(-1);  // No UV
        }
        
        // Normal index
        if (std::getline(vss, index_str, '/')) {
            if (!index_str.empty()) {
                int idx = std::stoi(index_str);
                face.normal_indices.push_back(idx > 0 ? idx - 1 : normals_.size() + idx);
            } else {
                face.normal_indices.push_back(-1);  // No normal
            }
        } else {
            face.normal_indices.push_back(-1);  // No normal
        }
    }
    
    // Triangulate faces with more than 3 vertices
    if (face.vertex_indices.size() == 3) {
        faces_.push_back(face);
    } else if (face.vertex_indices.size() > 3) {
        // Fan triangulation
        for (size_t i = 1; i < face.vertex_indices.size() - 1; ++i) {
            FaceData tri;
            tri.material_name = face.material_name;
            
            // First vertex
            tri.vertex_indices.push_back(face.vertex_indices[0]);
            tri.uv_indices.push_back(face.uv_indices[0]);
            tri.normal_indices.push_back(face.normal_indices[0]);
            
            // Current vertex
            tri.vertex_indices.push_back(face.vertex_indices[i]);
            tri.uv_indices.push_back(face.uv_indices[i]);
            tri.normal_indices.push_back(face.normal_indices[i]);
            
            // Next vertex
            tri.vertex_indices.push_back(face.vertex_indices[i + 1]);
            tri.uv_indices.push_back(face.uv_indices[i + 1]);
            tri.normal_indices.push_back(face.normal_indices[i + 1]);
            
            faces_.push_back(tri);
        }
    }
}

void ObjLoader::parseMaterial(const std::string& line) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix >> current_material_;
}

void ObjLoader::parseMTLLib(const std::string& line, const std::string& base_path) {
    std::istringstream iss(line);
    std::string prefix, mtl_file;
    iss >> prefix >> mtl_file;
    
    std::filesystem::path mtl_path = std::filesystem::path(base_path) / mtl_file;
    if (material_loader_.loadMTL(mtl_path.string())) {
        std::cout << "Loaded MTL file: " << mtl_path.string() << std::endl;
    }
}

bool ObjLoader::buildSurfaceMesh(pmp::SurfaceMesh& mesh) {
    mesh.clear();
    
    // Add vertices
    std::vector<pmp::Vertex> vertex_handles;
    for (const auto& v : vertices_) {
        vertex_handles.push_back(mesh.add_vertex(v));
    }
    
    // Add faces
    for (const auto& face : faces_) {
        std::vector<pmp::Vertex> face_vertices;
        for (int idx : face.vertex_indices) {
            if (idx >= 0 && idx < vertex_handles.size()) {
                face_vertices.push_back(vertex_handles[idx]);
            }
        }
        if (face_vertices.size() == 3) {
            mesh.add_face(face_vertices);
        }
    }
    
    return mesh.n_vertices() > 0 && mesh.n_faces() > 0;
}

Material* ObjLoader::getMaterialForFace(size_t face_index) {
    if (face_index >= faces_.size()) return nullptr;
    return material_loader_.getMaterial(faces_[face_index].material_name);
}

Vec2f ObjLoader::getUVForFaceVertex(size_t face_index, size_t vertex_index) const {
    if (face_index >= faces_.size() || vertex_index >= faces_[face_index].uv_indices.size()) {
        return Vec2f(0, 0);
    }
    
    int uv_idx = faces_[face_index].uv_indices[vertex_index];
    if (uv_idx >= 0 && uv_idx < uvs_.size()) {
        return uvs_[uv_idx];
    }
    
    return Vec2f(0, 0);
}

}  // namespace obj2blocks