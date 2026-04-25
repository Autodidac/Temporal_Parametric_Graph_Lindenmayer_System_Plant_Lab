#include "twol/io.hpp"

#include <fstream>
#include <string>

namespace twol {

namespace {

int preset_to_int(const PlantPreset preset) noexcept
{
    return static_cast<int>(preset);
}

int mode_to_int(const DimensionMode mode) noexcept
{
    return static_cast<int>(mode);
}

int style_to_int(const LeafStyle style) noexcept
{
    return static_cast<int>(style);
}

PlantPreset preset_from_int(const int value) noexcept
{
    if (value < 0 || value > 4) {
        return PlantPreset::Cannabis;
    }
    return static_cast<PlantPreset>(value);
}

DimensionMode mode_from_int(const int value) noexcept
{
    if (value < 0 || value > 3) {
        return DimensionMode::Plant3D;
    }
    return static_cast<DimensionMode>(value);
}

LeafStyle style_from_int(const int value) noexcept
{
    if (value < 0 || value > 4) {
        return LeafStyle::Fan;
    }
    return static_cast<LeafStyle>(value);
}

} // namespace

bool save_parameters(const PlantParameters& p, const std::filesystem::path& path)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "twol_version 6\n";
    out << "preset " << preset_to_int(p.preset) << '\n';
    out << "dimension_mode " << mode_to_int(p.dimension_mode) << '\n';
    out << "seed " << p.seed << '\n';
    out << "generations " << p.generations << '\n';
    out << "branch_count " << p.branch_count << '\n';
    out << "recursive_branch_depth " << p.recursive_branch_depth << '\n';
    out << "shoots_per_branch " << p.shoots_per_branch << '\n';
    out << "initial_sapling_nodes " << p.initial_sapling_nodes << '\n';
    out << "sapling_pre_age " << p.sapling_pre_age << '\n';
    out << "branch_activation_delay " << p.branch_activation_delay << '\n';
    out << "branch_growth_window " << p.branch_growth_window << '\n';
    out << "curve_segments " << p.curve_segments << '\n';
    out << "radial_segments " << p.radial_segments << '\n';
    out << "stem_length " << p.stem_length << '\n';
    out << "internode_scale " << p.internode_scale << '\n';
    out << "base_radius " << p.base_radius << '\n';
    out << "radius_decay " << p.radius_decay << '\n';
    out << "branch_length_scale " << p.branch_length_scale << '\n';
    out << "branch_angle_from_up_deg " << p.branch_angle_from_up_deg << '\n';
    out << "phyllotaxis_deg " << p.phyllotaxis_deg << '\n';
    out << "twist_jitter_deg " << p.twist_jitter_deg << '\n';
    out << "side_branch_upward_bend " << p.side_branch_upward_bend << '\n';
    out << "side_branch_bend_curve " << p.side_branch_bend_curve << '\n';
    out << "side_branch_outward_keep " << p.side_branch_outward_keep << '\n';
    out << "branch_sag " << p.branch_sag << '\n';
    out << "leaf_style " << style_to_int(p.leaf_style) << '\n';
    out << "fan_leaves_per_node " << p.fan_leaves_per_node << '\n';
    out << "leaflet_count " << p.leaflet_count << '\n';
    out << "leaflet_segments " << p.leaflet_segments << '\n';
    out << "leaf_scale " << p.leaf_scale << '\n';
    out << "leaf_width_ratio " << p.leaf_width_ratio << '\n';
    out << "leaf_pitch_down_deg " << p.leaf_pitch_down_deg << '\n';
    out << "leaf_outer_extra_drop_deg " << p.leaf_outer_extra_drop_deg << '\n';
    out << "leaf_droop " << p.leaf_droop << '\n';
    out << "fan_spread_deg " << p.fan_spread_deg << '\n';
    out << "petiole_length " << p.petiole_length << '\n';
    out << "leaf_node_density " << p.leaf_node_density << '\n';
    out << "time_seconds " << p.time_seconds << '\n';
    out << "full_growth_seconds " << p.full_growth_seconds << '\n';
    out << "leaf_delay " << p.leaf_delay << '\n';
    out << "leaf_grow_seconds " << p.leaf_grow_seconds << '\n';
    out << "show_node_markers " << (p.show_node_markers ? 1 : 0) << '\n';
    out << "wireframe " << (p.wireframe ? 1 : 0) << '\n';
    out << "leaf_veins " << (p.leaf_veins ? 1 : 0) << '\n';
    return true;
}

bool load_parameters(PlantParameters& p, const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    std::string key;
    while (in >> key) {
        if (key == "twol_version") { int ignored = 0; in >> ignored; }
        else if (key == "preset") { int v = 0; in >> v; p.preset = preset_from_int(v); }
        else if (key == "dimension_mode") { int v = 0; in >> v; p.dimension_mode = mode_from_int(v); }
        else if (key == "seed") { in >> p.seed; }
        else if (key == "generations") { in >> p.generations; }
        else if (key == "branch_count") { in >> p.branch_count; }
        else if (key == "recursive_branch_depth") { in >> p.recursive_branch_depth; }
        else if (key == "shoots_per_branch") { in >> p.shoots_per_branch; }
        else if (key == "initial_sapling_nodes") { in >> p.initial_sapling_nodes; }
        else if (key == "sapling_pre_age") { in >> p.sapling_pre_age; }
        else if (key == "branch_activation_delay") { in >> p.branch_activation_delay; }
        else if (key == "branch_growth_window") { in >> p.branch_growth_window; }
        else if (key == "curve_segments") { in >> p.curve_segments; }
        else if (key == "radial_segments") { in >> p.radial_segments; }
        else if (key == "stem_length") { in >> p.stem_length; }
        else if (key == "internode_scale") { in >> p.internode_scale; }
        else if (key == "base_radius") { in >> p.base_radius; }
        else if (key == "radius_decay") { in >> p.radius_decay; }
        else if (key == "branch_length_scale") { in >> p.branch_length_scale; }
        else if (key == "branch_angle_from_up_deg") { in >> p.branch_angle_from_up_deg; }
        else if (key == "phyllotaxis_deg") { in >> p.phyllotaxis_deg; }
        else if (key == "twist_jitter_deg") { in >> p.twist_jitter_deg; }
        else if (key == "side_branch_upward_bend") { in >> p.side_branch_upward_bend; }
        else if (key == "side_branch_bend_curve") { in >> p.side_branch_bend_curve; }
        else if (key == "side_branch_outward_keep") { in >> p.side_branch_outward_keep; }
        else if (key == "branch_sag") { in >> p.branch_sag; }
        else if (key == "leaf_style") { int v = 0; in >> v; p.leaf_style = style_from_int(v); }
        else if (key == "fan_leaves_per_node") { in >> p.fan_leaves_per_node; }
        else if (key == "leaflet_count") { in >> p.leaflet_count; }
        else if (key == "leaflet_segments") { in >> p.leaflet_segments; }
        else if (key == "leaf_scale") { in >> p.leaf_scale; }
        else if (key == "leaf_width_ratio") { in >> p.leaf_width_ratio; }
        else if (key == "leaf_pitch_down_deg") { in >> p.leaf_pitch_down_deg; }
        else if (key == "leaf_outer_extra_drop_deg") { in >> p.leaf_outer_extra_drop_deg; }
        else if (key == "leaf_droop") { in >> p.leaf_droop; }
        else if (key == "fan_spread_deg") { in >> p.fan_spread_deg; }
        else if (key == "petiole_length") { in >> p.petiole_length; }
        else if (key == "leaf_node_density") { in >> p.leaf_node_density; }
        else if (key == "time_seconds") { in >> p.time_seconds; }
        else if (key == "full_growth_seconds") { in >> p.full_growth_seconds; }
        else if (key == "leaf_delay") { in >> p.leaf_delay; }
        else if (key == "leaf_grow_seconds") { in >> p.leaf_grow_seconds; }
        else if (key == "show_node_markers") { int v = 0; in >> v; p.show_node_markers = v != 0; }
        else if (key == "wireframe") { int v = 0; in >> v; p.wireframe = v != 0; }
        else if (key == "leaf_veins") { int v = 0; in >> v; p.leaf_veins = v != 0; }
        else {
            std::string discard;
            std::getline(in, discard);
        }
    }

    return true;
}

bool export_obj(const Mesh& mesh, const std::filesystem::path& path)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "# TwoOL Procedural Plant Lab OBJ\n";
    for (const Vertex& v : mesh.vertices) {
        out << "v " << v.position.x << ' ' << v.position.y << ' ' << v.position.z << ' '
            << v.color.r << ' ' << v.color.g << ' ' << v.color.b << '\n';
    }
    for (const Vertex& v : mesh.vertices) {
        out << "vn " << v.normal.x << ' ' << v.normal.y << ' ' << v.normal.z << '\n';
    }
    for (const Vertex& v : mesh.vertices) {
        out << "vt " << v.uv.x << ' ' << v.uv.y << '\n';
    }
    for (std::size_t i = 0; i + 2U < mesh.indices.size(); i += 3U) {
        const std::uint32_t a = mesh.indices[i + 0U] + 1U;
        const std::uint32_t b = mesh.indices[i + 1U] + 1U;
        const std::uint32_t c = mesh.indices[i + 2U] + 1U;
        out << "f "
            << a << '/' << a << '/' << a << ' '
            << b << '/' << b << '/' << b << ' '
            << c << '/' << c << '/' << c << '\n';
    }

    return true;
}

bool export_graph_summary(const NodeGraph& graph, const std::filesystem::path& path)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "preset: " << to_string(graph.parameters.preset) << '\n';
    out << "dimension: " << to_string(graph.parameters.dimension_mode) << '\n';
    out << "seed: " << graph.parameters.seed << '\n';
    out << "generations: " << graph.parameters.generations << '\n';
    out << "branch_count: " << graph.parameters.branch_count << '\n';
    out << "nodes: " << graph.nodes.size() << '\n';
    out << "branches: " << graph.branches.size() << '\n';
    out << "leaf_clusters: " << graph.leaves.size() << '\n';
    return true;
}

} // namespace twol
