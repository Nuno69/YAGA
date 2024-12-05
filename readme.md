# YAGA: Yet Another Gothic Accessibility Attempt

## What is YAGA?

**YAGA** is the **union plugin** which aims to make the game fully playable and accessible for the blind players. This goal is going to be achieved with both speech and sound in order to aid and guide the player throughout the game's world.

## Requirements

### For players

- The steam version of [Gothic 1](https://store.steampowered.com/app/65540/Gothic_1/) or [Gothic II: Night Of The Raven](https://store.steampowered.com/app/39510/Gothic_II_Gold_Edition/).  
Other versions of the game should work however that has not been tested and so is not recommended.
- [Union](https://www.worldofgothic.de/dl/download_651.htm)
- The **NVDA screenreader** or the **Zhengdu Screenreader**
- [yaga.vdf](https://github.com/Nuno69/YAGA/releases)

### For developers

Everything outlined above plus the following:

- [git lfs](https://git-lfs.com/) (required for fetching lfs files from repo)
- [Visual Studio](https://visualstudio.microsoft.com/pl/downloads/) (2022 or higher)

### Building

To build the project, open up the **YAGA.sln** in [Visual Studio](https://visualstudio.microsoft.com/pl/downloads/) and hit **Build Solution**.  
This will produce the **Bin/yaga.dll** and **VDF/yaga.vdf** files.  
You can also build **YAGA** project, to produce only the plugin without generating **yaga.vdf**.

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