#pragma once

#include <vector>
#include <set>
#include <map>
#include "types.h"

namespace obj2blocks {
    class BlockOptimizer {
    public:
        BlockOptimizer();

        ~BlockOptimizer();

        std::vector<MinecraftCommand> optimize(const std::set<Vec3i>&voxels);
        
        // New method with color support
        std::vector<MinecraftCommand> optimizeWithColors(const std::set<VoxelData>&voxels);

        void setOptimizationEnabled(bool enabled) { optimization_enabled_ = enabled; }

        bool isOptimizationEnabled() const { return optimization_enabled_; }

    private:
        bool optimization_enabled_;

        std::vector<Box3i> findRectangularRegions(std::set<Vec3i> voxels);
        
        // Color-aware optimization methods
        std::map<Color4, std::vector<Box3i>> findRectangularRegionsByColor(const std::set<VoxelData>& voxels);

        Box3i expandRegion(const Vec3i&start, std::set<Vec3i>&remaining_voxels);
        
        Box3i expandRegionWithColor(const Vec3i&start, const Color4& color, 
                                   std::set<Vec3i>&remaining_voxels);

        bool canFormBox(const Box3i&box, const std::set<Vec3i>&voxels);
        
        bool canFormBoxWithColor(const Box3i&box, const Color4& color, 
                                const std::set<Vec3i>&voxels);

        void removeBoxVoxels(const Box3i&box, std::set<Vec3i>&voxels);

        int calculateSavings(const Box3i&box);
    };
}
