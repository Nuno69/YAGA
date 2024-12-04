# Compiling the Plugin

Before you proceed, make sure to compile the project in [Visual Studio](https://visualstudio.microsoft.com/pl/downloads/).  
It's recommended to use **MP x2 MT Release** configuration.

# Building the VDF archive

To generate **yaga.vdf** archive, open up the **powershell** command line in this directory and run this command:
```
./vdfsbuilder build.vm
```
Copy the produced archive from **YOUR_REPO_PATH/VDF/yaga.vdf** into **YOUR_GAME_PATH/Data/yaga.vdf**