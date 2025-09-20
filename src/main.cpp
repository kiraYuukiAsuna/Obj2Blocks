#include <iostream>
#include <string>
#include <fstream>
#include <cxxopts.hpp>

#include "mesh_processor.h"
#include "voxelizer.h"
#include "block_optimizer.h"
#include "json_exporter.h"
#include "types.h"
#include "ObjGenerator.h"

using namespace obj2blocks;

int obj2blocks_main(int argc, char* argv[]) {
    ConversionParams params;

    cxxopts::Options options("obj2blocks", "OBJ to Minecraft Blocks Converter");

    options.add_options()
            ("i,input", "Input OBJ file", cxxopts::value<std::string>())
            ("o,output", "Output JSON file", cxxopts::value<std::string>())
            ("s,size", "Target size for largest dimension", cxxopts::value<double>()->default_value("200"))
            ("v,voxel-size", "Voxel size", cxxopts::value<double>()->default_value("1.0"))
            ("scale", "Manual scale factor (disables auto-scale)", cxxopts::value<double>())
            ("surface", "Only voxelize surface (no interior fill)", cxxopts::value<bool>()->default_value("true"))
            ("optimize", "Enable fillarea optimization", cxxopts::value<bool>()->default_value("false"))
            ("with-texture", "Use texture mapping for block colors (if available)", cxxopts::value<bool>()->default_value("false"))
            ("h,help", "Show this help message");

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (!result.count("input") || !result.count("output")) {
            std::cerr << "Error: Input and output files are required.\n\n";
            std::cout << options.help() << std::endl;
            return 1;
        }

        params.input_file = result["input"].as<std::string>();
        params.output_file = result["output"].as<std::string>();
        params.target_size = result["size"].as<double>();
        params.voxel_size = result["voxel-size"].as<double>();

        if (result.count("scale")) {
            params.scale_factor = result["scale"].as<double>();
            params.auto_scale = false;
        }

        if (result.count("surface")) {
            params.solid = !result["surface"].as<bool>();
        }

        if (result.count("optimize")) {
            params.optimize = result["optimize"].as<bool>();
        }
        if (result.count("with-texture")) {
            params.with_texture = result["with-texture"].as<bool>();
        }
    }
    catch (const cxxopts::exceptions::exception&e) {
        std::cerr << "Error parsing options: " << e.what() << "\n\n";
        std::cout << options.help() << std::endl;
        return 1;
    }

    std::cout << "=== OBJ to Minecraft Blocks Converter ===" << std::endl;
    std::cout << "Input: " << params.input_file << std::endl;
    std::cout << "Output: " << params.output_file << std::endl;
    std::cout << "Target size: " << params.target_size << std::endl;
    std::cout << "Voxel size: " << params.voxel_size << std::endl;
    std::cout << "Fill mode: " << (params.solid ? "solid" : "surface") << std::endl;
    std::cout << "Optimization: " << (params.optimize ? "enabled" : "disabled") << std::endl;
    std::cout << "Texture mapping: " << (params.with_texture ? "enabled" : "disabled") << std::endl;
    std::cout << std::endl;

    MeshProcessor processor;
    std::cout << "Loading OBJ file..." << std::endl;
    if (!processor.loadOBJ(params.input_file)) {
        std::cerr << "Failed to load OBJ file." << std::endl;
        return 1;
    }

    processor.centerMesh();

    if (params.auto_scale) {
        processor.autoScale(params.target_size);
        pmp::Point min_pt, max_pt;
        processor.getBoundingBox(min_pt, max_pt);
        params.scale_factor = params.target_size / processor.getMaxDimension();
    }
    else {
        processor.scaleMesh(params.scale_factor);
    }

    Voxelizer voxelizer(params.voxel_size);
    std::cout << "\nStarting voxelization..." << std::endl;
    
    // Check if we have material information
    std::vector<MinecraftCommand> commands;
    size_t total_voxels = 0;
    
    if (processor.hasObjLoader() && params.with_texture) {
        // Use material-aware voxelization
        std::set<VoxelData> voxels_with_colors = voxelizer.voxelizeWithMaterials(processor, params.solid);
        
        if (voxels_with_colors.empty()) {
            std::cerr << "Error: No voxels generated from the model." << std::endl;
            return 1;
        }
        
        total_voxels = voxels_with_colors.size();
        
        BlockOptimizer optimizer;
        optimizer.setOptimizationEnabled(params.optimize);
        std::cout << "\nOptimizing block placement with colors..." << std::endl;
        commands = optimizer.optimizeWithColors(voxels_with_colors);
    } else {
        // Fallback to simple voxelization
        std::set<Vec3i> voxels = voxelizer.voxelize(processor.getMesh(), params.solid);
        
        if (voxels.empty()) {
            std::cerr << "Error: No voxels generated from the model." << std::endl;
            return 1;
        }
        
        total_voxels = voxels.size();
        
        BlockOptimizer optimizer;
        optimizer.setOptimizationEnabled(params.optimize);
        std::cout << "\nOptimizing block placement..." << std::endl;
        commands = optimizer.optimize(voxels);
    }

    JsonExporter exporter;
    std::cout << "\nExporting to JSON..." << std::endl;
    if (!exporter.exportToFile(params.output_file, commands, params)) {
        std::cerr << "Failed to export JSON file." << std::endl;
        return 1;
    }

    std::cout << "\n=== Conversion Complete ===" << std::endl;
    std::cout << "Total blocks: " << total_voxels << std::endl;
    std::cout << "Total commands: " << commands.size() << std::endl;

    if (params.optimize) {
        double reduction = 100.0 * (1.0 - (double)commands.size() / total_voxels);
        std::cout << "Command reduction: " << reduction << "%" << std::endl;
    }

    return 0;
}


int json2obj_main(int argc, char* argv[]) {
    std::string inputFile, outputFile;

    cxxopts::Options options("json2obj", "JSON to OBJ Converter");

    options.add_options()
            ("i,input", "Input JSON file", cxxopts::value<std::string>())
            ("o,output", "Output OBJ file", cxxopts::value<std::string>())
            ("h,help", "Show this help message");

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (!result.count("input") || !result.count("output")) {
            std::cerr << "Error: Input and output files are required.\n\n";
            std::cout << options.help() << std::endl;
            return 1;
        }

        inputFile = result["input"].as<std::string>();
        outputFile = result["output"].as<std::string>();
    }
    catch (const cxxopts::exceptions::exception&e) {
        std::cerr << "Error parsing options: " << e.what() << "\n\n";
        std::cout << options.help() << std::endl;
        return 1;
    }

    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open input file " << inputFile << std::endl;
        return 1;
    }

    json root;
    try {
        file >> root;
    }
    catch (const std::exception&e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return 1;
    }
    file.close();

    ObjGenerator generator;

    if (root.contains("commands") && root["commands"].is_array()) {
        for (const auto&command: root["commands"]) {
            generator.processCommand(command);
        }
    }
    else {
        std::cerr << "Error: JSON must contain 'commands' array" << std::endl;
        return 1;
    }

    generator.writeToFile(outputFile);

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode> [options]\n";
        std::cerr << "\nModes:\n";
        std::cerr << "  obj2json    Convert OBJ file to Minecraft commands JSON\n";
        std::cerr << "  json2obj    Convert JSON commands to OBJ file\n";
        std::cerr << "\nUse '<mode> --help' for mode-specific options\n";
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "obj2json") {
        return obj2blocks_main(argc - 1, argv + 1);
    }
    else if (mode == "json2obj") {
        return json2obj_main(argc - 1, argv + 1);
    }
    else if (mode == "--help" || mode == "-h") {
        std::cout << "Usage: " << argv[0] << " <mode> [options]\n";
        std::cout << "\nModes:\n";
        std::cout << "  obj2json    Convert OBJ file to Minecraft commands JSON\n";
        std::cout << "  json2obj    Convert JSON commands to OBJ file\n";
        std::cout << "\nUse '<mode> --help' for mode-specific options\n";
        return 0;
    }
    else {
        std::cerr << "Error: Unknown mode '" << mode << "'\n";
        std::cerr << "Use '" << argv[0] << " --help' for usage information\n";
        return 1;
    }
}
