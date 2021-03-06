# Yaga: Yet Another Gothic Accessibility Attempt

## What is YAGA?

YAGA is the Gothic I, Gothic II and Gothic II: Night of the Raven plugin which aims to make the game fully playable and accessible for the blind players. This goal is going to be achieved with both speech and sound in order to aid and guide the player throughout the game's world. Please keep in mind that Union is required to play the game with YAGA, while the Union SDK is required to contribute to the project.

## Requirements

### For players

- The steam version of Gothic, Gothic II or Gothic II: Night Of The Raven. Other versions of the game should work however that has not been tested and so is not recommended.
- Union plugin, which can be obtained from [here](https://www.worldofgothic.de/dl/download_651.htm)
- The NVDA screenreader or the Zhengdu Screenreader
- The plugin itself (not available for pre-build download yet)

### For developers

Everything outlined above plus the following:

- Visual Studio 2010 or higher
- Union SDK (obtained from [here](https://worldofplayers.ru/resources/136/download)
- Gothic VDFS (optional if you want to pack the plugin to the VDF file).

## Settings

Here all the settings of the mod are going to be described. All settings are located in the gothic ini file, in the "YAGA" key. If the setting is not in the file, its default value will be restored and written to the file.

### DefaultSpeechEngine

Controls what speech engine is being used for speech output in YAGA. Possible values are 0 (ZDSR) and 1 (NVDA). The default value is 1.

### ZDSRUseMultiChannel

Controls whether the multi channel feature of ZDSR is going to be utilizedby YAGA. If NVDA is used, this option is not even queried by the mod so nothing will be changed. The default value is 1.

## Goals

- Make the game automatically speak in menus and dialogs (partially done)
- Make an accessible audio radar which will inform the player about their surroundings with both speech and sound (partially done)
- Make all game screens accessible, namely trading, learning, ETC (not touched yet).
- Make all game popups accessible (not touched yet).
- Make the plugin localizable (not touched yet).
- Make the plugin configurable (Done!)

## Contributions

Please keep in mind that I am an inexperienced programmer, and YAGA is my first large-scale project. Please forgive me any stupid errors I might have. I don't have the skill, I am just very determined to make it happen. with that in mind if you have any suggestion about the code structure, functions or you want to write your own feature, please drop me a PR.