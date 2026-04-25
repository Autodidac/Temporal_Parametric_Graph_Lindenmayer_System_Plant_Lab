#include <filesystem>
#include <iostream>
#include <string>

#include "twol/twol.hpp"

namespace {

void export_preset(const std::filesystem::path& out_dir, twol::PlantPreset preset, std::uint32_t seed, twol::DimensionMode mode)
{
    twol::PlantParameters params = twol::make_preset(preset);
    params.seed = seed;
    params.dimension_mode = mode;
    params.time_seconds = params.full_growth_seconds;
    params.show_node_markers = false;

    twol::NodeGraph graph = twol::generate_node_graph(params);
    twol::Mesh mesh = twol::build_mesh(graph, twol::timeline01(params));

    const std::string base = std::string(twol::to_string(preset));
    std::string safe;
    for (char c : base) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            safe += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (!safe.empty() && safe.back() != '_') {
            safe += '_';
        }
    }
    if (safe.empty()) {
        safe = "plant";
    }

    const auto obj_path = out_dir / (safe + ".obj");
    const auto twol_path = out_dir / (safe + ".twol");
    const auto summary_path = out_dir / (safe + "_summary.txt");

    twol::export_obj(mesh, obj_path);
    twol::save_parameters(params, twol_path);
    twol::export_graph_summary(graph, summary_path);

    std::cout << "[TwoOL] " << twol::to_string(preset)
              << " seed=" << params.seed
              << " mode=" << twol::to_string(params.dimension_mode)
              << " nodes=" << graph.nodes.size()
              << " branches=" << graph.branches.size()
              << " leaves=" << graph.leaves.size()
              << " vertices=" << mesh.vertices.size()
              << " indices=" << mesh.indices.size()
              << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    std::filesystem::path out_dir = argc >= 2 ? std::filesystem::path(argv[1]) : std::filesystem::current_path() / "out";
    std::filesystem::create_directories(out_dir);

    export_preset(out_dir, twol::PlantPreset::Cannabis, 1337U, twol::DimensionMode::Plant3D);
    export_preset(out_dir, twol::PlantPreset::Tree, 2401U, twol::DimensionMode::Plant3D);
    export_preset(out_dir, twol::PlantPreset::Bush, 99U, twol::DimensionMode::Plant3D);
    export_preset(out_dir, twol::PlantPreset::Fern, 404U, twol::DimensionMode::Plant25D);
    export_preset(out_dir, twol::PlantPreset::Pattern2D, 2026U, twol::DimensionMode::Pattern2D);

    std::cout << "[TwoOL] Export complete: " << out_dir.string() << '\n';
    return 0;
}
