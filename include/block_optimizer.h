#pragma once

#include <vector>
#include <set>
#include "types.h"

namespace obj2blocks {
    class BlockOptimizer {
    public:
        BlockOptimizer();

        ~BlockOptimizer();

        std::vector<MinecraftCommand> optimize(const std::set<Vec3i>&voxels);

        void setOptimizationEnabled(bool enabled) { optimization_enabled_ = enabled; }

        bool isOptimizationEnabled() const { return optimization_enabled_; }

    private:
        bool optimization_enabled_;

        std::vector<Box3i> findRectangularRegions(std::set<Vec3i> voxels);

        Box3i expandRegion(const Vec3i&start, std::set<Vec3i>&remaining_voxels);

        bool canFormBox(const Box3i&box, const std::set<Vec3i>&voxels);

        void removeBoxVoxels(const Box3i&box, std::set<Vec3i>&voxels);

        int calculateSavings(const Box3i&box);
    };
}
