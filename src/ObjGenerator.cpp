#include "ObjGenerator.h"
#include <algorithm>
#include <vector>
#include <cmath>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

void ObjGenerator::addCube(const Vec3& position, const std::string& materialName, double size) {
    double half = size / 2.0;

    vertices.push_back(Vec3(position.x - half, position.y - half, position.z - half));
    vertices.push_back(Vec3(position.x + half, position.y - half, position.z - half));
    vertices.push_back(Vec3(position.x + half, position.y + half, position.z - half));
    vertices.push_back(Vec3(position.x - half, position.y + half, position.z - half));
    vertices.push_back(Vec3(position.x - half, position.y - half, position.z + half));
    vertices.push_back(Vec3(position.x + half, position.y - half, position.z + half));
    vertices.push_back(Vec3(position.x + half, position.y + half, position.z + half));
    vertices.push_back(Vec3(position.x - half, position.y + half, position.z + half));

    int base = vertexOffset + 1;
    faces.push_back({{base, base + 1, base + 2, base + 3}, materialName});
    faces.push_back({{base + 4, base + 7, base + 6, base + 5}, materialName});
    faces.push_back({{base, base + 4, base + 5, base + 1}, materialName});
    faces.push_back({{base + 1, base + 5, base + 6, base + 2}, materialName});
    faces.push_back({{base + 2, base + 6, base + 7, base + 3}, materialName});
    faces.push_back({{base + 3, base + 7, base + 4, base}, materialName});

    vertexOffset += 8;
}

void ObjGenerator::addFilledArea(const Vec3& corner1, const Vec3& corner2, const std::string& materialName) {
    double xMin = std::min(corner1.x, corner2.x);
    double xMax = std::max(corner1.x, corner2.x);
    double yMin = std::min(corner1.y, corner2.y);
    double yMax = std::max(corner1.y, corner2.y);
    double zMin = std::min(corner1.z, corner2.z);
    double zMax = std::max(corner1.z, corner2.z);

    vertices.push_back(Vec3(xMin, yMin, zMin));
    vertices.push_back(Vec3(xMax, yMin, zMin));
    vertices.push_back(Vec3(xMax, yMax, zMin));
    vertices.push_back(Vec3(xMin, yMax, zMin));
    vertices.push_back(Vec3(xMin, yMin, zMax));
    vertices.push_back(Vec3(xMax, yMin, zMax));
    vertices.push_back(Vec3(xMax, yMax, zMax));
    vertices.push_back(Vec3(xMin, yMax, zMax));

    int base = vertexOffset + 1;
    faces.push_back({{base, base + 1, base + 2, base + 3}, materialName});
    faces.push_back({{base + 4, base + 7, base + 6, base + 5}, materialName});
    faces.push_back({{base, base + 4, base + 5, base + 1}, materialName});
    faces.push_back({{base + 1, base + 5, base + 6, base + 2}, materialName});
    faces.push_back({{base + 2, base + 6, base + 7, base + 3}, materialName});
    faces.push_back({{base + 3, base + 7, base + 4, base}, materialName});

    vertexOffset += 8;
}

std::string ObjGenerator::getOrCreateMaterial(const Color& color) {
    auto it = materials.find(color);
    if (it != materials.end()) {
        return it->second;
    }
    
    std::string matName = color.toMaterialName();
    materials[color] = matName;
    return matName;
}

void ObjGenerator::processCommand(const json& command) {
    std::string type = command["type"];
    
    // Extract color if present
    Color color;
    if (command.contains("color") && command["color"].is_array()) {
        const auto& col = command["color"];
        if (col.size() >= 3) {
            color.r = col[0];
            color.g = col[1];
            color.b = col[2];
            if (col.size() >= 4) {
                color.a = col[3];
            }
        }
    }
    
    std::string materialName = getOrCreateMaterial(color);

    if (type == "createblock") {
        const auto& pos = command["position"];
        Vec3 position(pos[0], pos[1], pos[2]);
        addCube(position, materialName);
    }
    else if (type == "fillarea") {
        const auto& c1 = command["corner1"];
        const auto& c2 = command["corner2"];
        Vec3 corner1(c1[0], c1[1], c1[2]);
        Vec3 corner2(c2[0], c2[1], c2[2]);
        addFilledArea(corner1, corner2, materialName);
    }
}

void ObjGenerator::writeToFile(const std::string& filename) {
    // Build reverse lookup: material name to color
    std::map<std::string, Color> materialToColor;
    std::map<Color, int> colorIndexMap;
    int colorIdx = 0;
    for (const auto& [color, matName] : materials) {
        materialToColor[matName] = color;
        colorIndexMap[color] = colorIdx++;
    }
    
    // Calculate texture atlas size
    int atlasSize = std::ceil(std::sqrt(materials.size()));
    if (atlasSize < 1) atlasSize = 1;
    if (atlasSize > 256) atlasSize = 256;
    
    // Write MTL file if we have materials
    if (!materials.empty()) {
        std::string mtlFilename = filename.substr(0, filename.find_last_of('.')) + ".mtl";
        writeMTLFile(mtlFilename);
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open output file " << filename << std::endl;
        return;
    }

    file << "# OBJ file generated from JSON commands\n";
    file << "# Generated by Obj2Blocks converter\n\n";
    
    // Reference MTL file if we have materials
    if (!materials.empty()) {
        std::string mtlBasename = filename.substr(filename.find_last_of("/\\") + 1);
        mtlBasename = mtlBasename.substr(0, mtlBasename.find_last_of('.')) + ".mtl";
        file << "mtllib " << mtlBasename << "\n";
        file << "usemtl textured_blocks\n\n";
    }

    // Write vertices
    for (const auto& v : vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    file << "\n";
    
    // Pre-calculate UV coordinates for each material
    std::map<std::string, std::pair<float, float>> materialUVs;
    for (const auto& [matName, color] : materialToColor) {
        int idx = colorIndexMap[color];
        int row = idx / atlasSize;
        int col = idx % atlasSize;
        float u = (col + 0.5f) / atlasSize;
        float v = 1.0f - (row + 0.5f) / atlasSize;
        materialUVs[matName] = {u, v};
    }
    
    // Write UV coordinates - simplified: one UV per material, reused
    file << "# UV coordinates\n";
    std::map<std::string, int> materialUVIndex;
    int uvIdx = 1;
    for (const auto& [matName, uv] : materialUVs) {
        file << "vt " << uv.first << " " << uv.second << "\n";
        materialUVIndex[matName] = uvIdx++;
    }

    file << "\n# Faces with texture coordinates\n";
    
    // Write faces - much faster now
    for (const auto& [faceIndices, matName] : faces) {
        int uvIndex = materialUVIndex[matName];
        file << "f";
        for (int idx : faceIndices) {
            file << " " << idx << "/" << uvIndex;
        }
        file << "\n";
    }

    file.close();
    std::cout << "OBJ file written to: " << filename << std::endl;
}

void ObjGenerator::writeMTLFile(const std::string& filename) {
    // Create color texture
    std::string textureFile = filename.substr(0, filename.find_last_of('.')) + "_colors.png";
    createColorTexture(textureFile);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open MTL file " << filename << std::endl;
        return;
    }
    
    file << "# MTL file generated from JSON commands\n";
    file << "# Generated by Obj2Blocks converter\n\n";
    
    // Use texture instead of per-material colors
    file << "newmtl textured_blocks\n";
    file << "Ka 1.0 1.0 1.0\n";
    file << "Kd 1.0 1.0 1.0\n";
    file << "Ks 0.0 0.0 0.0\n";
    file << "Ns 1.0\n";
    file << "d 1.0\n";
    
    std::string textureBasename = textureFile.substr(textureFile.find_last_of("/\\") + 1);
    file << "map_Kd " << textureBasename << "\n\n";
    
    file.close();
    std::cout << "MTL file written to: " << filename << std::endl;
}

void ObjGenerator::createColorTexture(const std::string& filename) {
    // Create a simple texture atlas with one pixel per color
    int atlasSize = std::ceil(std::sqrt(materials.size()));
    if (atlasSize < 1) atlasSize = 1;
    if (atlasSize > 256) atlasSize = 256;  // Max 256x256
    
    std::vector<uint8_t> pixels(atlasSize * atlasSize * 3, 255);
    
    int idx = 0;
    for (const auto& [color, matName] : materials) {
        if (idx >= atlasSize * atlasSize) break;
        
        int pixelIdx = idx * 3;
        pixels[pixelIdx] = color.r;
        pixels[pixelIdx + 1] = color.g;
        pixels[pixelIdx + 2] = color.b;
        idx++;
    }
    
    stbi_write_png(filename.c_str(), atlasSize, atlasSize, 3, pixels.data(), atlasSize * 3);
    std::cout << "Color texture created: " << filename << " (" << atlasSize << "x" << atlasSize << ")" << std::endl;
}