#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <pmp/surface_mesh.h>
#include "types.h"
#include "material_loader.h"

namespace obj2blocks {
    
struct FaceData {
    std::vector<int> vertex_indices;
    std::vector<int> uv_indices;
    std::vector<int> normal_indices;
    std::string material_name;
};

class ObjLoader {
public:
    ObjLoader();
    ~ObjLoader();
    
    bool load(const std::string& obj_path);
    
    const std::vector<pmp::Point>& getVertices() const { return vertices_; }
    const std::vector<Vec2f>& getUVs() const { return uvs_; }
    const std::vector<pmp::Point>& getNormals() const { return normals_; }
    const std::vector<FaceData>& getFaces() const { return faces_; }
    const MaterialLoader& getMaterialLoader() const { return material_loader_; }
    MaterialLoader& getMaterialLoader() { return material_loader_; }
    
    bool buildSurfaceMesh(pmp::SurfaceMesh& mesh);
    
    // Get material for a face
    Material* getMaterialForFace(size_t face_index);
    
    // Get UV coordinates for a face vertex
    Vec2f getUVForFaceVertex(size_t face_index, size_t vertex_index) const;

private:
    std::vector<pmp::Point> vertices_;
    std::vector<Vec2f> uvs_;
    std::vector<pmp::Point> normals_;
    std::vector<FaceData> faces_;
    MaterialLoader material_loader_;
    std::string current_material_;
    
    void parseLine(const std::string& line);
    void parseVertex(const std::string& line);
    void parseUV(const std::string& line);
    void parseNormal(const std::string& line);
    void parseFace(const std::string& line);
    void parseMaterial(const std::string& line);
    void parseMTLLib(const std::string& line, const std::string& base_path);
};

}  // namespace obj2blocks