# 007: Agent Under Fire recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - plays headless. default.xbe's movies, START, bond_eng.xbe's title, menus, briefing and first mission;
  DRIVING.XBE's five levels and back. No trap in 200 s of the first mission. Not yet built or played on Windows.
- XDK: 4039 on SDK branch agentunderfire (not in the fork's main): default.xbe 98, bond_eng.xbe 113, DRIVING.XBE 112 of
  266 functions; layout proven from bond_eng.xbe's code
- Push buffers: none (no game code calls MakeSpace or touches the device; RunPushBuffer not called)
- Smoke: scripts/smoke.sh, default.xbe -> START -> bond_eng.xbe's first mission, about 3 minutes, SMOKE OK
- GitHub: hk21x/007-agentunderfire-recomp; SDK branch agentunderfire pushed
- Not done: Windows, French and German xbes, DRIVING.XBE in the smoke test, levels played to their end, PC features

## Disc
- PAL, title 0x4541000D (EA), certificate region 0x4, version 2; extracted with extract-xiso from Harry's image.
- default.xbe 3dbed4eb51701a53676e0ace4cdea1752dc04697, built 2002-03-27: XAPILIB, D3D8, XBOXKRNL, LIBC, XGRAPHC
- bond_eng.xbe 66aace3859a5ca9a992f2d1845c2b5f5265fd9d6, built 2002-03-27: XAPILIB, XBOXKRNL, LIBC, D3D8, D3DX8, XGRAPHC
- bond_fre.xbe 87b2d3c8b7b40916fe29b03490c84c4f690a4c12, built 2002-03-27; bond_ger.xbe 88d06191f3ba940394f258f138f0aa37f1ced1dd, built 2002-04-08
- DRIVING.XBE 6b58e61ee8ce9aa2a1a357affe188af874cf05f4, built 2002-03-26: XAPILIB, D3DX8, D3D8, XGRAPHC, XBOXKRNL, LIBC, LIBCPMT
- Every library 1.0.4039. Sections include a DSOUND section though the library table does not list it.

## Log

### 2026-10-30 - Every level loads headless; the driving handover goes both ways; GitHub repository
- hk21x/007-agentunderfire-recomp holds main (merged GitHub's initial commit, the MIT licence). .gitmodules follows the SDK
  branch agentunderfire until it merges into the fork's main.
- Tour, 75 s each with the pad script: mis02a, mis02b, mis07a, mis07b, mis09 from patched launch data, and bond_eng.xbe with
  0x2A9F98 = 0, 3, 4, 5, 7, 9, 10, 11, 12 (6 and 8 relaunch into DRIVING.XBE): no traps, no warnings but wbinvd, a frame of
  each looked at. Each only stood or drove a little: none played to its end. Index 5 and 12 looked like the same place.
- Quit Mission in mis02a relaunches into BOND_ENG.XBE with the level name still in the launch data; it boots and draws.
- Generated HLSL: 24 shaders from bond_eng.xbe's menus and first mission, 22 from mis02a, all compile with DXC.
- The runtime always reports English (language 1), so default.xbe always picks bond_eng.xbe: French and German wait for a
  language option.
- Next: Harry's first Windows build.

### 2026-10-30 - The first mission and a driving level play headless (sdk branch agentunderfire)
- XDK 4039 is new, between 3948 and 4831. Names came from aligning each library section with PGR1's, NightFire's and
  PGR2's in link order (body similarity, ret sizes must agree), then by hand from the instructions. Scan from 16-30 of
  258 functions to 98-113 of 266. layout.toml cites bond_eng.xbe throughout.
- 4039 has its own state numbering (StateNumbering = 4039, runtime xdk/state_numbering.h): 82 simple, 35 deferred
  (SwapFilter added at 116: the frame buffer init 0x198FEB stores it), 29 complex read from the setter table 0x19EE94
  (no SampleAlpha). Swap and frame surface pointers as 4831; present interval a device field and GetBackBuffer's out
  pointer as 3948.
- Push buffers: none. .text of all three xbes has no call to MakeSpace, no reference to D3D_Device or D3D_pDevice, and no
  stores of the method headers searched. Immediate mode goes through the library's SetVertexData calls (D3D section).
- bond_eng.xbe needed SetI3DL2Listener named (unnamed, the library ran on the host's sound object and crashed).
- DRIVING.XBE hung: the heap's lock pointer (0x81CB6010 + 0x38) held pixels. Something writes exactly one row (0xA00)
  past the 640x576 surface at 0x81B4E000 while the loading screen is up, and the host had put the heap right above it.
  Frame surfaces now get a spare row (SDK NewSurface). The writer itself was not found: a hardware watchpoint did not fire.
- Reached mis02a by poking the menu's mission (0x2A9F98 = 1, 0x2A5D38 = 1) on the briefing: the relaunch carries it in
  the launch data; a copy is in .local/hand. The menu's own unlock path has not been played.
- Evidence: SMOKE OK; captures of movies, title, menus, briefing, mission 1, mis02a's gun section and pause; 60 s of
  bond_eng.xbe's mix with no silent second and no clipping; scripts/wincheck.sh ok.
- Next: Windows build (needs a GitHub repository), then the remaining driving levels and languages.

### 2026-10-30 - Project set up
- From the SDK's templates/game with three images. First scans with the 3948 seed: default 16, bond_eng 27, DRIVING 30
  of 258 functions. Next: name the rest, then the push buffer check.
