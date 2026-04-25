# TwoOL Procedural Plant Lab

Standalone C++23 procedural plant / L-system lab.

This package replaces the old WickedEngine-specific prototype with a reusable core plus a small OpenGL viewer.

## Current focus

- Generic node-based plant / L-system core
- 3D, 2.5D, 2D plant, and 2D pattern modes
- Runtime GUI controls in the OpenGL viewer
- Time-based growth playback and scrubbing
- Seeded regeneration
- OBJ export
- `.twol` text state save/load

## Presets

- Cannabis-style visual plant
  - 7-prong fan leaves by default
  - tan/brown stems instead of green sticks
  - fast upward-bending side branches for grow-tent spacing
  - visible petiole, midrib, and side-vein geometry on fan leaves
- Tree
- Bush
- Fern/vine-ish plant
- 2D L-system pattern

## Run on Windows

Extract the zip, open PowerShell in this folder, then run:

```powershell
.\build_and_run.bat
```

That is the main entry point. It builds and runs the OpenGL viewer.

Requires:

- Visual Studio 2022 Build Tools or full Visual Studio
- CMake
- vcpkg with `VCPKG_ROOT` set

The script installs/checks `glfw3:x64-windows` automatically.

## Export without viewer

```powershell
.\export_obj.bat
```

Output goes to:

```text
out/
```

## Drop into an engine

Use only:

```text
src/twol/
```

Do not take `examples/opengl_glfw/` unless you want the demo viewer.

Main engine-facing types:

```cpp
twol::PlantParameters
twol::NodeGraph
twol::Mesh
```

The core has no OpenGL dependency.


V3 fixes: terminal tips are capped/pointed, plant mode cycling no longer jumps into pattern mode, side branches start higher on the node, GUI now exposes fan leaves per node and leaf density. Keyboard load moved to semicolon (`;`) because `L` now increments fan leaves per node.

## v4 changes

This version raises the ceiling and fixes the visible max-out problem from v3:

- Branch generation now supports secondary branch recursion instead of only first-order side branches.
- Added GUI controls for `BRANCH DEPTH` and `SIDE SHOOTS`.
- Increased limits: generations up to 14, branch count up to 12, fans per node up to 12.
- Removed pencil-cone terminal tips; terminal segments now use caps instead.
- Cannabis leaf vein geometry is off by default. Leaf detail is now better handled by texture.
- Added replaceable leaf texture: `assets/leaf_texture.ppm`.
- Viewer can reload leaf texture at runtime with the `RELOAD TEX` GUI button.

To use your own texture, replace `assets/leaf_texture.ppm` with another binary PPM P6 RGB file, then click `RELOAD TEX`.


V5 atlas support:
- `assets/plant_texture_atlas.png` is the original user-provided atlas.
- `assets/plant_texture_atlas.ppm` is the dependency-free runtime texture loaded by the OpenGL viewer.
- GUI buttons `ATLAS -` / `ATLAS +` switch UV regions: cannabis, broad, compound, fern, needle.
- `Q` / `E` also switch atlas regions.
- White atlas background is converted to transparent alpha at load time.


## V6 growth simulation changes

V6 changes the timing model from a pure reveal animation to a persistent dormant scaffold with real temporal organ activation. The first trunk nodes and starter foliage are pre-aged so the plant begins as a sapling instead of a naked pole. Trunk internodes, lateral branches, secondary shoots, and leaves now overlap in time, so branches grow while the trunk keeps extending. New controls include Sapling Nodes, Sapling Age, Branch Delay, and Branch Window.
