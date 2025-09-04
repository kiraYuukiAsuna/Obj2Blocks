#pragma once

#include <string>

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

        MinecraftCommand(const Vec3i&pos)
            : type(CommandType::CreateBlock), position(pos) {
        }

        MinecraftCommand(const Box3i&box)
            : type(CommandType::FillArea), area(box) {
        }
    };

    struct ConversionParams {
        std::string input_file;
        std::string output_file;
        double target_size = 100.0; // Target maximum dimension
        double voxel_size = 1.0; // Size of each voxel
        bool auto_scale = true; // Auto-calculate scale
        double scale_factor = 1.0; // Manual scale factor
        bool solid = true; // Fill interior (true) or surface only (false)
        bool optimize = true; // Optimize with fillarea commands
    };
}
