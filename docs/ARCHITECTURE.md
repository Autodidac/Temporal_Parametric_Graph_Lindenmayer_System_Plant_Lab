# Architecture

## Core

`src/twol/` is the reusable portion.

It has no OpenGL, GLFW, WickedEngine, or editor dependency.

Main flow:

```cpp
twol::PlantParameters params = twol::make_preset(twol::PlantPreset::Cannabis);
params.time_seconds = params.full_growth_seconds;

twol::NodeGraph graph = twol::generate_node_graph(params);
twol::Mesh mesh = twol::build_mesh(graph, twol::timeline01(params));
```

## Node graph

The node graph owns the procedural structure:

- nodes
- branch segments
- leaf clusters
- timeline birth/end values

The mesh builder is separate. This lets an engine consume the graph directly, rebuild partial mesh states, or upload the generated mesh into its own renderer.

## Time model

Each branch segment and leaf cluster has a normalized birth/end range.

`build_mesh(graph, time01)` evaluates the graph at a timeline position.

This keeps reverse playback cheap and deterministic.

## Plant modes

`DimensionMode` controls output shape:

- `Plant3D`: normal 3D plant
- `Plant25D`: flattened Z depth, useful for side-view games or 2.5D scenes
- `Plant2D`: fully flattened plant
- `Pattern2D`: pure L-system line pattern mode

## Cannabis visual preset

The cannabis preset is only a visual/generative preset.

It uses:

- early upward side-branch reorientation
- narrow vertical plant shape
- large drooping fan leaves
- segmented leaflets for curved blades
- high leaf density around nodes

## L-system support

`lsystem.hpp/cpp` contains a simple deterministic context-free L-system expander/tracer. Pattern mode uses that directly.

The plant generator uses a node-based procedural 2OL-style structure rather than string-only turtle rendering. That is more useful for engine integration because the graph remains editable and time-addressable.


V5 atlas support:
- `assets/plant_texture_atlas.png` is the original user-provided atlas.
- `assets/plant_texture_atlas.ppm` is the dependency-free runtime texture loaded by the OpenGL viewer.
- GUI buttons `ATLAS -` / `ATLAS +` switch UV regions: cannabis, broad, compound, fern, needle.
- `Q` / `E` also switch atlas regions.
- White atlas background is converted to transparent alpha at load time.


## V6 growth simulation changes

V6 changes the timing model from a pure reveal animation to a persistent dormant scaffold with real temporal organ activation. The first trunk nodes and starter foliage are pre-aged so the plant begins as a sapling instead of a naked pole. Trunk internodes, lateral branches, secondary shoots, and leaves now overlap in time, so branches grow while the trunk keeps extending. New controls include Sapling Nodes, Sapling Age, Branch Delay, and Branch Window.
