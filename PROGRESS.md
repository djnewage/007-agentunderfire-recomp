# 007: Agent Under Fire recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - plays headless. default.xbe's movies, START, bond_eng.xbe's title, menus, briefing and first mission;
  DRIVING.XBE's five levels and back. No trap in 200 s of the first mission. On Windows the first mission plays to its end; the fix for torn models awaits a Windows run.
- XDK: 4039 on SDK branch agentunderfire (not in the fork's main): default.xbe 98, bond_eng.xbe 113, DRIVING.XBE 112 of
  266 functions; layout proven from bond_eng.xbe's code
- Push buffers: none (no game code calls MakeSpace or touches the device; RunPushBuffer not called)
- Smoke: scripts/smoke.sh, default.xbe -> START -> bond_eng.xbe's first mission, about 3 minutes, SMOKE OK
- GitHub: hk21x/007-agentunderfire-recomp; SDK branch agentunderfire pushed
- Not done: French and German xbes, DRIVING.XBE in the smoke test, levels played to their end, PC features

## Disc
- PAL, title 0x4541000D (EA), certificate region 0x4, version 2; extracted with extract-xiso from Harry's image.
- default.xbe 3dbed4eb51701a53676e0ace4cdea1752dc04697, built 2002-03-27: XAPILIB, D3D8, XBOXKRNL, LIBC, XGRAPHC
- bond_eng.xbe 66aace3859a5ca9a992f2d1845c2b5f5265fd9d6, built 2002-03-27: XAPILIB, XBOXKRNL, LIBC, D3D8, D3DX8, XGRAPHC
- bond_fre.xbe 87b2d3c8b7b40916fe29b03490c84c4f690a4c12, built 2002-03-27; bond_ger.xbe 88d06191f3ba940394f258f138f0aa37f1ced1dd, built 2002-04-08
- DRIVING.XBE 6b58e61ee8ce9aa2a1a357affe188af874cf05f4, built 2002-03-26: XAPILIB, D3DX8, D3D8, XGRAPHC, XBOXKRNL, LIBC, LIBCPMT
- Every library 1.0.4039. Sections include a DSOUND section though the library table does not list it.

## Log

### 2026-10-30 - Vertex blending on the render thread (sdk)
- Harry's capture 154810 (D3D12): the flaps gone, most of the hand and the flag missing. The render thread's state copy
  (gpu/render_thread.cpp) carried only transforms[256], so WORLD1 to WORLD3 reached the backend as zeros and vertices weighted
  to them collapsed. It now carries all four.
- Proof in the harness built as the Windows exe (RT_THREAD=1 RAW=1): with the old line the frame is Harry's (thumb, fragments,
  no flag); with the new one hand, sleeve and flag are whole. The previous entry had only tested the plain harness.
  SMOKE OK, wincheck ok. Windows still to confirm.

### 2026-10-30 - Vertex blending: the torn hands and characters (sdk)
- Harry: the signed lock (below) did not fix it. The earlier diagnosis was wrong: the tear is in every renderer, also the
  harness's (RAW=1, RT_THREAD=1 and neither alike); it had been read as whole in small montages. Also wrong: the "white door"
  and black ground in the captures were this viewer showing the BMPs' alpha over white. Flatten them (`-alpha off`).
- Cause: bond_eng.xbe's models are FVF 0x11C (XYZB4, stride 48) with D3DRS_VERTEXBLEND 5 (3 weights, 4 matrices): each vertex
  has its weight on one of WORLD to WORLD3 (604 of 1272 logged vertices not on the first). Neither renderer implemented
  vertex blending, the first game to use it. Culling off left the tear; assembling strips as fans broke the sky.
- SDK: FixedKey::blend, FixedVertexHlsl and the reference renderer blend position and normal in view space, then project
  (DrawConstants bwv, bwvn, vpj). Modes 1-6 written, only 5 seen. The shader cache is keyed on the source, so old entries are
  not reused. Evidence: the harness draws hand, sleeve and the flag on the pole whole. 24 shaders compile, wincheck ok, SMOKE OK.
- Open: one segfault in default.xbe at 0.14 s, in the movie stream ring (0x321D0, "STRM"), during a smoke run right after a
  build; 6 runs alone and 36 runs at 6 at a time did not repeat it. A race with the streaming thread is suspected, not shown;
  GameInfo::disc_rate (EoN) is the first thing to try if it returns.
- Next: Harry's rebuild on Windows.

### 2026-10-30 - First Windows run; see-through models: vertex buffer locks did not wait (sdk: 4039 signs the lock's wait)
- Harry, Windows, D3D12, RX 7900 XT: the game loads and the first mission plays to its end with no crash or hitch, but
  surfaces can be seen through. His F3 capture (.local/win/144318, 2560x1440): the hand is torn into pieces, also in the
  depth buffer, so triangles are missing rather than transparent; the ground is black.
- Thought then to be the cause (it was not, see above; the signing stands), as NightFire's FINDINGS: D3D_BlockOnNonSurfaceResource was not in 4039's table (seeded from 3948, which lacks
  it), so a vertex buffer lock (0x196AF0, calling 0x198080) never waited for the renderer. Windows draws on a render thread
  and reads buffers the game has already refilled; the harness has no such thread, which fits its frames being whole.
- Now signed: default.xbe 0x45900, bond_eng.xbe 0x198080, DRIVING.XBE 0x11B030. Compared the table with 4831's and 5659's:
  nothing else missing that this game links, D3DDevice_GetRasterStatus not looked for.
- SMOKE OK, mis02a 40 s without a trap, wincheck ok. None of that tests the fix: only a Windows run can.
- Next: Harry's rebuild; the black ground may be a separate problem.

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
