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

## Building

1. Clone with submodules: `git clone --recurse-submodules https://github.com/hk21x/007-agentunderfire-recomp`.
2. Extract your disc into `assets` (`default.xbe`, `bond_eng.xbe`, `DRIVING.XBE`, `assets\`, `driving.dat\`).
3. Install Visual Studio 2022 with C++ desktop development and run `build.bat`. The exes are in `out\build\release`.
   Run `AgentUnderFire.exe`; it starts the other two itself.

The game is three programs, and the build recompiles all three, which takes a while.
