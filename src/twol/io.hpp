#pragma once

#include <filesystem>

#include "twol/plant.hpp"

namespace twol {

bool save_parameters(const PlantParameters& params, const std::filesystem::path& path);
bool load_parameters(PlantParameters& params, const std::filesystem::path& path);
bool export_obj(const Mesh& mesh, const std::filesystem::path& path);
bool export_graph_summary(const NodeGraph& graph, const std::filesystem::path& path);

} // namespace twol
