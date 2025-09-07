#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include "types.h"

namespace obj2blocks {
    class MaterialLoader {
    public:
        MaterialLoader();
        ~MaterialLoader();

        bool loadMTL(const std::string& mtl_path);
        bool loadTexture(const std::string& texture_path, TextureData& texture_data);
        Color4 sampleTexture(const TextureData& texture, float u, float v) const;
        
        const std::unordered_map<std::string, Material>& getMaterials() const { return materials_; }
        Material* getMaterial(const std::string& name);
        
        Color4 calculateFinalColor(const Material& material, float u, float v) const;

    private:
        std::unordered_map<std::string, Material> materials_;
        std::filesystem::path base_path_;
        
        std::string resolvePath(const std::string& path) const;
        void parseMTLLine(const std::string& line, Material& current_material);
    };
}