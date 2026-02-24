# ImmersiveUI

VR Menu Framework for Skyrim VR - Interactive menus on the non-dominant hand with raycast activation.

## Features
- Native VR UI elements (Buttons, Sliders, Toggles, Containers).
- Attached to the player's non-dominant hand.
- Interaction via a laser pointer from the dominant hand.
- MCM-like integration with INI settings.
- Extensible API for other mods.

## Requirements
* [XMake](https://xmake.io) [3.0.1+]
* C++23 Compiler (MSVC, Clang-CL)
* CommonLibSSE-NG

## Getting Started
```bat
git clone --recurse-submodules https://github.com/GabeAlvz/ImmersiveUI.git
```

### Build
To build the project, run:
```bat
xmake build
```

### Build Output
The built plugin will be in the `build/` directory.

### Credits
- CommonLibSSE-NG
- SKSE VR
- SimpleIni
- ClibUtil