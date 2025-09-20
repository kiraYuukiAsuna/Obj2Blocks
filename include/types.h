#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>
#include <unordered_map>

namespace obj2blocks {
    struct Vec3i {
        int x, y, z;

        Vec3i(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {
        }

        bool operator==(const Vec3i&other) const {
            return x == other.x && y == other.y && z == other.z;
        }

        bool operator<(const Vec3i&other) const {
            if (x != other.x) return x < other.x;
            if (y != other.y) return y < other.y;
            return z < other.z;
        }
    };

    struct Vec2f {
        float u, v;
        Vec2f(float u = 0.0f, float v = 0.0f) : u(u), v(v) {}
    };

    struct Color4 {
        uint8_t r, g, b, a;
        
        Color4() : r(255), g(255), b(255), a(255) {}
        Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
            : r(r), g(g), b(b), a(a) {}
        
        bool operator==(const Color4& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        
        bool operator<(const Color4& other) const {
            if (r != other.r) return r < other.r;
            if (g != other.g) return g < other.g;
            if (b != other.b) return b < other.b;
            return a < other.a;
        }
    };

    struct VoxelData {
        Vec3i position;
        Color4 color;
        
        VoxelData(const Vec3i& pos, const Color4& col = Color4()) 
            : position(pos), color(col) {}
        
        bool operator<(const VoxelData& other) const {
            if (position != other.position) return position < other.position;
            return color < other.color;
        }
    };

    struct TextureData {
        std::vector<uint8_t> data;
        int width = 0;
        int height = 0;
        int channels = 0;
        
        bool isValid() const { return !data.empty() && width > 0 && height > 0; }
    };

    struct Material {
        std::string name;
        
        // Color properties from MTL
        std::array<float, 3> ambient = {0.2f, 0.2f, 0.2f};    // Ka
        std::array<float, 3> diffuse = {1.0f, 1.0f, 1.0f};    // Kd
        std::array<float, 3> specular = {0.0f, 0.0f, 0.0f};   // Ks
        std::array<float, 3> emissive = {0.0f, 0.0f, 0.0f};   // Ke
        float shininess = 1.0f;  // Ns
        float opacity = 1.0f;     // d or Tr
        
        // Texture paths
        std::string ambient_texture_path;   // map_Ka
        std::string diffuse_texture_path;   // map_Kd
        std::string specular_texture_path;  // map_Ks
        std::string emissive_texture_path;  // map_Ke
        std::string normal_texture_path;    // map_Bump or norm
        std::string opacity_texture_path;   // map_d
        
        // Loaded texture data
        std::unordered_map<std::string, TextureData> textures;
        
        bool hasDiffuseTexture() const { 
            return textures.find(diffuse_texture_path) != textures.end() && 
                   textures.at(diffuse_texture_path).isValid();
        }
    };

    struct Box3i {
        Vec3i min;
        Vec3i max;

        Box3i() = default;

        Box3i(const Vec3i&min, const Vec3i&max) : min(min), max(max) {
        }

        int volume() const {
            return (max.x - min.x + 1) * (max.y - min.y + 1) * (max.z - min.z + 1);
        }

        bool contains(const Vec3i&p) const {
            return p.x >= min.x && p.x <= max.x &&
                   p.y >= min.y && p.y <= max.y &&
                   p.z >= min.z && p.z <= max.z;
        }
    };

    enum class CommandType {
        CreateBlock,
        FillArea
    };

    struct MinecraftCommand {
        CommandType type;
        Vec3i position; // For CreateBlock
        Box3i area; // For FillArea
        Color4 color; // Color for the block(s)

        MinecraftCommand(const Vec3i&pos, const Color4& col = Color4())
            : type(CommandType::CreateBlock), position(pos), color(col) {
        }

        MinecraftCommand(const Box3i&box, const Color4& col = Color4())
            : type(CommandType::FillArea), area(box), color(col) {
        }
    };

    struct ConversionParams {
        std::string input_file;
        std::string output_file;
        double target_size = 200.0; // Target maximum dimension
        double voxel_size = 1.0; // Size of each voxel
        bool auto_scale = true; // Auto-calculate scale
        double scale_factor = 1.0; // Manual scale factor
        bool solid = false; // Fill interior (true) or surface only (false)
        bool optimize = false; // Optimize with fillarea commands
        bool with_texture = false; // Use texture mapping for block colors
    };
}
