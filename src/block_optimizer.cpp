#include "block_optimizer.h"
#include <iostream>

namespace obj2blocks {
    BlockOptimizer::BlockOptimizer() : optimization_enabled_(true) {
    }

    BlockOptimizer::~BlockOptimizer() {
    }

    std::vector<MinecraftCommand> BlockOptimizer::optimize(const std::set<Vec3i>&voxels) {
        std::vector<MinecraftCommand> commands;

        if (!optimization_enabled_ || voxels.empty()) {
            for (const auto&voxel: voxels) {
                commands.emplace_back(voxel, Color4());  // Default color
            }
            return commands;
        }

        std::cout << "Optimizing " << voxels.size() << " blocks..." << std::endl;

        std::set<Vec3i> remaining_voxels = voxels;
        std::vector<Box3i> regions = findRectangularRegions(remaining_voxels);

        for (const auto&region: regions) {
            commands.emplace_back(region, Color4());  // Default color
        }

        for (const auto&voxel: remaining_voxels) {
            commands.emplace_back(voxel, Color4());  // Default color
        }

        std::cout << "Optimized to " << commands.size() << " commands" << std::endl;
        std::cout << "  - FillArea commands: " << regions.size() << std::endl;
        std::cout << "  - CreateBlock commands: " << remaining_voxels.size() << std::endl;

        return commands;
    }

    std::vector<Box3i> BlockOptimizer::findRectangularRegions(std::set<Vec3i> voxels) {
        std::vector<Box3i> regions;

        while (!voxels.empty()) {
            Vec3i start = *voxels.begin();
            Box3i region = expandRegion(start, voxels);

            if (calculateSavings(region) > 0) {
                regions.push_back(region);
                removeBoxVoxels(region, voxels);
            }
            else {
                voxels.erase(start);
            }
        }

        return regions;
    }

    Box3i BlockOptimizer::expandRegion(const Vec3i&start, std::set<Vec3i>&remaining_voxels) {
        Box3i best_box(start, start);
        int best_savings = 0;

        for (int dx = 0; dx <= 20; ++dx) {
            for (int dy = 0; dy <= 20; ++dy) {
                for (int dz = 0; dz <= 20; ++dz) {
                    Box3i candidate(start, Vec3i(start.x + dx, start.y + dy, start.z + dz));

                    if (canFormBox(candidate, remaining_voxels)) {
                        int savings = calculateSavings(candidate);
                        if (savings > best_savings) {
                            best_box = candidate;
                            best_savings = savings;
                        }
                    }
                }
            }
        }

        return best_box;
    }

    bool BlockOptimizer::canFormBox(const Box3i&box, const std::set<Vec3i>&voxels) {
        for (int x = box.min.x; x <= box.max.x; ++x) {
            for (int y = box.min.y; y <= box.max.y; ++y) {
                for (int z = box.min.z; z <= box.max.z; ++z) {
                    if (voxels.find(Vec3i(x, y, z)) == voxels.end()) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void BlockOptimizer::removeBoxVoxels(const Box3i&box, std::set<Vec3i>&voxels) {
        for (int x = box.min.x; x <= box.max.x; ++x) {
            for (int y = box.min.y; y <= box.max.y; ++y) {
                for (int z = box.min.z; z <= box.max.z; ++z) {
                    voxels.erase(Vec3i(x, y, z));
                }
            }
        }
    }

    int BlockOptimizer::calculateSavings(const Box3i&box) {
        int volume = box.volume();
        return volume - 1;
    }
    
    std::vector<MinecraftCommand> BlockOptimizer::optimizeWithColors(const std::set<VoxelData>&voxels) {
        std::vector<MinecraftCommand> commands;
        
        if (!optimization_enabled_ || voxels.empty()) {
            for (const auto&voxel: voxels) {
                commands.emplace_back(voxel.position, voxel.color);
            }
            return commands;
        }
        
        std::cout << "Optimizing " << voxels.size() << " colored blocks..." << std::endl;
        
        // Group voxels by color
        std::map<Color4, std::set<Vec3i>> voxels_by_color;
        for (const auto& vd : voxels) {
            voxels_by_color[vd.color].insert(vd.position);
        }
        
        std::cout << "Found " << voxels_by_color.size() << " unique colors" << std::endl;
        
        int total_fillarea = 0;
        int total_createblock = 0;
        
        // Optimize each color group separately
        for (const auto& [color, color_voxels] : voxels_by_color) {
            std::set<Vec3i> remaining = color_voxels;
            std::vector<Box3i> regions = findRectangularRegions(remaining);
            
            for (const auto& region : regions) {
                commands.emplace_back(region, color);
                total_fillarea++;
            }
            
            for (const auto& voxel : remaining) {
                commands.emplace_back(voxel, color);
                total_createblock++;
            }
        }
        
        std::cout << "Optimized to " << commands.size() << " commands" << std::endl;
        std::cout << "  - FillArea commands: " << total_fillarea << std::endl;
        std::cout << "  - CreateBlock commands: " << total_createblock << std::endl;
        
        return commands;
    }
}
