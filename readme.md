# YAGA: Yet Another Gothic Accessibility Attempt

## What is YAGA?

**YAGA** is the **union plugin** which aims to make the game fully playable and accessible for the blind players. This goal is going to be achieved with both speech and sound in order to aid and guide the player throughout the game's world.

## Requirements

### For players

- The steam version of [Gothic 1](https://store.steampowered.com/app/65540/Gothic_1/) or [Gothic II: Night Of The Raven](https://store.steampowered.com/app/39510/Gothic_II_Gold_Edition/).  
Other versions of the game should work however that has not been tested and so is not recommended.
- The **NVDA screenreader** or the **Zhengdu Screenreader**
- [yaga.vdf](https://github.com/Nuno69/YAGA/releases)

### For developers

Everything outlined above plus the following:

- [git lfs](https://git-lfs.com/) (required for fetching lfs files from repo)
- [CMake](https://cmake.org/)
- [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html)

### Building

Before compiling the mod you need to clone this repository recursively by executing the following command: `git clone --recursive https://github.com/nuno69/yaga`

### building with Visual Studio

1. Open the repository in Visual Studio
2. Choose a build target that matches your game version
3. Build the project with `CTRL+SHIFT+B`

### Building with Command Line

1. Configure the build system with the following command: `cmake --preset G2-Release`
2. Build the mod with `cmake --build --preset G2-Release`
3. Optionally package the mod: `cmake --build out/build/G2-Release --target vdf`

## Settings

Here all the settings of the mod are going to be described. All settings are located in the **gothic.ini** file, in the `[YAGA]` section. If the setting is not in the file, its default value will be restored and written to the file.

### DefaultSpeechEngine

Controls what speech engine is being used for speech output in YAGA. Possible values are:
- `0` - (ZDSR)
- `1` - (NVDA)
- `2` - (SAPI)

The default value is `1`.

### ZDSRUseMultiChannel

Controls whether the multi channel feature of **ZDSR** is going to be utilized by YAGA (works only for **ZDSR** speech engine).  
The default value is `1`.

### SAPIRate

Controls the rate of the **SAPI5** speech engine.  
The default value value is `5`, minimum is `1` and maximum is `10`.

### SAPIVolume

Controls the volume of the **SAPI5** speech engine.  
The default value is `100`, minimum is `1` and maximum is `100`.

### CompassAngleOffset

Controls the global compass offset angle in degrees, when you press `SHIFT+LEFT_ARROW` or `SHIFT+RIGHT_ARROW` your world rotation will be set to next closest angle.  
The default value is `45`.

### NavigatorMode

Controls how the navigator walks the hero along a route:
- `0` - the navigator hands the hero one waypoint at a time to the engine's NPC movement and watches every step (recommended)
- `1` - the whole route is handed to the engine (`EV_GOROUTE`), the navigator only watches the result

The default value is `0`.

### NavigatorProgressEvery

The navigator speaks the remaining distance every this many waypoints. `0` speaks only at the start, on recalculation and on arrival.
The default value is `5`.

### Language

Language of the messages YAGA speaks itself (the game's own texts are always in the game's language):
- `auto` - the language of the game: Polish for the Polish release, English otherwise
- `pl` - Polish
- `en` - English

The default value is `auto`.

### NavMesh

With `1`, the navigator walks on a navigation mesh built from the world's geometry. See [Navigation mesh](#navigation-mesh). `0` keeps the game's way net only.
The default value is `1`.

### DevCommands

Development aid. With `1` the plugin reads commands from `yaga_cmd.txt` in the game folder and writes the results to `YAGA.log`. Commands:
- position, route planning, benchmarks and teleporting;
- an invulnerable hero with monsters removed around it (`god`, `peace`);
- way net and height map dumps (`dumpnet`, `heightmap`);
- a graph check of every sector (`routeall`);
- a walk through a list of destinations (`tour`);
- the navigation mesh (`navmesh` with `info`, `rebuild`, `export FILE [input]`, `probe x1 z1 x2 z2`, `gates`, `polys`).

The default value is `0`.

## Navigation

Press `M` to open the sector map. Arrows move between 50 by 50 metre sectors, `ENTER` describes the selected sector and `SPACE` closes the map and walks the hero to it. `PAGE DOWN` and `PAGE UP` jump between named places (camps, mines, towers, the mountain fortress...) in alphabetical order. A sector with a named place says its name when you enter it with the arrows and on `ENTER`.

- Routes run on a navigation mesh of every surface the hero can walk on, with the game's way net as its roads. See [Navigation mesh](#navigation-mesh).
- The goal is the named place of the sector, if it has one. Otherwise it is the reachable spot nearest to the centre of the sector.
- `ENTER` on the map tells a sector the hero cannot walk to from where it stands ("No way there from here").
- The hero is walked by the engine's NPC movement, which steps around small obstacles. Where it gives up, the navigator runs the hero itself with the engine's animation controller and jumps or climbs when the hero stops moving. No key is ever pressed or held for the player: doors and ladders are used through the engine's own mob interaction.
- Ladders on the way are climbed, up or down. Closed doors on the way are opened; locked ones are reported, and so are closed gates. Places behind a gate the game opens by script (the monastery ruins, the fog tower, the Sleeper's temple) lead to the spot in front of it.
- Legs through deep water are swum. When the route is chosen they count ten times their length on the mesh (four times on the way net), so a dry way round wins when there is one. The lake below the intro cliff, which kills a swimming hero, is left out.
- When the hero is stuck or pushed off the route, the navigator remembers the blocked leg and recalculates the route around it.
- Any movement key cancels the walk. The walk also stops when the hero takes damage.
- `J` speaks the navigation status: target, remaining distance and waypoint.
- Giving up inside the target sector, or a few metres from the goal, counts as arrival.

### Navigation mesh

The game's way net was made for NPCs. It covers the paths they walk, in 30 separate pieces, and large walkable areas have no waypoints at all. YAGA therefore builds its own map of where the hero can walk: a navigation mesh made with [Recast & Detour](https://github.com/recastnavigation/recastnavigation) (zlib licence).

What goes into the mesh, all read from the loaded world (the ZEN file in memory):
- **Geometry:** the static world mesh and every object NPCs collide with.
- **The hero's limits:** step height, steepest walkable ground, and how deep the water has to be before he swims.
- **Left out:** surfaces the engine does not collide with from where the hero stands. The engine collides with a polygon from its front only, so a rock shelf or the far-mountain backdrop above a path is no ceiling, and the renderer's invisible occluders and portals are no walls.

Our own links on top of the mesh:
- **Ladders:** every ladder links its foot and its top.
- **Way net:** every way of the game's way net is a link that counts half its length, so routes keep to the paths NPCs use. They leave those paths only for the first and last stretch, or where no way leads. The ways also bridge the few places where the mesh is cut, such as a lip in the terrain.

How the mesh is used:
- **Doors:** they are left open in the mesh; the navigator opens them on the way.
- **Gates:** their ground is left out of a route while the gate is down.
- **Water:** swimming costs ten times its length, wading three times.
- **When it is built:** the mesh of a world is built once, in the background, about 5 seconds for the colony. It is saved in `YAGA\navmesh\<WORLD>.nav` in the game folder, and later loads read that file in a few milliseconds. A changed world (a mod) gets a new mesh.
- **Fallback:** until the mesh is ready, or where the hero stands off it, the navigator uses the way net alone.

Test results in the colony, with the hero starting at the start of the game:
- **Sectors:** the hero can reach 576 sectors on the mesh; the way net alone reaches 253.
- **Named places:** a walk through all 24 named places arrived at 21:
  - the orc graveyard was 8 m short when the test's time limit ran out;
  - the sunken tower needs swimming the hero gets stuck in;
  - the castle ruins failed because the hero was still at the sunken tower.

### Navigator waypoints

Some places the player can reach have no waypoints, or only a separate patch of them, because the way net was made for NPCs. YAGA adds its own waypoints and ways there. They are part of the way net's roads on the mesh, and the whole route when the mesh is not used. The navigator is the only one to see them: they are not inserted into the game's way net, so NPC routes and savegames do not change.

They come from a table built into the plugin and from `yaga_waynet_patch.txt` in the game folder, read when a world is loaded. One entry per line, `#` starts a comment:

```
world WORLD.ZEN        # following entries apply to this world only
node NAME x y z        # a new waypoint (world coordinates in cm, it is dropped onto the ground below)
link A B [jump]        # a way between two waypoints (new or the game's); "jump" marks a leg that needs a jump
unlink A B             # removes a link given earlier
avoid A B              # a way of the game's way net the hero cannot walk
```

With `DevCommands=1`, `patch` reloads the file and `rec start NAME` / `rec stop` record the hero's walk into `yaga_rec.txt` in this format.

Every spoken line and every navigator decision is written to `YAGA.log` in the game folder. Attach it to bug reports.

## TODO

- [ ] Perform code clean up. Refactor the code to adhere to single responsibility principle. Put input handling in one place
- [ ] Make the game automatically speak in menus and dialogs
- [ ] Make an accessible audio radar which will inform the player about their surroundings with both speech and sound
- [ ] Create a system which will allow the player to traverse the world from point A to point B (turn by turn navigation). Auto-walk to a map sector is done; turn by turn guidance is not.
- [ ] During combat, report adversary's weapon proficiency.
- [ ] Make all game screens accessible, namely trading, learning, ETC
- [ ] Make all game popups accessible
- [ ] Make the plugin localizable. YAGA's own messages are in Polish and English, picked by the game language; more languages need a translation table.
- [x] Make the plugin configurable

## Contributions

Please keep in mind that I am an inexperienced programmer, and YAGA is my first large-scale project.

Please forgive me any stupid errors I might have. I don't have the skill, I am just very determined to make it happen.

With that in mind if you have any suggestion about the code structure, functions or you want to write your own feature, please drop me a PR.
