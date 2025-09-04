#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "types.h"

namespace obj2blocks {
    class JsonExporter {
    public:
        JsonExporter();

        ~JsonExporter();

        bool exportToFile(const std::string&filename,
                          const std::vector<MinecraftCommand>&commands,
                          const ConversionParams&params);

        nlohmann::json createJson(const std::vector<MinecraftCommand>&commands,
                                  const ConversionParams&params);

    private:
        nlohmann::json commandToJson(const MinecraftCommand&cmd);

        int countTotalBlocks(const std::vector<MinecraftCommand>&commands);
    };
}
