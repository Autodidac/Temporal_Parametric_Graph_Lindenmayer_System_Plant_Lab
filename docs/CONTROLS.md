# Controls

## Mouse

- Left drag outside GUI: orbit camera
- Mouse wheel: zoom
- Left click/drag inside GUI: buttons and sliders

## GUI

The left panel is the primary control surface:

- Preset: Cannabis, Tree, Bush, Fern, Pattern
- Mode: 3D, 2.5D, 2D Plant, 2D Pattern
- Time: scrub growth timeline
- Speed: playback speed
- Generations: node/system depth
- Branches: branches per growth node
- Branch Angle: branch launch angle measured from vertical
- Upward Bend: how hard side branches hook upward
- Leaf Scale: leaf size
- Leaf Droop: downward leaf bend
- Leaflets: fan leaflet count; cannabis defaults to 7
- Leaf Pitch: whole fan downward pitch
- Veins: visible petiole/midrib/side-vein geometry
- Nodes: debug node markers
- Wire: wireframe mode
- Save/Load: `out/current.twol`
- Export: `out/current.obj`

## Keyboard Shortcuts

```text
1-5       presets: cannabis/tree/bush/fern/pattern
M         cycle dimensions: 3D / 2.5D / 2D / pattern
Space     pause/resume
R         reverse playback
T         reset timeline
Left/Right scrub timeline
- / =     speed down/up
G         regenerate current seed
N         new seed
[ / ]     generations down/up
B / V     branch count down/up
A / Z     branch angle from vertical down/up
U / J     upward branch bend up/down
O / P     leaf scale down/up
D / F     leaf droop down/up
K / I     leaflet count down/up
Y         leaf veins on/off
W         wireframe
C         node markers
S         save state to out/current.twol
L         load state from out/current.twol
X         export OBJ to out/current.obj
Esc       quit
```


V3 fixes: terminal tips are capped/pointed, plant mode cycling no longer jumps into pattern mode, side branches start higher on the node, GUI now exposes fan leaves per node and leaf density. Keyboard load moved to semicolon (`;`) because `L` now increments fan leaves per node.

## v4 controls

Additional GUI controls:

- `BRANCH DEPTH` controls secondary branch recursion.
- `SIDE SHOOTS` controls how many child shoots can emerge from branch nodes.
- `TEXTURE` toggles textured leaf rendering.
- `RELOAD TEX` reloads `assets/leaf_texture.ppm` at runtime.

Keyboard:

- `H / L` fan leaves per node down/up.
- `;` load saved state.


V5 atlas support:
- `assets/plant_texture_atlas.png` is the original user-provided atlas.
- `assets/plant_texture_atlas.ppm` is the dependency-free runtime texture loaded by the OpenGL viewer.
- GUI buttons `ATLAS -` / `ATLAS +` switch UV regions: cannabis, broad, compound, fern, needle.
- `Q` / `E` also switch atlas regions.
- White atlas background is converted to transparent alpha at load time.


## V6 growth simulation changes

V6 changes the timing model from a pure reveal animation to a persistent dormant scaffold with real temporal organ activation. The first trunk nodes and starter foliage are pre-aged so the plant begins as a sapling instead of a naked pole. Trunk internodes, lateral branches, secondary shoots, and leaves now overlap in time, so branches grow while the trunk keeps extending. New controls include Sapling Nodes, Sapling Age, Branch Delay, and Branch Window.
