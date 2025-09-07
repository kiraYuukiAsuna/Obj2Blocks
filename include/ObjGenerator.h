#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <vector>
#include <tuple>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

struct Vec3 {
    double x, y, z;
    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

struct Color {
    int r, g, b, a;
    
    Color(int r = 255, int g = 255, int b = 255, int a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    bool operator<(const Color& other) const {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        if (b != other.b) return b < other.b;
        return a < other.a;
    }
    
    std::string toMaterialName() const {
        std::stringstream ss;
        ss << "material_" << std::hex << std::setfill('0') 
           << std::setw(2) << r << std::setw(2) << g 
           << std::setw(2) << b << std::setw(2) << a;
        return ss.str();
    }
};

class ObjGenerator {
private:
    std::vector<Vec3> vertices;
    std::vector<std::tuple<std::vector<int>, std::string>> faces;
    std::map<Color, std::string> materials;
    int vertexOffset = 0;

    void addCube(const Vec3& position, const std::string& materialName, double size = 1.0);
    void addFilledArea(const Vec3& corner1, const Vec3& corner2, const std::string& materialName);
    std::string getOrCreateMaterial(const Color& color);
    void writeMTLFile(const std::string& filename);
    void createColorTexture(const std::string& filename);

public:
    void processCommand(const json& command);
    void writeToFile(const std::string& filename);
};
