# 007: Agent Under Fire recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - plays headless. default.xbe's movies, START, bond_eng.xbe's title, menus, briefing and first mission;
  DRIVING.XBE's five levels and back. No trap in 200 s of the first mission. On Windows the first mission plays to its end; Harry (2026-10-30, D3D12): plays with the driving fix, no obvious visual bugs on a first look.
- XDK: 4039 in the fork's main: default.xbe 98, bond_eng.xbe 113, DRIVING.XBE 112 of
  266 functions; layout proven from bond_eng.xbe's code
- Push buffers: none (no game code calls MakeSpace or touches the device; RunPushBuffer not called)
- Smoke: scripts/smoke.sh, default.xbe -> START -> bond_eng.xbe's first mission, about 6 minutes, then the driving part, SMOKE OK
- GitHub: hk21x/007-agentunderfire-recomp; SDK at the fork's main 36bb42e
- PC features: skip intro, field of view, mouse on foot confirmed by Harry; skip intro, field of view, mouse, 60 FPS, radar and culling confirmed by Harry
- Not done: in-game PC Settings page, French and German xbes, DRIVING.XBE in the smoke test, levels played to their end, PC features

## Disc
- PAL, title 0x4541000D (EA), certificate region 0x4, version 2; extracted with extract-xiso from Harry's image.
- default.xbe 3dbed4eb51701a53676e0ace4cdea1752dc04697, built 2002-03-27: XAPILIB, D3D8, XBOXKRNL, LIBC, XGRAPHC
- bond_eng.xbe 66aace3859a5ca9a992f2d1845c2b5f5265fd9d6, built 2002-03-27: XAPILIB, XBOXKRNL, LIBC, D3D8, D3DX8, XGRAPHC
- bond_fre.xbe 87b2d3c8b7b40916fe29b03490c84c4f690a4c12, built 2002-03-27; bond_ger.xbe 88d06191f3ba940394f258f138f0aa37f1ced1dd, built 2002-04-08
- DRIVING.XBE 6b58e61ee8ce9aa2a1a357affe188af874cf05f4, built 2002-03-26: XAPILIB, D3DX8, D3D8, XGRAPHC, XBOXKRNL, LIBC, LIBCPMT
- Every library 1.0.4039. Sections include a DSOUND section though the library table does not list it.

## Log

### 2026-10-31 - Every mission loads; DRIVING.XBE's prompts show keys
- Sweep with the smoke's poke on 0x2A9F98, indices 0 to 15 for 110 s each: no crash. All on-foot levels draw (12 to 15 look like 5, so probably past the list). 1, 2, 6, 8 relaunch to mis02a, mis02b, mis07a, mis09; those ran 60 to 90 s, no crash. That is loading and the first minute, not levels played through.
- bond_eng.xbe 30 minutes on random input: alive at the end, 77 MB flat. (rc=124 is the timeout's.)
- Driving prompts: a 256x256 P8 atlas, FNV 0x0D17DFB8FC1761F7, same in mis02b and mis09, at a different address per level (82104080, 82103880). An earlier note's 820C1580 and 820E2580 were not checked again. Bound with five palettes: the icons' is the one with the A oval's rim green. The line's A is the tall oval (99,100 to 131,164), squeezed to 12x20; change page and scroll are the round cells at (202,150) and (202,176). Keys: a return arrow, back arrow, bare arrows; caps elsewhere. The palette has no dark grey, so bare glyphs read better.
- Took several wrong turns: fonts first, the wrong palette, the wrong cells, and blaming the reference renderer's cache, which a live poke disproved (it clears each frame). The redraw must be in the bind, which returns true: only that marks the texture dirty for the real renderers.
- Seen on screen with AUF_KEYS_TEST=1 (keys from the first bind) and with a controller (the disc's). A switch mid-pause, keys to pad, seen in the log and on screen on the pad side only; D3D12 itself untested. Cells other than the line's not seen in use.
- SMOKE OK, wincheck of the two files ok.

### 2026-10-31 - Multiplayer no longer crashes; long runs; the pointer's other pages; Select Mission by mouse
- Crash found by opening Multiplayer, by d-pad too, so not the pointer: 0x5BE11 ends a typed-out text's 256-byte copy at the count of letters shown, and this page's speed takes that to 1071, a zero past the top of the guest stack at 0xD006003F. Probably mapped memory on the console. Hook ClampTextEnd keeps it in the buffer; the page opens and shows its text. No match started here (one pad): multiplayer play itself untried.
- Gun on the real path: save, Select Mission, left to Precious Cargo, relaunch with mis02a. AUF_MOUSE_TEST 2400 counts turned 2.39 radians, then -0.2605 +/- 0.0003 for 1000 frames.
- Long runs on random input: car mis02b 760 s and gun mis02a 283 s played to the mission's end and relaunched, no crash, 85 and 92 MB flat. Those pads do not fight well: both are survival to a mission end, not coverage of a level.
- Pointer: option rows join both columns (they were one or the other), Multiplayer's choices share one box so are not pointed at. Clicks are read a poll after the press, as a jump and click in one poll used the old answer.
- Select Mission: side pictures step, the middle starts, the difficulty's line steps it, elsewhere nothing. Seen in captures. Rectangles measured at 16:9 only, hand-kept for this page. Option values are still arrows (A does not modify them).
- SMOKE OK, wincheck of src/common/menu_mouse.cpp ok.

### 2026-10-30 - The launch segfault (0x321D0) did not come back
- 360 launches of default.xbe by exit code, 24 at a time, each alive at 3 s: none died. (240 more earlier were judged by their logs, which cannot show a silent death, so they do not count.) With the earlier 42, about 400 without a repeat; it remains one sighting right after a build.
- Nothing changed: a fix without a repro could not be shown to work. Still open; disc_rate first if it returns. 0x321D0 walks the ring from [ebx+0xC] outside the lock, so a stale chunk is plausible, not shown.

### 2026-10-30 - A visible pointer in the on-foot menus
- Harry chose it after "clicking feels odd". While a menu is up in bond_eng.xbe the Windows arrow shows (a window procedure in src/common/menu_mouse.cpp; no SDK change, so no shared gate). src/bond/pointer.cpp hooks 0x56930, the menu's per-frame input, whose argument is the menu file.
- The item under the pointer becomes current by the game's own sequence (0x37AD0, 0x37A30, page +0x60, sound 0x202E0, as 0x39120). A click waits until its item is current; a click off every item does nothing; pages of 0 or 1 items keep click anywhere.
- Rectangles from the page's own widgets; each page's are logged. Wrong at first twice: a save row's thumbnail and caption made rows overlap (now the largest text), the pause rows' only picture is the chevron (narrow pictures give way to text).
- Tried headless with AUF_POINTER=polls:x:y:buttons,... (16:9), read off captures: main menu hover, click off an item ignored, click opens Load Mission; device page click anywhere; slot BONDA, Yes loads the save, back at the main menu; pause menu hover 1 and 2 of 3.
- Not seen: the arrow itself and real window coordinates (no window here, Windows code only compiled); Harry to try. Options pages, Multiplayer, ncpausemenu not tried. Select Mission's carousel and difficulty are not items: still the arrows and wheel. DRIVING.XBE's menu is another engine with no rectangles found: no pointer there.
- wincheck and SMOKE OK.

### 2026-10-30 - The gun's spin, 1 for guns and 2 for gadgets
- Harry: aim better; moving a mounted gun spun the camera 360 degrees; clicking in the main menus feels odd; the wheel zoom not reached yet. His save (.local/win/hdd.zip, all three mission kinds).
- The spin, two causes: rates were set only when the mouse reported movement, so the last one kept turning (the harness's test always "reports", so it never showed); and +0x1BC, +0x1C0 are ramps a held stick builds, read as an angle.
  Now every frame on a gun rates and targets are set, 0 with no movement, ramps cleared. Measured: 2020 counts 1.78 radians (0.00088 a count), the heading the same in the next dump. Up and down stopped at 0.147 (the mission's limit, by the look).
- d-pad tried in play at last: right Q-Laser then Q-Claw (gadgets), down the gun. The earlier mapping read off the code was backwards. 1 next weapon (down), 3 before (up), 2 next gadget (right), the wheel weapons (down, up).
- Menu clicks stepped once a click headless, right button back once, the wheel moves: no double presses found. There is no cursor: a click acts on what is highlighted. Harry to say if that is the oddness.
- Correction: an earlier note here said his save loads by itself and that the relaunch was seen; that run showed neither (no relaunch line, and "00 AGENT" came from my own DOWN on Select Mission).
  Tried properly: Load Mission, Xbox Hard Disk, BONDA (Mission 3), Yes (the prompt starts on No), back at the main menu. Select Mission opens on Dangerous Pursuit, RIGHT does not go past it. Start it: 0x2A6BB relaunches into DRIVING.XBE with level mis02b; that launch data then ran 60 s, 2520 frames, no traps (HOLD_REPORTS). SMOKE OK.
  Not run from the save: the mounted gun by the menu or a real mouse; the spin fix is measured with AUF_MOUSE_TEST only.

### 2026-10-30 - Auto-aim off, NightFire's keys
- Harry: key pictures OK. Auto-aim off (option auto_aim, default off, every input): on foot the target search 0xB8BB0 sees player +0x716 clear for the call (the log says the option was on);
  on guns 0x76310 sees no target [0x1C6C18] for the call (logged on a target at 23 s in mis02a). Whether aim feels free: Harry's. Homing weapons and 0x9DDB0's use of the amounts untouched.
- On foot as NightFire: Q cycle weapon, G use gadget, 1 next weapon, 2 next gadget, 3 weapon before. Which way d-pad down cycles gadgets is from the code, not tried. A layout saved in settings.toml's [keys] keeps what it says.

### 2026-10-30 - Scope wheel, mounted gun aim, key pictures; SDK merged (fork main 36bb42e)
- Harry confirmed the radar and on-foot pop-in. Gate on 36bb42e, each game's commit in a copy (.local/gate2): PGR2, NightFire, PGR1 plus repro's live race,
  EoN (its SDK merged, -X theirs on what already conflicts), this game: SMOKE OK. Fast-forward; .gitmodules follows main.
- Mounted guns (src/driving/mouse.cpp): the camera 0x77A20 turns by rates +0x250/+0x254 eased to targets +0x260/+0x264. Stick full: 1.55 radians in 390 frames.
  Counts now set rates and targets directly (mouse_direct on). AUF_MOUSE_TEST 20,0: the view turned right; 0,20: down. Not felt by hand. Earlier "wrong camera"
  readings were before the stick or in a pause: memory dumps, not gdb's slow runs, showed it. Sensitivity assumes 60 frames a second there.
- MenuUp was wrong in both exes. DRIVING.XBE's 0x1801D8 was up in play (a coincidence of a diff), so the left button played A in the car on Windows; now [0x1BEB1C] == 3
  or 0x1E1818: in and out at the pause and the mission's end. bond_eng.xbe compared the name's pointer's bytes, so never the front end; the menu mouse there did not work
  before now. Fixed, in from 0.18 s to the mission at 37.3 s.
- Scope wheel: notches multiply weapon +0x51C at 0xBD768, up to +0x524. Not run: no scope reachable headless. Whether the zoom resets on leaving is the game's: Harry to see.
- Key pictures (src/common/buttons.cpp): bond_eng.xbe's atlas 0x837ECB80 and the Press A picture 0x8382CB80 replaced by keys while the keyboard plays, by their FNV. Drawings checked by
  dumping them (AUF_BUTTON_DUMP): ENTER and BKSP in menus; the reference renderer does not show replacements, so not seen on screen. Only bond_eng.xbe: DRIVING.XBE's pause icons
  are in P8 atlases 820C1580/820E2580, not done.
- Item 6: no keypad screens exist (the safes use the Q-Decryptor). Enter is A everywhere on foot, the left button too where the player has no control (rate 0).
- Smoke's driving part failed once: harness read poke between creation and write and removed it. Moved into place whole now.

### 2026-10-30 - A and D strafe on foot; driving pop-in and 60 FPS confirmed
- Harry: the culling fix cured the driving pop-in; 60 FPS performs OK. A and D turned the view: they played the left stick's x, which the game's Setup 1
  makes yaw. Now the right stick's x (RSLeft, RSRight), its strafe; the mouse turns. Headless, the right stick held: the command's right 0x7E (cmd +0x11),
  forward 0, yaw unchanged. A layout saved in settings.toml's [keys] keeps what it says.

### 2026-10-30 - Driving pop-in: the world's frustum is fixed at the console's angle
- Harry: 60 FPS fine; the pop-in shows just driving along the road.
- Traced from world draws (gdb stacks: 0x7DF90, 0xA6AF0, the list at +0xC08): the list builder 0x7DC10 tests spheres with 0x7E140 against the
  frustum at 0x190640, built by 0x7E3C0 from the camera's half angle (33 degrees at 0x1906A4) and nothing else: the view's widening never reached
  it, hence the same draw count at every width. 0x7E3C0 now gets the angle widened by the frame and EoN's 2.2 margin, at most 80 degrees.
- Evidence: mis07a frame 1500, 344 draws (277 world) before, 357 (290) after, the picture the same; SMOKE OK. A pop itself was never caught headless:
  whether this cures it is Harry's to see. The earlier memory scan missed it: the planes are in columns, the watchpoint candidate 0x82A6AE20 was mesh data.
- The second test 0xAE550 (planes at 0x1C6CA0) was not looked at; the count of those planes is 0x1C7CA0. First suspect if it still pops.

### 2026-10-30 - Harry's ultrawide run: the radar, 60 FPS, a culling margin (sdk branch agentunderfire)
- Harry, 3440x1440: intro skip, field of view and mouse on foot fine. Three problems.
- Driving map gone: the radar draws in its own viewport (477,48,135x135) and the squeeze moved the picture out of it. A draw squeezed
  whole now takes its viewport and scissor along (gpu::SqueezedX, D3D12, D3D11, reference). Reproduced headless once the reference renderer
  squeezed 576 line targets: cut in half before, whole at FRAME_ASPECT_X100=239 after. Changes SDK behaviour: needs the gate again.
- 30 FPS on foot: 0x139015 and 0x138940 ask for interval 2. Now the default interval, paced by fps_cap (src/bond/framerate.cpp, option uncapped,
  AUF_30FPS). Timing runs off a clock (0x25CA0): 600 frames, 10.18 s of game time, 60 a second headless.
- Pop-in on foot: a margin of 2.2 on the culling frustum's sides (0x15CD00), EoN's remedy. About 16% more draws in the first mission's opening,
  the picture the same; not compared like for like (frame shape differed). Whether it cures Harry's pop-in is not known: not reproduced.
- Pop-in driving: NOT fixed. Draw counts are the same at 4:3 and 4x (284, 344), so what is drawn does not follow the view's width and something
  else chooses it; no EoN-style frustum found by code pattern or by scanning memory for 33 degree planes, one candidate 0x82A6AE20 not tested.
  4x frames show no gaps standing still. Harry's two captures (mis02b) have blank final frames and no log. Next: a clip or capture at a pop.

### 2026-10-30 - SDK merged into the fork's main (0733ba3)
- Gate, each game's own commit in a disposable copy (.local/gate) with the candidate SDK: PGR2 SMOKE OK; NightFire SMOKE OK; PGR1 SMOKE OK and
  repro.sh's frame a live race; EoN SMOKE OK on its pinned SDK merged with the candidate (-X theirs for launcher, handover and boot, which
  conflict between its line and main already); this game SMOKE OK including the driving part. Fast-forward; main had not moved.
- Smoke test now goes on into DRIVING.XBE (Windows build settings, HOLD_REPORTS). Not checked that it fails without the fix by running it;
  the same hold reproduced the freeze by hand. .gitmodules follows main again.

### 2026-10-30 - PC features, headless (pc features in src/; not yet on Windows)
- Skip intro: 0x20BD0 returns at once but for HILITE; bond_eng.xbe reached 0.26 s after start.
- Widescreen: bond_eng.xbe, [0x3467B8] written in 0x1387E0 (draw, culling, tags together, true proportions); DRIVING.XBE,
  0x102990 as NightFire's. 2D kept in 4:3 (hud_aspect). The reference renderer squeezes 480 line targets alone, so the driving
  HUD's 640x576 stays full width in the harness; the GPU rule has no such limit: Windows to confirm (log: "ultrawide:").
- Field of view: the four projection helpers' angle. The first person weapon's own path is skipped (0x14B654), but at AUF_FOV=130 the
  gadget hand still looked smaller: another path draws it. Harry to judge.
- Mouse: bond_eng.xbe 0x24570, degrees by the game's rates (scope, manual aim, no control). AUF_MOUSE_TEST=30,0,500,560 turned the view
  right. DRIVING.XBE's guns take it as the right stick (mouse_direct off): no direct aim there, not tried headless.
- Menus: MenuUp in bond_eng.xbe [0x3B2428], [0x3B2418], state name "frontend"; DRIVING.XBE byte 0x1801D8 (pause: the only byte up across one
  by memory diff) or 0x1E1818 (not in play yet). 0x1801D8 blinks a few times as a pause closes. Guns: the control file's name at 0x17FED1.
- Keys from the game's Controller Setup page; DRIVING.XBE's follow NightFire's, as the same engine's, unconfirmed here.
- Left out: an in-game PC Settings page and keyboard button pictures in the prompts. Auto-aim stays the game's while the mouse aims.

### 2026-10-30 - Windows confirmed
- Harry: the freeze and the models are fixed, the game works, no obvious visual bugs on a first look. Not yet a full
  playthrough. Next: the SDK gate and merge, then PC features.

### 2026-10-30 - Driving levels froze on Windows: visibility reports (sdk)
- Harry: textures fixed (D3D12). Starting the second mission froze: DRIVING.XBE drew about 11 frames, then none, F3's frame
  never came. Thread 14's stack (log, guest_mem_8864656): 0x87851, the loop 0x87830 calling 0xFFC10 until true, that is
  GetVisibilityTestResult 0x114BE0, in the level's load.
- 4039's GetVisibilityTestResult only reads the report. Later XDKs' also set a fence, and the runtime's hosted D3D_SetFence is
  what kicks the renderer; nothing here did, so the report never came. The harness's null backend writes reports at once,
  which hid it.
- SDK: an image naming D3DDevice_GetVisibilityTestResult (DRIVING.XBE only; the others do not link it) walks the pushes and
  kicks the renderer before the library's read. Other games do not name it, so their behaviour is unchanged.
- Proof, in a temporary harness build (RT_THREAD=1 RAW=1, null backend holding reports until Flush): before, frames stop at
  about 24 s with 1054 draws; after, mis02a plays through 60 s. SMOKE OK, wincheck ok. Windows still to confirm.

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
