# YAGA: Yet Another Gothic Accessibility Attempt

## What is YAGA?

**YAGA** is the **union plugin** which aims to make the game fully playable and accessible for the blind players. This goal is going to be achieved with both speech and sound in order to aid and guide the player throughout the game's world.

## Requirements

### For players

- The steam version of [Gothic 1](https://store.steampowered.com/app/65540/Gothic_1/) or [Gothic II: Night Of The Raven](https://store.steampowered.com/app/39510/Gothic_II_Gold_Edition/).  
Other versions of the game should work however that has not been tested and so is not recommended.
- A screen reader: **NVDA**, **JAWS**, the **Zhengdu Screenreader** (ZDSR) or another one supported by [Prism](https://github.com/ethindp/prism). Without one, YAGA speaks through the Windows voices (OneCore or SAPI).
- [yaga.vdf](https://github.com/Nuno69/YAGA/releases)

### For developers

Everything outlined above plus the following:

- [git lfs](https://git-lfs.com/) (required for fetching lfs files from repo)
- [CMake](https://cmake.org/) 3.28 or newer
- Visual Studio 2022 (or its Build Tools) with the **C++ ATL** component, which [Prism](https://github.com/ethindp/prism) needs
- An internet connection for the first configure: CMake downloads the Prism release source (pinned by its hash)
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

### SpeechBackend

YAGA speaks through [Prism](https://github.com/ethindp/prism), which talks to the screen reader or to a speech engine. Possible values are:
- `auto` - the running screen reader; when none runs, a Windows voice (OneCore or SAPI). A screen reader started later takes over within ten seconds, and one that is closed hands over to a Windows voice.
- a Prism backend name, for example `NVDA`, `JAWS`, `ZDSR`, `SAPI` or `OneCore` (exact case). If it does not work, `auto` is used.

Text also goes to the braille display when the screen reader supports it.  
The default value is `auto`.

Prism uses the ZDSR library installed with the screen reader. If an older YAGA was copied into the game by hand, delete `ZDSRAPI.dll` and `nvdaControllerClient32.dll` from `Gothic\system\Autorun`: the old `ZDSRAPI.dll` is loaded by Union first and keeps ZDSR from working. YAGA says so when it finds it.

### SpeechRate

Rate of a Windows voice (screen readers keep their own settings), from `0` to `100`.  
The default value is `50`, the voice's normal rate.

### SpeechVolume

Volume of a Windows voice (screen readers keep their own settings), from `0` to `100`.  
The default value is `50`, the system's normal volume.

### CompassAngleOffset

Controls the global compass offset angle in degrees, when you press `SHIFT+LEFT_ARROW` or `SHIFT+RIGHT_ARROW` your world rotation will be set to next closest angle.  
The default value is `45`.

## TODO

- [ ] Perform code clean up. Refactor the code to adhere to single responsibility principle. Put input handling in one place
- [ ] Make the game automatically speak in menus and dialogs
- [ ] Make an accessible audio radar which will inform the player about their surroundings with both speech and sound
- [ ] Create a system which will allow the player to traverse the world from point A to point B (turn by turn navigation)
- [ ] During combat, report adversary's weapon proficiency.
- [ ] Make all game screens accessible, namely trading, learning, ETC
- [ ] Make all game popups accessible
- [ ] Make the plugin localizable
- [x] Make the plugin configurable

## Contributions

Please keep in mind that I am an inexperienced programmer, and YAGA is my first large-scale project.

Please forgive me any stupid errors I might have. I don't have the skill, I am just very determined to make it happen.

With that in mind if you have any suggestion about the code structure, functions or you want to write your own feature, please drop me a PR.
