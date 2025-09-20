cmake_minimum_required(VERSION 3.20)
project(Obj2Blocks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    find_package(OpenMP REQUIRED)
endif ()

find_package(cxxopts CONFIG REQUIRED)
find_package(pmp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
        src/main.cpp
        src/mesh_processor.cpp
        src/voxelizer.cpp
        src/block_optimizer.cpp
        src/json_exporter.cpp
)

# Headers
set(HEADERS
        include/mesh_processor.h
        include/voxelizer.h
        include/block_optimizer.h
        include/json_exporter.h
        include/types.h
        include/ObjGenerator.h
)

# Create executable
add_executable(obj2blocks ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(obj2blocks
        cxxopts::cxxopts
        pmp
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
)