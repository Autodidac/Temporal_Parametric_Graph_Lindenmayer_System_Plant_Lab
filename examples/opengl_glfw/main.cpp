#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "twol/twol.hpp"

namespace {

struct ViewerState {
    twol::PlantParameters params = twol::make_preset(twol::PlantPreset::Cannabis);
    twol::NodeGraph graph;
    twol::Mesh mesh;

    bool graph_dirty = true;
    bool mesh_dirty = true;
    bool paused = false;
    float playback_direction = 1.0f;
    float playback_speed = 1.0f;

    float yaw = 34.0f;
    float pitch = 23.0f;
    float distance = 8.5f;
    twol::Vec3 target{0.0f, 2.3f, 0.0f};

    bool dragging = false;
    double last_x = 0.0;
    double last_y = 0.0;

    double mouse_x = 0.0;
    double mouse_y = 0.0;
    bool mouse_down = false;
    bool mouse_pressed = false;
    bool mouse_released = false;
    int active_gui_control = -1;

    GLuint leaf_texture = 0U;
    bool leaf_texture_loaded = false;
    bool leaf_texture_enabled = true;
    int leaf_atlas_region = 0;

    std::filesystem::path root = std::filesystem::current_path();
};

struct AtlasRegion {
    const char* name;
    float u0;
    float v0;
    float u1;
    float v1;
};

constexpr float atlas_size = 2048.0f;
constexpr AtlasRegion make_region(const char* name, const float x0, const float y0, const float x1, const float y1) noexcept
{
    return AtlasRegion{
        name,
        x0 / atlas_size,
        1.0f - (y1 / atlas_size),
        x1 / atlas_size,
        1.0f - (y0 / atlas_size),
    };
}

constexpr std::array<AtlasRegion, 8> kAtlasRegions{
    make_region("cannabis single", 0.0f, 1536.0f, 512.0f, 2048.0f),
    make_region("cannabis cola", 512.0f, 1536.0f, 1024.0f, 2048.0f),
    make_region("broad maple", 512.0f, 0.0f, 1024.0f, 512.0f),
    make_region("broad serrated", 1024.0f, 0.0f, 1536.0f, 512.0f),
    make_region("oak", 0.0f, 0.0f, 512.0f, 512.0f),
    make_region("ivy", 0.0f, 512.0f, 512.0f, 1024.0f),
    make_region("fern", 1536.0f, 512.0f, 2048.0f, 1024.0f),
    make_region("needle", 1536.0f, 0.0f, 2048.0f, 512.0f),
};

const AtlasRegion& selected_atlas_region(const ViewerState& state) noexcept
{
    const int count = static_cast<int>(kAtlasRegions.size());
    const int wrapped = ((state.leaf_atlas_region % count) + count) % count;
    return kAtlasRegions[static_cast<std::size_t>(wrapped)];
}

void cycle_atlas_region(ViewerState& state, const int delta) noexcept
{
    const int count = static_cast<int>(kAtlasRegions.size());
    state.leaf_atlas_region = ((state.leaf_atlas_region + delta) % count + count) % count;
}

[[nodiscard]] twol::Vec2 atlas_uv(const ViewerState& state, const twol::Vec2 uv) noexcept
{
    const AtlasRegion& r = selected_atlas_region(state);
    return {
        r.u0 + std::clamp(uv.x, 0.0f, 1.0f) * (r.u1 - r.u0),
        r.v0 + std::clamp(uv.y, 0.0f, 1.0f) * (r.v1 - r.v0),
    };
}

void clamp_params(twol::PlantParameters& p)
{
    p.generations = std::clamp(p.generations, 1, 14);
    p.branch_count = std::clamp(p.branch_count, 1, 12);
    p.recursive_branch_depth = std::clamp(p.recursive_branch_depth, 0, 4);
    p.shoots_per_branch = std::clamp(p.shoots_per_branch, 0, 4);
    p.initial_sapling_nodes = std::clamp(p.initial_sapling_nodes, 0, 12);
    p.sapling_pre_age = std::clamp(p.sapling_pre_age, 0.0f, 0.75f);
    p.branch_activation_delay = std::clamp(p.branch_activation_delay, 0.0f, 0.30f);
    p.branch_growth_window = std::clamp(p.branch_growth_window, 0.08f, 0.90f);
    p.curve_segments = std::clamp(p.curve_segments, 2, 16);
    p.radial_segments = std::clamp(p.radial_segments, 4, 16);
    p.branch_angle_from_up_deg = std::clamp(p.branch_angle_from_up_deg, 5.0f, 85.0f);
    p.side_branch_upward_bend = std::clamp(p.side_branch_upward_bend, 0.0f, 1.0f);
    p.side_branch_bend_curve = std::clamp(p.side_branch_bend_curve, 0.08f, 2.0f);
    p.side_branch_outward_keep = std::clamp(p.side_branch_outward_keep, 0.0f, 1.0f);
    p.leaflet_count = std::clamp(p.leaflet_count, 1, 15);
    if ((p.leaflet_count % 2) == 0) {
        ++p.leaflet_count;
    }
    p.leaflet_segments = std::clamp(p.leaflet_segments, 1, 12);
    p.leaf_scale = std::clamp(p.leaf_scale, 0.05f, 8.0f);
    p.leaf_droop = std::clamp(p.leaf_droop, 0.0f, 1.4f);
    p.leaf_pitch_down_deg = std::clamp(p.leaf_pitch_down_deg, -15.0f, 85.0f);
    p.fan_leaves_per_node = std::clamp(p.fan_leaves_per_node, 1, 12);
    p.leaf_node_density = std::clamp(p.leaf_node_density, 0.0f, 1.0f);
    p.full_growth_seconds = std::clamp(p.full_growth_seconds, 1.0f, 120.0f);
    p.time_seconds = std::clamp(p.time_seconds, 0.0f, p.full_growth_seconds);
}

void rebuild_if_needed(ViewerState& state)
{
    if (state.graph_dirty) {
        clamp_params(state.params);
        state.graph = twol::generate_node_graph(state.params);
        state.graph_dirty = false;
        state.mesh_dirty = true;
    }

    if (state.mesh_dirty) {
        state.mesh = twol::build_mesh(state.graph, twol::timeline01(state.params));
        state.mesh_dirty = false;
    }
}

void print_controls()
{
    std::cout << "\nTwoOL Procedural Plant Lab controls\n";
    std::cout << "-----------------------------------\n";
    std::cout << "1-5       presets: cannabis/tree/bush/fern/pattern\n";
    std::cout << "M         cycle dimensions: 3D / 2.5D / 2D / pattern\n";
    std::cout << "Space     pause/resume\n";
    std::cout << "R         reverse playback\n";
    std::cout << "T         reset timeline\n";
    std::cout << "Left/Right scrub timeline\n";
    std::cout << "- / =     speed down/up\n";
    std::cout << "G         regenerate current seed\n";
    std::cout << "N         new seed\n";
    std::cout << "[ / ]     generations down/up\n";
    std::cout << "B / V     branch count down/up\n";
    std::cout << "A / Z     branch angle from vertical down/up\n";
    std::cout << "U / J     upward branch bend up/down\n";
    std::cout << "O / P     leaf scale down/up\n";
    std::cout << "D / F     leaf droop down/up\n";
    std::cout << "K / I     leaflet count down/up\n";
    std::cout << "W         wireframe\n";
    std::cout << "C         node markers\n";
    std::cout << "Y         leaf veins\n";
    std::cout << "Q / E     atlas region previous/next\n";
    std::cout << "S         save state to out/current.twol\n";
    std::cout << "L         load state from out/current.twol\n";
    std::cout << "X         export OBJ to out/current.obj\n";
    std::cout << "Mouse     drag orbit, wheel zoom\n";
    std::cout << "Esc       quit\n\n";
}

void update_title(GLFWwindow* window, ViewerState& state)
{
    std::ostringstream title;
    title << "TwoOL Plant Lab | "
          << twol::to_string(state.params.preset) << " | "
          << twol::to_string(state.params.dimension_mode) << " | seed " << state.params.seed
          << " | gen " << state.params.generations
          << " | branches " << state.params.branch_count
          << " | t " << static_cast<int>(twol::timeline01(state.params) * 100.0f) << "%"
          << " | atlas " << selected_atlas_region(state).name
          << " | verts " << state.mesh.vertices.size();
    glfwSetWindowTitle(window, title.str().c_str());
}

bool read_ppm_token(std::istream& in, std::string& out)
{
    out.clear();
    char c = 0;
    while (in.get(c)) {
        if (c == '#') {
            std::string ignored;
            std::getline(in, ignored);
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(c);
            break;
        }
    }
    while (in.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            break;
        }
        out.push_back(c);
    }
    return !out.empty();
}

bool load_leaf_texture(ViewerState& state, const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        state.leaf_texture_loaded = false;
        return false;
    }

    std::string token;
    if (!read_ppm_token(in, token) || token != "P6") {
        state.leaf_texture_loaded = false;
        return false;
    }
    if (!read_ppm_token(in, token)) { state.leaf_texture_loaded = false; return false; }
    const int width = std::stoi(token);
    if (!read_ppm_token(in, token)) { state.leaf_texture_loaded = false; return false; }
    const int height = std::stoi(token);
    if (!read_ppm_token(in, token)) { state.leaf_texture_loaded = false; return false; }
    const int max_value = std::stoi(token);
    if (width <= 0 || height <= 0 || max_value != 255) {
        state.leaf_texture_loaded = false;
        return false;
    }

    std::vector<unsigned char> rgb(static_cast<std::size_t>(width * height * 3));
    in.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    if (in.gcount() != static_cast<std::streamsize>(rgb.size())) {
        state.leaf_texture_loaded = false;
        return false;
    }

    std::vector<unsigned char> rgba(static_cast<std::size_t>(width * height * 4));
    for (int i = 0; i < width * height; ++i) {
        const unsigned char r = rgb[static_cast<std::size_t>(i * 3 + 0)];
        const unsigned char g = rgb[static_cast<std::size_t>(i * 3 + 1)];
        const unsigned char b = rgb[static_cast<std::size_t>(i * 3 + 2)];
        const bool checker_white =
            r > 168 && g > 168 && b > 168 &&
            std::abs(static_cast<int>(r) - static_cast<int>(g)) < 28 &&
            std::abs(static_cast<int>(g) - static_cast<int>(b)) < 28;
        const bool checker_gray =
            r > 118 && g > 118 && b > 118 &&
            r < 205 && g < 205 && b < 205 &&
            std::abs(static_cast<int>(r) - static_cast<int>(g)) < 18 &&
            std::abs(static_cast<int>(g) - static_cast<int>(b)) < 18;
        const bool near_background = checker_white || checker_gray;
        rgba[static_cast<std::size_t>(i * 4 + 0)] = r;
        rgba[static_cast<std::size_t>(i * 4 + 1)] = g;
        rgba[static_cast<std::size_t>(i * 4 + 2)] = b;
        rgba[static_cast<std::size_t>(i * 4 + 3)] = near_background ? 0U : 255U;
    }

    if (state.leaf_texture == 0U) {
        glGenTextures(1, &state.leaf_texture);
    }
    glBindTexture(GL_TEXTURE_2D, state.leaf_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0U);
    state.leaf_texture_loaded = true;
    std::cout << "[TwoOL] Loaded leaf texture: " << path.string() << '\n';
    return true;
}

void save_state(const ViewerState& state)
{
    const auto out_dir = state.root / "out";
    std::filesystem::create_directories(out_dir);
    const auto path = out_dir / "current.twol";
    if (twol::save_parameters(state.params, path)) {
        std::cout << "[TwoOL] Saved " << path.string() << '\n';
    } else {
        std::cout << "[TwoOL] ERROR: failed to save " << path.string() << '\n';
    }
}

void load_state(ViewerState& state)
{
    const auto path = state.root / "out" / "current.twol";
    twol::PlantParameters loaded = state.params;
    if (twol::load_parameters(loaded, path)) {
        state.params = loaded;
        state.graph_dirty = true;
        state.mesh_dirty = true;
        std::cout << "[TwoOL] Loaded " << path.string() << '\n';
    } else {
        std::cout << "[TwoOL] ERROR: failed to load " << path.string() << '\n';
    }
}

void export_current_obj(ViewerState& state)
{
    rebuild_if_needed(state);
    const auto out_dir = state.root / "out";
    std::filesystem::create_directories(out_dir);
    const auto obj_path = out_dir / "current.obj";
    const auto txt_path = out_dir / "current_graph.txt";
    const bool obj_ok = twol::export_obj(state.mesh, obj_path);
    const bool summary_ok = twol::export_graph_summary(state.graph, txt_path);
    std::cout << "[TwoOL] OBJ export " << (obj_ok ? "OK: " : "FAILED: ") << obj_path.string() << '\n';
    if (summary_ok) {
        std::cout << "[TwoOL] Summary: " << txt_path.string() << '\n';
    }
}

void set_preset(ViewerState& state, const twol::PlantPreset preset)
{
    const std::uint32_t old_seed = state.params.seed;
    const twol::DimensionMode old_mode = state.params.dimension_mode;
    state.params = twol::make_preset(preset);
    state.params.seed = old_seed;
    if (preset == twol::PlantPreset::Pattern2D) {
        state.params.dimension_mode = twol::DimensionMode::Pattern2D;
    } else if (old_mode != twol::DimensionMode::Pattern2D) {
        state.params.dimension_mode = old_mode;
    }

    switch (preset) {
    case twol::PlantPreset::Cannabis:
        state.leaf_atlas_region = 0;
        break;
    case twol::PlantPreset::Tree:
        state.leaf_atlas_region = 3;
        break;
    case twol::PlantPreset::Bush:
        state.leaf_atlas_region = 5;
        break;
    case twol::PlantPreset::Fern:
        state.leaf_atlas_region = 6;
        break;
    case twol::PlantPreset::Pattern2D:
        break;
    }
    state.graph_dirty = true;
}

void cycle_dimension(ViewerState& state)
{
    using twol::DimensionMode;
    if (state.params.preset == twol::PlantPreset::Pattern2D) {
        state.params.dimension_mode = DimensionMode::Pattern2D;
        state.graph_dirty = true;
        return;
    }
    switch (state.params.dimension_mode) {
    case DimensionMode::Plant3D:
        state.params.dimension_mode = DimensionMode::Plant25D;
        break;
    case DimensionMode::Plant25D:
        state.params.dimension_mode = DimensionMode::Plant2D;
        break;
    case DimensionMode::Plant2D:
    case DimensionMode::Pattern2D:
        state.params.dimension_mode = DimensionMode::Plant3D;
        break;
    }
    state.graph_dirty = true;
}

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }

    bool graph_changed = false;
    bool mesh_changed = false;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_1:
        set_preset(*state, twol::PlantPreset::Cannabis);
        break;
    case GLFW_KEY_2:
        set_preset(*state, twol::PlantPreset::Tree);
        break;
    case GLFW_KEY_3:
        set_preset(*state, twol::PlantPreset::Bush);
        break;
    case GLFW_KEY_4:
        set_preset(*state, twol::PlantPreset::Fern);
        break;
    case GLFW_KEY_5:
        set_preset(*state, twol::PlantPreset::Pattern2D);
        break;
    case GLFW_KEY_M:
        cycle_dimension(*state);
        break;
    case GLFW_KEY_SPACE:
        state->paused = !state->paused;
        break;
    case GLFW_KEY_R:
        state->playback_direction *= -1.0f;
        break;
    case GLFW_KEY_T:
        state->params.time_seconds = 0.0f;
        mesh_changed = true;
        break;
    case GLFW_KEY_RIGHT:
        state->params.time_seconds += state->params.full_growth_seconds * 0.025f;
        mesh_changed = true;
        break;
    case GLFW_KEY_LEFT:
        state->params.time_seconds -= state->params.full_growth_seconds * 0.025f;
        mesh_changed = true;
        break;
    case GLFW_KEY_EQUAL:
        state->playback_speed = std::min(8.0f, state->playback_speed * 1.25f);
        break;
    case GLFW_KEY_MINUS:
        state->playback_speed = std::max(0.05f, state->playback_speed / 1.25f);
        break;
    case GLFW_KEY_G:
        graph_changed = true;
        break;
    case GLFW_KEY_N:
        state->params.seed = state->params.seed * 1664525U + 1013904223U;
        graph_changed = true;
        break;
    case GLFW_KEY_LEFT_BRACKET:
        --state->params.generations;
        graph_changed = true;
        break;
    case GLFW_KEY_RIGHT_BRACKET:
        ++state->params.generations;
        graph_changed = true;
        break;
    case GLFW_KEY_B:
        --state->params.branch_count;
        graph_changed = true;
        break;
    case GLFW_KEY_V:
        ++state->params.branch_count;
        graph_changed = true;
        break;
    case GLFW_KEY_A:
        state->params.branch_angle_from_up_deg -= 3.0f;
        graph_changed = true;
        break;
    case GLFW_KEY_Z:
        state->params.branch_angle_from_up_deg += 3.0f;
        graph_changed = true;
        break;
    case GLFW_KEY_U:
        state->params.side_branch_upward_bend += 0.04f;
        graph_changed = true;
        break;
    case GLFW_KEY_J:
        state->params.side_branch_upward_bend -= 0.04f;
        graph_changed = true;
        break;
    case GLFW_KEY_O:
        state->params.leaf_scale -= 0.12f;
        graph_changed = true;
        break;
    case GLFW_KEY_P:
        state->params.leaf_scale += 0.12f;
        graph_changed = true;
        break;
    case GLFW_KEY_D:
        state->params.leaf_droop -= 0.04f;
        graph_changed = true;
        break;
    case GLFW_KEY_F:
        state->params.leaf_droop += 0.04f;
        graph_changed = true;
        break;
    case GLFW_KEY_K:
        state->params.leaflet_count -= 2;
        graph_changed = true;
        break;
    case GLFW_KEY_I:
        state->params.leaflet_count += 2;
        graph_changed = true;
        break;
    case GLFW_KEY_H:
        --state->params.fan_leaves_per_node;
        graph_changed = true;
        break;
    case GLFW_KEY_L:
        ++state->params.fan_leaves_per_node;
        graph_changed = true;
        break;
    case GLFW_KEY_Q:
        cycle_atlas_region(*state, -1);
        break;
    case GLFW_KEY_E:
        cycle_atlas_region(*state, 1);
        break;
    case GLFW_KEY_W:
        state->params.wireframe = !state->params.wireframe;
        break;
    case GLFW_KEY_Y:
        state->params.leaf_veins = !state->params.leaf_veins;
        mesh_changed = true;
        break;
    case GLFW_KEY_C:
        state->params.show_node_markers = !state->params.show_node_markers;
        state->graph.parameters.show_node_markers = state->params.show_node_markers;
        mesh_changed = true;
        break;
    case GLFW_KEY_S:
        save_state(*state);
        break;
    case GLFW_KEY_SEMICOLON:
        load_state(*state);
        break;
    case GLFW_KEY_X:
        export_current_obj(*state);
        break;
    default:
        break;
    }

    clamp_params(state->params);
    if (graph_changed) {
        state->graph_dirty = true;
    }
    if (mesh_changed) {
        state->mesh_dirty = true;
    }
}

bool is_over_gui_area(const double x, const double y) noexcept
{
    return x >= 8.0 && x <= 412.0 && y >= 8.0 && y <= 1128.0;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int)
{
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr || button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    glfwGetCursorPos(window, &state->mouse_x, &state->mouse_y);
    state->last_x = state->mouse_x;
    state->last_y = state->mouse_y;

    if (action == GLFW_PRESS) {
        state->mouse_down = true;
        state->mouse_pressed = true;
        state->mouse_released = false;
        if (!is_over_gui_area(state->mouse_x, state->mouse_y)) {
            state->dragging = true;
        }
    } else if (action == GLFW_RELEASE) {
        state->mouse_down = false;
        state->mouse_pressed = false;
        state->mouse_released = true;
        state->dragging = false;
        state->active_gui_control = -1;
    }
}

void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }

    state->mouse_x = x;
    state->mouse_y = y;

    if (!state->dragging) {
        return;
    }

    const double dx = x - state->last_x;
    const double dy = y - state->last_y;
    state->last_x = x;
    state->last_y = y;

    state->yaw += static_cast<float>(dx) * 0.22f;
    state->pitch += static_cast<float>(dy) * 0.18f;
    state->pitch = std::clamp(state->pitch, -82.0f, 82.0f);
}

void scroll_callback(GLFWwindow* window, double, double yoffset)
{
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }
    const float factor = yoffset > 0.0 ? 0.88f : 1.14f;
    state->distance = std::clamp(state->distance * factor, 1.25f, 80.0f);
}

void perspective(const float fovy_deg, const float aspect, const float z_near, const float z_far)
{
    const float fH = std::tan(fovy_deg * twol::deg_to_rad * 0.5f) * z_near;
    const float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, z_near, z_far);
}

void draw_grid(const float size, const int divisions)
{
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glColor4f(0.18f, 0.20f, 0.20f, 1.0f);
    glBegin(GL_LINES);
    for (int i = -divisions; i <= divisions; ++i) {
        const float v = size * static_cast<float>(i) / static_cast<float>(divisions);
        glVertex3f(-size, 0.0f, v);
        glVertex3f(size, 0.0f, v);
        glVertex3f(v, 0.0f, -size);
        glVertex3f(v, 0.0f, size);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

bool is_leaf_vertex(const twol::Vertex& v) noexcept
{
    return v.color.g > v.color.r * 1.20f && v.color.g > v.color.b * 1.45f;
}

void draw_mesh_pass(const twol::Mesh& mesh, const ViewerState& state, const bool want_leaf_triangles, const bool textured)
{
    glBegin(GL_TRIANGLES);
    for (std::size_t i = 0; i + 2U < mesh.indices.size(); i += 3U) {
        const std::uint32_t i0 = mesh.indices[i + 0U];
        const std::uint32_t i1 = mesh.indices[i + 1U];
        const std::uint32_t i2 = mesh.indices[i + 2U];
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            continue;
        }
        const twol::Vertex& a = mesh.vertices[i0];
        const twol::Vertex& b = mesh.vertices[i1];
        const twol::Vertex& c = mesh.vertices[i2];
        const bool leaf_triangle = is_leaf_vertex(a) && is_leaf_vertex(b) && is_leaf_vertex(c);
        if (leaf_triangle != want_leaf_triangles) {
            continue;
        }
        const twol::Vertex* verts[3] = {&a, &b, &c};
        for (const twol::Vertex* v : verts) {
            if (textured) {
                glColor4f(1.0f, 1.0f, 1.0f, v->color.a);
            } else {
                glColor4f(v->color.r, v->color.g, v->color.b, v->color.a);
            }
            glNormal3f(v->normal.x, v->normal.y, v->normal.z);
            if (textured) {
                const twol::Vec2 tuv = atlas_uv(state, v->uv);
                glTexCoord2f(tuv.x, tuv.y);
            }
            glVertex3f(v->position.x, v->position.y, v->position.z);
        }
    }
    glEnd();
}

void draw_mesh(const twol::Mesh& mesh, const ViewerState& state)
{
    glDisable(GL_TEXTURE_2D);
    draw_mesh_pass(mesh, state, false, false);

    const bool use_leaf_texture = state.leaf_texture_enabled && state.leaf_texture_loaded && state.leaf_texture != 0U;
    if (use_leaf_texture) {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glAlphaFunc(GL_GREATER, 0.22f);
        glBindTexture(GL_TEXTURE_2D, state.leaf_texture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        draw_mesh_pass(mesh, state, true, true);
        glBindTexture(GL_TEXTURE_2D, 0U);
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_TEXTURE_2D);
    } else {
        draw_mesh_pass(mesh, state, true, false);
    }
}


void gui_rect(const float x, const float y, const float w, const float h, const twol::Color4 c)
{
    glColor4f(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void gui_outline(const float x, const float y, const float w, const float h, const twol::Color4 c)
{
    glColor4f(c.r, c.g, c.b, c.a);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

std::array<unsigned char, 7> gui_glyph(const char in) noexcept
{
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(in)));
    switch (c) {
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 14};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '.': return {0, 0, 0, 0, 0, 12, 12};
    case ':': return {0, 12, 12, 0, 12, 12, 0};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '/': return {1, 1, 2, 4, 8, 16, 16};
    case '%': return {17, 1, 2, 4, 8, 16, 17};
    case '<': return {2, 4, 8, 16, 8, 4, 2};
    case '>': return {8, 4, 2, 1, 2, 4, 8};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

void gui_text(float x, const float y, const std::string& text, const float scale, const twol::Color4 c)
{
    glColor4f(c.r, c.g, c.b, c.a);
    for (char ch : text) {
        if (ch == ' ') {
            x += scale * 4.0f;
            continue;
        }
        const auto rows = gui_glyph(ch);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((rows[static_cast<std::size_t>(row)] & (1U << (4 - col))) != 0U) {
                    gui_rect(x + static_cast<float>(col) * scale, y + static_cast<float>(row) * scale, scale, scale, c);
                }
            }
        }
        x += scale * 6.0f;
    }
}

bool gui_contains(const ViewerState& state, const float x, const float y, const float w, const float h) noexcept
{
    return state.mouse_x >= x && state.mouse_x <= x + w && state.mouse_y >= y && state.mouse_y <= y + h;
}

bool gui_button(ViewerState& state, const std::string& label, const float x, const float y, const float w, const float h)
{
    const bool hover = gui_contains(state, x, y, w, h);
    const twol::Color4 fill = hover ? twol::Color4{0.20f, 0.30f, 0.22f, 0.96f} : twol::Color4{0.12f, 0.16f, 0.14f, 0.94f};
    gui_rect(x, y, w, h, fill);
    gui_outline(x, y, w, h, {0.48f, 0.64f, 0.36f, 1.0f});
    gui_text(x + 6.0f, y + 7.0f, label, 2.0f, {0.82f, 0.95f, 0.68f, 1.0f});
    return hover && state.mouse_pressed;
}

bool gui_toggle(ViewerState& state, const std::string& label, bool& value, const float x, const float y, const float w, const float h)
{
    const bool clicked = gui_button(state, label + (value ? " ON" : " OFF"), x, y, w, h);
    if (clicked) {
        value = !value;
    }
    return clicked;
}

bool gui_slider_float(ViewerState& state, const int id, const std::string& label, float& value, const float min_value, const float max_value, const float x, const float y, const float w)
{
    const float h = 34.0f;
    const bool hover = gui_contains(state, x, y, w, h);
    if (state.mouse_pressed && hover) {
        state.active_gui_control = id;
    }
    if (!state.mouse_down && state.active_gui_control == id) {
        state.active_gui_control = -1;
    }

    bool changed = false;
    if (state.active_gui_control == id && state.mouse_down) {
        const float old = value;
        const float t = std::clamp((static_cast<float>(state.mouse_x) - x) / w, 0.0f, 1.0f);
        value = min_value + (max_value - min_value) * t;
        changed = std::abs(value - old) > 0.0001f;
    }

    gui_text(x, y, label, 2.0f, {0.78f, 0.88f, 0.72f, 1.0f});
    const float bar_y = y + 18.0f;
    gui_rect(x, bar_y, w, 7.0f, {0.06f, 0.08f, 0.07f, 0.95f});
    const float t = std::clamp((value - min_value) / std::max(0.0001f, max_value - min_value), 0.0f, 1.0f);
    gui_rect(x, bar_y, w * t, 7.0f, {0.32f, 0.62f, 0.24f, 1.0f});
    gui_rect(x + w * t - 4.0f, bar_y - 4.0f, 8.0f, 15.0f, hover || state.active_gui_control == id ? twol::Color4{0.86f, 0.95f, 0.56f, 1.0f} : twol::Color4{0.60f, 0.72f, 0.42f, 1.0f});

    char buffer[48]{};
    std::snprintf(buffer, sizeof(buffer), "%.2F", static_cast<double>(value));
    gui_text(x + w - 54.0f, y, buffer, 2.0f, {0.88f, 0.92f, 0.78f, 1.0f});
    return changed;
}

bool gui_slider_int(ViewerState& state, const int id, const std::string& label, int& value, const int min_value, const int max_value, const float x, const float y, const float w, const bool force_odd)
{
    float tmp = static_cast<float>(value);
    const bool changed_float = gui_slider_float(state, id, label, tmp, static_cast<float>(min_value), static_cast<float>(max_value), x, y, w);
    if (!changed_float) {
        return false;
    }
    int next = static_cast<int>(std::lround(tmp));
    if (force_odd && (next % 2) == 0) {
        next += next < value ? -1 : 1;
    }
    next = std::clamp(next, min_value, max_value);
    const bool changed = next != value;
    value = next;
    return changed;
}

void render_gui(GLFWwindow* window, ViewerState& state, const int width, const int height)
{
    (void)height;
    bool graph_changed = false;
    bool mesh_changed = false;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    gui_rect(8.0f, 8.0f, 404.0f, 1120.0f, {0.015f, 0.025f, 0.020f, 0.88f});
    gui_outline(8.0f, 8.0f, 404.0f, 1120.0f, {0.30f, 0.45f, 0.30f, 1.0f});
    gui_text(20.0f, 22.0f, "TWOOL PLANT LAB", 2.0f, {0.84f, 0.96f, 0.64f, 1.0f});

    float y = 54.0f;
    gui_text(20.0f, y, "PRESET", 2.0f, {0.70f, 0.86f, 0.58f, 1.0f});
    y += 18.0f;
    if (gui_button(state, "CANNABIS", 20.0f, y, 108.0f, 28.0f)) { set_preset(state, twol::PlantPreset::Cannabis); graph_changed = true; }
    if (gui_button(state, "TREE", 136.0f, y, 76.0f, 28.0f)) { set_preset(state, twol::PlantPreset::Tree); graph_changed = true; }
    if (gui_button(state, "BUSH", 220.0f, y, 76.0f, 28.0f)) { set_preset(state, twol::PlantPreset::Bush); graph_changed = true; }
    if (gui_button(state, "FERN", 304.0f, y, 70.0f, 28.0f)) { set_preset(state, twol::PlantPreset::Fern); graph_changed = true; }
    y += 34.0f;
    if (gui_button(state, "PATTERN", 20.0f, y, 100.0f, 28.0f)) { set_preset(state, twol::PlantPreset::Pattern2D); graph_changed = true; }
    if (gui_button(state, "MODE", 128.0f, y, 76.0f, 28.0f)) { cycle_dimension(state); graph_changed = true; }
    if (gui_button(state, "NEW SEED", 212.0f, y, 112.0f, 28.0f)) { state.params.seed = state.params.seed * 1664525U + 1013904223U; graph_changed = true; }
    if (gui_button(state, "REGEN", 330.0f, y, 52.0f, 28.0f)) { graph_changed = true; }

    y += 40.0f;
    gui_text(20.0f, y, std::string("MODE ") + twol::to_string(state.params.dimension_mode), 2.0f, {0.78f, 0.88f, 0.72f, 1.0f});
    y += 22.0f;
    if (gui_button(state, state.paused ? "PLAY" : "PAUSE", 20.0f, y, 78.0f, 28.0f)) { state.paused = !state.paused; }
    if (gui_button(state, state.playback_direction < 0.0f ? "FWD" : "REV", 106.0f, y, 70.0f, 28.0f)) { state.playback_direction *= -1.0f; }
    if (gui_button(state, "RESET", 184.0f, y, 78.0f, 28.0f)) { state.params.time_seconds = 0.0f; mesh_changed = true; }
    if (gui_button(state, "EXPORT", 270.0f, y, 104.0f, 28.0f)) { export_current_obj(state); }
    y += 36.0f;
    if (gui_slider_float(state, 100, "TIME", state.params.time_seconds, 0.0f, state.params.full_growth_seconds, 20.0f, y, 350.0f)) { mesh_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 101, "SPEED", state.playback_speed, 0.05f, 8.0f, 20.0f, y, 350.0f)) {}
    y += 44.0f;
    if (gui_slider_int(state, 102, "GENERATIONS", state.params.generations, 1, 14, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 103, "BRANCHES", state.params.branch_count, 1, 12, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 112, "BRANCH DEPTH", state.params.recursive_branch_depth, 0, 4, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 113, "SIDE SHOOTS", state.params.shoots_per_branch, 0, 4, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 114, "SAPLING NODES", state.params.initial_sapling_nodes, 0, 12, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 115, "SAPLING AGE", state.params.sapling_pre_age, 0.0f, 0.75f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 116, "BRANCH DELAY", state.params.branch_activation_delay, 0.0f, 0.30f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 117, "BRANCH WINDOW", state.params.branch_growth_window, 0.08f, 0.90f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 104, "BRANCH ANGLE", state.params.branch_angle_from_up_deg, 5.0f, 85.0f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 105, "UPWARD BEND", state.params.side_branch_upward_bend, 0.0f, 1.0f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 106, "LEAF SCALE", state.params.leaf_scale, 0.05f, 8.0f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 107, "LEAF DROOP", state.params.leaf_droop, 0.0f, 1.4f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 108, "LEAFLETS", state.params.leaflet_count, 1, 15, 20.0f, y, 350.0f, true)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_int(state, 110, "FANS / NODE", state.params.fan_leaves_per_node, 1, 12, 20.0f, y, 350.0f, false)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 111, "LEAF DENSITY", state.params.leaf_node_density, 0.0f, 1.0f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 44.0f;
    if (gui_slider_float(state, 109, "LEAF PITCH", state.params.leaf_pitch_down_deg, -15.0f, 85.0f, 20.0f, y, 350.0f)) { graph_changed = true; }
    y += 42.0f;
    if (gui_toggle(state, "VEINS", state.params.leaf_veins, 20.0f, y, 96.0f, 28.0f)) { mesh_changed = true; }
    if (gui_toggle(state, "NODES", state.params.show_node_markers, 126.0f, y, 100.0f, 28.0f)) { mesh_changed = true; }
    if (gui_toggle(state, "WIRE", state.params.wireframe, 236.0f, y, 92.0f, 28.0f)) {}
    y += 34.0f;
    if (gui_toggle(state, "TEXTURE", state.leaf_texture_enabled, 20.0f, y, 118.0f, 28.0f)) {}
    if (gui_button(state, "RELOAD TEX", 150.0f, y, 144.0f, 28.0f)) { load_leaf_texture(state, state.root / "assets" / "plant_texture_atlas.ppm"); }
    y += 34.0f;
    if (gui_button(state, "ATLAS -", 20.0f, y, 94.0f, 28.0f)) { cycle_atlas_region(state, -1); }
    if (gui_button(state, "ATLAS +", 124.0f, y, 94.0f, 28.0f)) { cycle_atlas_region(state, 1); }
    gui_text(230.0f, y + 7.0f, selected_atlas_region(state).name, 2.0f, {0.78f, 0.88f, 0.72f, 1.0f});
    y += 34.0f;
    if (gui_button(state, "SAVE", 20.0f, y, 80.0f, 28.0f)) { save_state(state); }
    if (gui_button(state, "LOAD ;", 110.0f, y, 80.0f, 28.0f)) { load_state(state); graph_changed = true; }

    char info[128]{};
    std::snprintf(info, sizeof(info), "SEED %u", state.params.seed);
    gui_text(202.0f, y + 7.0f, info, 2.0f, {0.78f, 0.88f, 0.72f, 1.0f});

    if (graph_changed) {
        clamp_params(state.params);
        state.graph_dirty = true;
    }
    if (mesh_changed) {
        state.mesh_dirty = true;
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

    state.mouse_pressed = false;
    state.mouse_released = false;
}

void render(GLFWwindow* window, ViewerState& state)
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    width = std::max(width, 1);
    height = std::max(height, 1);

    glViewport(0, 0, width, height);
    glClearColor(0.025f, 0.035f, 0.032f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective(55.0f, static_cast<float>(width) / static_cast<float>(height), 0.05f, 250.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -state.distance);
    glRotatef(state.pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(state.yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-state.target.x, -state.target.y, -state.target.z);

    const GLfloat light_pos[] = {3.0f, 8.0f, 4.0f, 1.0f};
    const GLfloat light_diffuse[] = {0.90f, 0.92f, 0.86f, 1.0f};
    const GLfloat light_ambient[] = {0.20f, 0.23f, 0.21f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

    draw_grid(8.0f, 16);

    glPolygonMode(GL_FRONT_AND_BACK, state.params.wireframe ? GL_LINE : GL_FILL);
    draw_mesh(state.mesh, state);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    render_gui(window, state, width, height);
}

} // namespace

int main()
{
    if (!glfwInit()) {
        std::cerr << "[TwoOL] ERROR: glfwInit failed.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1720, 1100, "TwoOL Plant Lab V6", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        std::cerr << "[TwoOL] ERROR: glfwCreateWindow failed.\n";
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ViewerState state;
    state.root = std::filesystem::current_path();
    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    load_leaf_texture(state, state.root / "assets" / "plant_texture_atlas.ppm");

    print_controls();

    auto previous = std::chrono::steady_clock::now();
    double title_accumulator = 1.0;

    while (!glfwWindowShouldClose(window)) {
        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;

        if (!state.paused) {
            state.params.time_seconds += dt * state.playback_speed * state.playback_direction;
            if (state.params.time_seconds > state.params.full_growth_seconds) {
                state.params.time_seconds = state.params.full_growth_seconds;
                state.paused = true;
            }
            if (state.params.time_seconds < 0.0f) {
                state.params.time_seconds = 0.0f;
                state.paused = true;
            }
            state.mesh_dirty = true;
        }

        clamp_params(state.params);
        rebuild_if_needed(state);
        render(window, state);
        glfwSwapBuffers(window);
        glfwPollEvents();

        title_accumulator += static_cast<double>(dt);
        if (title_accumulator >= 0.25) {
            update_title(window, state);
            title_accumulator = 0.0;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
