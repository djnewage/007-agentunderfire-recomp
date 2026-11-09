# VR for 007: Agent Under Fire recomp - design

Goal: a full VR mode (stereo, 6DOF head tracking, motion controllers) for the Windows build, SteamVR via OpenXR,
Valve Index first. Nothing here needs the game's code to be re-recompiled: it is a renderer feature of the SDK
runtime plus a handful of game hooks in `src/bond` and `src/driving`.

## Where the pieces already are

- Every draw reaches the renderer as a `gpu::State` snapshot with the full vertex constant file (`State::vsc`, 192
  registers, uploaded verbatim to `cbuffer VC : register(b0)` per draw in `D3D12Backend::UploadConstants`) and the
  fixed function transforms (`State::transforms`, folded into `DrawConstants::wvp/vpj` by `FillTransformConstants`).
  Changing the camera for an eye is changing those before the draw.
- `gpu::MatrixProgram(program)` already finds the four consecutive constants a vertex program multiplies its
  position by (used for the ultrawide HUD squeeze), and `interp.cpp` already inverts those matrices to track the
  camera between steps. The on-foot game (Quake III derivative) is mostly fixed function; `DRIVING.XBE` draws through
  vertex programs (`squeeze_screen_programs = true` in `src/common/game_info.cpp`).
- `gpu::Extension` (`sdk/runtime/gpu/ext.h`, Direct3D 12 only, nothing sets one today) hooks exactly the points
  stereo needs: `TargetKey` (another host copy of the same guest surface: one per eye), `MakeTarget` (size and
  format of a new target: the headset's per-eye size), `Constants` (the vertex registers as uploaded), `Bound`,
  `Reset`, `Settings`, and a `Context` lending the device, command list, transitions, blits and full screen passes.
- `interp.cpp` (`CreateInterpolating`) is a working record-and-replay wrapper around a `Backend`: it keeps a step's
  clears, draws and copies with their `State` and replays them with changed constants. The stereo wrapper is the
  same shape with a simpler job.
- The game-side FOV and aspect hooks (`src/bond/view.cpp`: helpers 0x13A350/0x13A3D0/0x13A440/0x13A4B0 take the
  angle on the stack; `src/driving/view.cpp`: 0x102990 takes hfov, aspect, near, far) see the game's own projection
  parameters each frame. They are where the game's projection is learned and where head yaw can be fed back.
- Input: `GameInfo::pad(port, input::Pad&)` edits a polled pad before the game sees it; `input::TakeMouse` gives the
  on-foot game direct look rates (`mouse_direct = true`). Controllers map onto those without touching game code.
- The backend chain is built in `sdk/runtime/xdk/d3d8.cpp` around line 571:
  `D3D12 -> Threaded -> Interpolating (driving only)`. Stereo slots in as another wrapper.
- OpenXR plumbing to lift: `C:\Users\djnew\nfsheat-vr\src\vr_openxr.cpp` (loader via LoadLibrary, instance, session,
  swapchains, xrWaitFrame/xrLocateViews/xrEndFrame, blit) with the Khronos headers and `openxr_loader.dll` vendored
  in `thirdparty/openxr`. It binds D3D11; here it binds D3D12 (`XR_KHR_D3D12_enable`, needs the command queue, so
  `gpu::Context` gets a `Queue()` accessor).

## Architecture

```
title thread                  render thread
D3D8 HLE  -> Stereo wrapper -> Threaded -> Interpolating? -> D3D12Backend (+ VrExtension)
             records the frame;                                 TargetKey: per-eye target copies
             eye 0 passes through live,                          MakeTarget: eye size from OpenXR
             eye 1 replayed at Present,                          Present: both eyes -> XR swapchains,
             each with per-eye State                             mirror (left eye) -> window
```

1. **`gpu::Stereo` wrapper** (`sdk/runtime/gpu/stereo.cpp`). Keeps the frame's Clear/Draw/Copy records (as
   `interp.cpp`'s `Rec`, without the vertex-keeping: the title is blocked in Swap while the replay is issued, and the
   Threaded backend already orders memory reads against the title). Eye 0 is passed through as it arrives with its
   `State` copy rewritten for the left eye; at `Present` the records are replayed rewritten for the right eye, then
   the inner `Present` runs once. Sets a thread-visible `g_eye` the extension reads.
2. **Per-eye rewrite of a `State`** (same file). For a vertex program with `MatrixProgram` index k: `M' = P_eye *
   E_eye * P_game^-1 * M` on `vsc[k..k+3]`, where `P_game` is rebuilt from the parameters the view hooks recorded
   this frame (fov, aspect, near, far: on foot from `src/bond/view.cpp`, driving from `src/driving/view.cpp`) and
   `E_eye` is the eye's pose relative to the game camera (HMD orientation minus the yaw already fed to the game,
   plus the IPD offset). For fixed function draws the same on `transforms[projection]` and `transforms[view]`.
   Draws classified 2D (`ScreenDraw`, `ScreenProgram`, `OrthoConstants`, pretransformed) are left alone and land on
   a fixed quad: both eyes get them at a chosen depth by a horizontal shift (the HUD squeeze code already moves 2D
   in x per draw: `hud[]` constants).
3. **`VrExtension : gpu::Extension`** (`sdk/runtime/gpu/vr_ext.cpp`). `TargetKey` XORs the eye into the key for
   every colour and depth target while in a 3D pass, so each eye draws into its own copies; `MakeTarget` sizes
   full-frame targets to the OpenXR recommended view size; `Reset` on setting changes. At `Present` (a new
   `Extension::Present(Context&, Target& left, Target& right)` hook, or done in the wrapper through `Context`) the
   two eye targets are copied into the acquired XR swapchain images and submitted with the poses they were rendered
   with; the window shows the left eye.
4. **OpenXR session** (`sdk/runtime/vr/openxr.cpp`, lifted from nfsheat-vr): lazy start on the first frame, quiet
   failure to flat when no runtime; `xrWaitFrame` at the first draw of a frame (paces the title to the headset in
   place of `app::Pace`), `xrLocateViews` gives the two view poses and asymmetric FOVs for that frame's rewrite.
5. **Head tracking into the game.** HMD yaw delta each frame becomes look input through `TakeMouse`/the right stick
   (on foot) so the game's own camera, aim and culling follow the head horizontally; pitch and roll are render-only.
   The culling margin in both `view.cpp` files grows for VR so edges do not pop when the head leads the camera.
   Driving: the car camera follows the head the same way through 0x102990's view and the chase camera's yaw.
6. **Controllers.** OpenXR actions (Index controllers) mapped in `GameInfo::pad`: right stick turn (snap or smooth,
   a setting), left stick move, triggers fire and aim, A/B/grip to the game's buttons. Hand-aimed weapons are a later
   step (the first person weapon is drawn by 0x14B654 with its own projection, per `src/bond/view.cpp`).
7. **Settings.** `vr = false` in `settings.toml` and the F1 window: VR on/off, render scale per eye, turn mode and
   speed, HUD distance and size, world scale. Off, nothing changes in the flat port.

## Milestones

1. **Mono head look in the headset.** OpenXR session on D3D12, left eye only, the window frame copied into one XR
   swapchain with the game's own projection; HMD yaw fed to the game as look. Proves the session, pacing and input path.
2. **Stereo.** The wrapper and extension: per-eye target copies, projection substitution for fixed function and
   vertex programs, 2D on a quad. On foot first (fixed function), then driving (vertex programs, composed with the
   Interpolating wrapper).
3. **6DOF and comfort.** Positional offset in `E_eye`, pitch and roll, cull margins, snap turn, HUD options.
4. **Controllers.** OpenXR actions to pad, then hand-aimed weapon as its own task.

## Rules kept from the repo

- SDK changes (`stereo.cpp`, `vr_ext.cpp`, `vr/openxr.cpp`, `Context::Queue`, an `Extension::Present` hook, the
  chain in `d3d8.cpp` behind the setting) are commits on the SDK fork's `vr` branch; game changes (`view.cpp` hooks
  recording projection parameters and feeding yaw, VR rows in PC Settings) on this repo's `vr` branch. Both track
  upstream `main` by merge, never rebase.
- `assets/`, `generated/`, `.local/` stay out of git. Local build: `.local/build-local.bat` (portable Clang 19.1.7
  in `.local/llvm`, VS Build Tools' CMake and Ninja).
