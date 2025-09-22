#include "json_exporter.h"
#include <fstream>
#include <iostream>
#include <climits>
#include <algorithm>
#include <map>

namespace obj2blocks {
    JsonExporter::JsonExporter() {
    }

    JsonExporter::~JsonExporter() {
    }

    bool JsonExporter::exportToFile(const std::string&filename,
                                    const std::vector<MinecraftCommand>&commands,
                                    const ConversionParams&params) {
        try {
            nlohmann::json json = createJson(commands, params);

            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
                return false;
            }

            file << json.dump(2);
            file.close();

            std::cout << "Successfully exported to: " << filename << std::endl;
            return true;
        }
        catch (const std::exception&e) {
            std::cerr << "Error exporting to JSON: " << e.what() << std::endl;
            return false;
        }
    }

    nlohmann::json JsonExporter::createJson(const std::vector<MinecraftCommand>&commands,
                                            const ConversionParams&params) {
        nlohmann::json json;

        json["model_info"]["source"] = params.input_file;
        json["model_info"]["target_size"] = params.target_size;
        json["model_info"]["voxel_size"] = params.voxel_size;
        json["model_info"]["scale_factor"] = params.scale_factor;
        json["model_info"]["auto_scale"] = params.auto_scale;
        json["model_info"]["solid_fill"] = params.solid;
        json["model_info"]["optimization_enabled"] = params.optimize;
        json["model_info"]["total_blocks"] = countTotalBlocks(commands);
        json["model_info"]["total_commands"] = commands.size();
        
        // Calculate bounding box
        int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
        int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;
        
        for (const auto& cmd : commands) {
            if (cmd.type == CommandType::CreateBlock) {
                minX = std::min(minX, cmd.position.x);
                minY = std::min(minY, cmd.position.y);
                minZ = std::min(minZ, cmd.position.z);
                maxX = std::max(maxX, cmd.position.x);
                maxY = std::max(maxY, cmd.position.y);
                maxZ = std::max(maxZ, cmd.position.z);
            } else {
                minX = std::min(minX, cmd.area.min.x);
                minY = std::min(minY, cmd.area.min.y);
                minZ = std::min(minZ, cmd.area.min.z);
                maxX = std::max(maxX, cmd.area.max.x);
                maxY = std::max(maxY, cmd.area.max.y);
                maxZ = std::max(maxZ, cmd.area.max.z);
            }
        }
        
        // Add bounding box info
        json["model_info"]["bounding_box"]["min"] = {minX, minY, minZ};
        json["model_info"]["bounding_box"]["max"] = {maxX, maxY, maxZ};
        json["model_info"]["bounding_box"]["size"] = {
            maxX - minX + 1,
            maxY - minY + 1,
            maxZ - minZ + 1
        };

        int fillarea_count = 0;
        int createblock_count = 0;

        nlohmann::json commands_array = nlohmann::json::array();
        for (const auto&cmd: commands) {
            commands_array.push_back(commandToJson(cmd));
            if (cmd.type == CommandType::FillArea) {
                fillarea_count++;
            }
            else {
                createblock_count++;
            }
        }

        // Compute duplicate blocks placed at the same position
        // We count how many extra placements happen on already-occupied positions.
        // Implementation note: uses std::map with Vec3i::operator< defined in types.h
        long long duplicate_blocks = 0;
        {
            std::map<Vec3i, int> freq;
            for (const auto& cmd : commands) {
                if (cmd.type == CommandType::CreateBlock) {
                    freq[cmd.position] += 1;
                } else { // FillArea
                    for (int x = cmd.area.min.x; x <= cmd.area.max.x; ++x) {
                        for (int y = cmd.area.min.y; y <= cmd.area.max.y; ++y) {
                            for (int z = cmd.area.min.z; z <= cmd.area.max.z; ++z) {
                                freq[{x, y, z}] += 1;
                            }
                        }
                    }
                }
            }
            for (const auto& kv : freq) {
                if (kv.second > 1) duplicate_blocks += static_cast<long long>(kv.second - 1);
            }
        }

        json["model_info"]["fillarea_commands"] = fillarea_count;
        json["model_info"]["createblock_commands"] = createblock_count;
        json["model_info"]["duplicate_blocks"] = duplicate_blocks;
        json["commands"] = commands_array;

        return json;
    }

    nlohmann::json JsonExporter::commandToJson(const MinecraftCommand&cmd) {
        nlohmann::json json_cmd;

        if (cmd.type == CommandType::CreateBlock) {
            json_cmd["type"] = "createblock";
            json_cmd["position"] = {cmd.position.x, cmd.position.y, cmd.position.z};
        }
        else {
            json_cmd["type"] = "fillarea";
            json_cmd["corner1"] = {cmd.area.min.x, cmd.area.min.y, cmd.area.min.z};
            json_cmd["corner2"] = {cmd.area.max.x, cmd.area.max.y, cmd.area.max.z};
        }
        
        // Add color information
        json_cmd["color"] = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};

        return json_cmd;
    }

    int JsonExporter::countTotalBlocks(const std::vector<MinecraftCommand>&commands) {
        int total = 0;
        for (const auto&cmd: commands) {
            if (cmd.type == CommandType::CreateBlock) {
                total += 1;
            }
            else {
                total += cmd.area.volume();
            }
        }
        return total;
    }
}
