cmake_minimum_required(VERSION 3.20)
project(Obj2Blocks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(pmp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(Stb REQUIRED)

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
        src/material_loader.cpp
        src/obj_loader.cpp
        src/ObjGenerator.cpp
)

# Headers
set(HEADERS
        include/mesh_processor.h
        include/voxelizer.h
        include/block_optimizer.h
        include/json_exporter.h
        include/types.h
        include/ObjGenerator.h
        include/material_loader.h
        include/obj_loader.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME}
        cxxopts::cxxopts
        pmp
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
)