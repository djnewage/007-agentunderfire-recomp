# 007: Agent Under Fire - XBE Recomp

An unofficial native PC port of the original Xbox version of 007: Agent Under Fire, in progress. The game's own x86 code
is statically recompiled with the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool. It is not an emulator.

> [!NOTE]
> This project does not include any game assets. You must provide the files from your own legally acquired copy of
> the game to build and play.

## Status

In progress and not yet released. The game runs through its movies, menus, every mission and the driving levels in a
headless test build; it has not yet been played on Windows.

## Supported version

The European (PAL) Xbox release, title id 4541000D. `default.xbe` SHA-1 `3dbed4eb51701a53676e0ace4cdea1752dc04697`,
`bond_eng.xbe` `66aace3859a5ca9a992f2d1845c2b5f5265fd9d6`, `DRIVING.XBE` `6b58e61ee8ce9aa2a1a357affe188af874cf05f4`.

## Features

Set in the F1 window (or `settings.toml` beside the exe); the runtime's own display, graphics and controller options are there too.

- **Widescreen** - True proportions on any frame wider than 4:3, with the 3D view widened and the menus and HUD kept in a 4:3 centre. The console's own 4:3 picture is kept on a 4:3 screen.
- **Field of view** - 50 to 150% of the game's, on foot and driving.
- **Skip intro movies** - Starts without the logos.
- **60 FPS on foot** - Paced by the frame rate cap in the F1 window, 60 by default, instead of the console's 30.
- **Mouse and keyboard** - The mouse looks directly on foot, moves a mounted gun as the right stick, and works the menus.
- **Sharp 2D** - Text and HUD pictures enlarged with sharp edges.

### Keyboard and mouse

| Control | On foot | Driving | On a mounted gun |
| --- | --- | --- | --- |
| Move / steer | W S forward and back, A D strafe | W gas, S brake, A D steer | |
| Look | Mouse | | Mouse |
| Fire | Left button | Left button | Left button |
| Manual aim / zoom | Right button | | Right button |
| Jump | Space | | |
| Crouch | C, Left Ctrl | | |
| Action, reload, open | E, R | | R reload |
| Use gadget | Q | Right button, E | |
| Alt. fire | F, Middle button | | |
| Weapon | Wheel, X, 1 2 | Wheel, Q, Z, 1 2 | |
| Gadget | 3 4, arrows up and down | 3 4 | |
| Handbrake | | Space | |
| Camera / look back | Tab, V center view | C camera, V look back | |
| Pause | Esc, P | Esc, P | Esc, P |
| Menus | Enter, left button select; Backspace, right button back; wheel, arrows move | the same | |

Keys can be changed in the F1 window.

## Building

1. Clone with submodules: `git clone --recurse-submodules https://github.com/hk21x/007-agentunderfire-recomp`.
2. Extract your disc into `assets` (`default.xbe`, `bond_eng.xbe`, `DRIVING.XBE`, `assets\`, `driving.dat\`).
3. Install Visual Studio 2022 with C++ desktop development and run `build.bat`. The exes are in `out\build\release`.
   Run `AgentUnderFire.exe`; it starts the other two itself.

The game is three programs, and the build recompiles all three, which takes a while.
