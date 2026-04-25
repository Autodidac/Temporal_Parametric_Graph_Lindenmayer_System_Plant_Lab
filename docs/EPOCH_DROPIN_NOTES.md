# Engine Drop-in Notes

Copy only:

```text
src/twol/
```

Do not copy:

```text
examples/
build/
out/
```

## Render integration

The core produces:

```cpp
struct twol::Mesh
{
    std::vector<twol::Vertex> vertices;
    std::vector<std::uint32_t> indices;
};
```

`twol::Vertex` contains:

- position
- normal
- color
- uv

Upload that into your engine's static/dynamic mesh path.

## Runtime regeneration

Keep `twol::PlantParameters` as the editable state.

When seed/preset/generation/shape parameters change:

```cpp
graph = twol::generate_node_graph(params);
```

When only time changes:

```cpp
mesh = twol::build_mesh(graph, twol::timeline01(params));
```

That separation avoids rebuilding the graph every frame.

## Serialization

Use:

```cpp
twol::save_parameters(params, path);
twol::load_parameters(params, path);
```

The format is plain text and diffable.
