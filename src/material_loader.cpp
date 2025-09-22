#include "material_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace obj2blocks {
    
MaterialLoader::MaterialLoader() {}

MaterialLoader::~MaterialLoader() {}

bool MaterialLoader::loadMTL(const std::string& mtl_path) {
    std::ifstream file(mtl_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open MTL file: " << mtl_path << std::endl;
        return false;
    }
    
    base_path_ = std::filesystem::path(mtl_path).parent_path();
    
    std::string line;
    Material current_material;
    bool has_material = false;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key == "newmtl") {
            if (has_material) {
                // Load textures for the previous material
                if (!current_material.diffuse_texture_path.empty()) {
                    TextureData tex_data;
                    if (loadTexture(resolvePath(current_material.diffuse_texture_path), tex_data)) {
                        current_material.textures[current_material.diffuse_texture_path] = tex_data;
                    }
                }
                if (!current_material.emissive_texture_path.empty()) {
                    TextureData tex_data;
                    if (loadTexture(resolvePath(current_material.emissive_texture_path), tex_data)) {
                        current_material.textures[current_material.emissive_texture_path] = tex_data;
                    }
                }
                if (!current_material.ambient_texture_path.empty()) {
                    TextureData tex_data;
                    if (loadTexture(resolvePath(current_material.ambient_texture_path), tex_data)) {
                        current_material.textures[current_material.ambient_texture_path] = tex_data;
                    }
                }
                if (!current_material.specular_texture_path.empty()) {
                    TextureData tex_data;
                    if (loadTexture(resolvePath(current_material.specular_texture_path), tex_data)) {
                        current_material.textures[current_material.specular_texture_path] = tex_data;
                    }
                }
                materials_[current_material.name] = current_material;
            }
            current_material = Material();
            iss >> current_material.name;
            has_material = true;
        } else if (has_material) {
            parseMTLLine(line, current_material);
        }
    }
    
    // Don't forget the last material
    if (has_material) {
        if (!current_material.diffuse_texture_path.empty()) {
            TextureData tex_data;
            if (loadTexture(resolvePath(current_material.diffuse_texture_path), tex_data)) {
                current_material.textures[current_material.diffuse_texture_path] = tex_data;
            }
        }
        if (!current_material.emissive_texture_path.empty()) {
            TextureData tex_data;
            if (loadTexture(resolvePath(current_material.emissive_texture_path), tex_data)) {
                current_material.textures[current_material.emissive_texture_path] = tex_data;
            }
        }
        if (!current_material.ambient_texture_path.empty()) {
            TextureData tex_data;
            if (loadTexture(resolvePath(current_material.ambient_texture_path), tex_data)) {
                current_material.textures[current_material.ambient_texture_path] = tex_data;
            }
        }
        if (!current_material.specular_texture_path.empty()) {
            TextureData tex_data;
            if (loadTexture(resolvePath(current_material.specular_texture_path), tex_data)) {
                current_material.textures[current_material.specular_texture_path] = tex_data;
            }
        }
        materials_[current_material.name] = current_material;
    }
    
    file.close();
    std::cout << "Loaded " << materials_.size() << " materials from " << mtl_path << std::endl;
    return true;
}

void MaterialLoader::parseMTLLine(const std::string& line, Material& material) {
    std::istringstream iss(line);
    std::string key;
    iss >> key;
    
    if (key == "Ka") {
        iss >> material.ambient[0] >> material.ambient[1] >> material.ambient[2];
    } else if (key == "Kd") {
        iss >> material.diffuse[0] >> material.diffuse[1] >> material.diffuse[2];
    } else if (key == "Ks") {
        iss >> material.specular[0] >> material.specular[1] >> material.specular[2];
    } else if (key == "Ke") {
        iss >> material.emissive[0] >> material.emissive[1] >> material.emissive[2];
    } else if (key == "Ns") {
        iss >> material.shininess;
    } else if (key == "d") {
        iss >> material.opacity;
    } else if (key == "Tr") {
        float transparency;
        iss >> transparency;
        material.opacity = 1.0f - transparency;
    } else if (key == "map_Ka") {
        iss >> material.ambient_texture_path;
    } else if (key == "map_Kd") {
        iss >> material.diffuse_texture_path;
    } else if (key == "map_Ks") {
        iss >> material.specular_texture_path;
    } else if (key == "map_Ke") {
        iss >> material.emissive_texture_path;
    } else if (key == "map_Bump" || key == "bump" || key == "norm") {
        iss >> material.normal_texture_path;
    } else if (key == "map_d") {
        iss >> material.opacity_texture_path;
    }
}

std::string MaterialLoader::resolvePath(const std::string& path) const {
    std::filesystem::path full_path = base_path_ / path;
    return full_path.string();
}

bool MaterialLoader::loadTexture(const std::string& texture_path, TextureData& texture_data) {
    int width, height, channels;
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << texture_path << std::endl;
        return false;
    }
    
    texture_data.width = width;
    texture_data.height = height;
    texture_data.channels = channels;
    texture_data.data.assign(data, data + (width * height * channels));
    
    stbi_image_free(data);
    
    std::cout << "Loaded texture: " << texture_path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    return true;
}

Color4 MaterialLoader::sampleTexture(const TextureData& texture, float u, float v) const {
    if (!texture.isValid()) {
        return Color4();
    }

    // 确保UV坐标在[0,1]范围内（使用wrap模式）
    u = u - std::floor(u);
    v = v - std::floor(v);

    // 处理负数情况
    if (u < 0)
      u += 1.0f;
    if (v < 0)
      v += 1.0f;

    // 额外的安全检查，确保UV在[0,1]范围内
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));

    // 转换为像素坐标 - 使用最近邻采样保持像素风格
    int x = static_cast<int>(u * texture.width);
    int y = static_cast<int>((1.0f - v) * texture.height); // 翻转V坐标

    // 确保坐标在有效范围内 - 使用更严格的边界检查
    x = std::max(0, std::min(x, texture.width - 1));
    y = std::max(0, std::min(y, texture.height - 1));

    // 计算像素索引
    int pixel_index = (y * texture.width + x) * texture.channels;
    
    // 额外的安全检查 - 确保索引不会超出数据范围
    Color4 color;
    if (pixel_index >= 0 && texture.data.size() > 0 &&
        pixel_index + texture.channels - 1 < static_cast<int>(texture.data.size())) {
      color.r = texture.data[pixel_index];
      color.g = texture.channels > 1 ? texture.data[pixel_index + 1]
                                     : texture.data[pixel_index];
      color.b = texture.channels > 2 ? texture.data[pixel_index + 2]
                                     : texture.data[pixel_index];
      color.a = texture.channels > 3 ? texture.data[pixel_index + 3] : 255;
    } else {
      // 安全保护：如果索引超出范围，返回白色
      color = Color4();
    }

    return color;
}

Material* MaterialLoader::getMaterial(const std::string& name) {
    auto it = materials_.find(name);
    if (it != materials_.end()) {
        return &it->second;
    }
    return nullptr;
}

Color4 MaterialLoader::calculateFinalColor(const Material& material, float u, float v) const {
    Color4 final_color;

    if (material.hasDiffuseTexture()) {
      final_color = sampleTexture(
          material.textures.at(material.diffuse_texture_path), u, v);
    } else {
      final_color.r = static_cast<uint8_t>(material.diffuse[0] * 255);
      final_color.g = static_cast<uint8_t>(material.diffuse[1] * 255);
      final_color.b = static_cast<uint8_t>(material.diffuse[2] * 255);
      final_color.a = static_cast<uint8_t>(material.opacity * 255);
    }

    if (!material.emissive_texture_path.empty()) {
        auto it = material.textures.find(material.emissive_texture_path);
        if (it != material.textures.end() && it->second.isValid()) {
            Color4 emissive = sampleTexture(it->second, u, v);
            final_color.r = std::min(
                255, final_color.r + static_cast<int>(emissive.r * 0.5));
            final_color.g = std::min(
                255, final_color.g + static_cast<int>(emissive.g * 0.5));
            final_color.b = std::min(
                255, final_color.b + static_cast<int>(emissive.b * 0.5));
        }
    } else if (material.emissive[0] > 0 || material.emissive[1] > 0 || material.emissive[2] > 0) {
      final_color.r = std::min(
          255, final_color.r + static_cast<int>(material.emissive[0] * 128));
      final_color.g = std::min(
          255, final_color.g + static_cast<int>(material.emissive[1] * 128));
      final_color.b = std::min(
          255, final_color.b + static_cast<int>(material.emissive[2] * 128));
    }

    if (!material.opacity_texture_path.empty()) {
        auto it = material.textures.find(material.opacity_texture_path);
        if (it != material.textures.end() && it->second.isValid()) {
            Color4 opacity = sampleTexture(it->second, u, v);
            final_color.a = opacity.r;  // Use red channel for grayscale opacity
        }
    }
    
    return final_color;
}

}  // namespace obj2blocks