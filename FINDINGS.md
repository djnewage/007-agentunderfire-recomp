# 007: Agent Under Fire recomp - findings

What the work has established, by topic. PROGRESS.md has the dated log with the evidence for each item. Each item is
marked **[console]** (how the Xbox, its XDK or the runtime behaves: the fix is in the SDK) or **[game]** (this game's
own code and data, with its addresses).

## The image
- **[game]** Five xbes: default.xbe (launcher, `.mpc` movies), bond_eng/fre/ger.xbe (the game, a Quake III derivative
  by its credits and cvars), DRIVING.XBE (EA's driving levels, EAGL). Their strings also name BOND.XBE,
  MOVIEXBOX_RELEASE.XBE and ASSET_*.BIG, which the disc does not have.
- **[game]** default.xbe plays the logos and hilite.mpc, then relaunches into BOND_ENG.XBE on START or at the end; the
  launch data names the locale file (IDL_ENG.LOC). Left idle, bond_eng.xbe's title relaunches into default.xbe.
- **[game]** bond_eng.xbe relaunches into DRIVING.XBE with the level name at launch data +0x448 (read into 0x1E0730):
  mis02a, mis02b, mis07a, mis07b, mis09. The level's archive is driving.dat\\<name>.viv, mounted after misc.viv.
- **[game]** The menu's mission is 0x2A9F98; 0x4AA40 starts it (1 mis02a, 2 mis02b, when 0x2A5D38 is set).

## XDK 4039
- **[console]** Between 3948 and 4831, see sdk/xdk/4039/layout.toml. CDevice is 0x2C60 bytes, miniport at +0x23E8.
  D3D_Device 0x1A0640 in bond_eng.xbe, and game code never touches it.
- **[console]** No DSOUND in the XBE's library table although the section is linked. No .XTLID section.

- **[console]** D3DRS_VERTEXBLEND is the NV2A skin mode (setter 0x193590 pushes method 0x328): 1, 3, 5 weights with one
  matrix more, the last taking the rest; 2, 4, 6 a weight per matrix. Blended in view space.
- **[game]** bond_eng.xbe's characters and first person hands are FVF 0x11C, mode 5, in small indexed strips per bone set.

- **[console]** 4039's D3DDevice_GetVisibilityTestResult (DRIVING.XBE 0x114BE0) sets no fence; a title polling it needs the host to
  kick the renderer itself.
- **[game]** DRIVING.XBE polls a visibility test in a loop while loading a level (0x87830).

- **[game]** DRIVING.XBE's world is chosen by the list builder 0x7DC10: spheres against the frustum at 0x190640 (0x7E140; planes in columns, built by
  0x7E3C0 from the camera's half angle), then 0xAE550 (planes at 0x1C6CA0).

- **[game]** DRIVING.XBE: level state [0x1BEB1C] 3 is paused or a movie; 0x1E1818 up from the level's start to the intro's end; the control file's name at 0x17FED1 tells a gun.
- **[game]** bond_eng.xbe: game state [0x3B2414], +4 index (3 front end, 4 in play), +0x10 a pointer to its name.
- **[game]** The gun and car camera is DRIVING.XBE 0x77A20 (rates +0x250, +0x254; targets +0x260, +0x264; turn +0x1BC).

## Push buffers
- **[game]** None in any of the three xbes.
- **[game]** bond_eng.xbe menus: 0x56930(file) reads input each frame. File +0x44 page, +0x54 lock. Page +0x54 item count, +0x5C items (0xDC each), +0x60 current. Item +0xB0 text chain (next +0x2C8: mode +0x8C, x +0x90, y +0x94, size +0x9C), +0xC4 picture chain (next +0x1F0: mode +0x90, x +0x94, y +0x98, w +0xA0, h +0xA4); mode 1 is a fraction of 640x480. Change item: 0x37AD0(old), 0x37A30(new), store +0x60, as 0x39120.
- **[game]** 0x5BE11 (text draw): ends a typed-out text's 256-byte stack copy at the shown count, unchecked; Multiplayer's text passes 1071. Clamped by hook ClampTextEnd.
- **[game]** DRIVING.XBE's prompt atlas: P8, FNV 0x0D17DFB8FC1761F7, five palettes, cells in src/common/buttons.cpp.
- **[harness]** The reference renderer reads textures fresh each frame (BeginFrame clears g_images). The palette a P8 was first bound with need not be the one a draw uses.
- **[game]** DRIVING.XBE steps once per timer tick and draws once per step: frame rate, game speed and slow motion (scale +0x20 of 0x180364) are one mechanism. NightFire's later build asks for 60 ticks where this PAL build asks for 50.

## The driving levels' cameras (USA Driving.xbe addresses; PAL not mapped)
- The missions: bond.xbe's table at 0x4AC14 (the mission number at USA 0x2A9888) starts orc_1, mis02a, mis02b, emstel_1, alp1_1, m6_1, mis07a, rig_1, mis09, uwb_2a, carrier1, alp2_1. The mis names are Driving.xbe's: mis02a (Precious Cargo), mis07b (the second half of Streets of Bucharest, chained from mis07a inside Driving.xbe at 0x9A6B8) and mis09 (Forbidden Depths) are mounted guns (0x99915 compares the level's name and loads povcfg%d.def); mis02b (Dangerous Pursuit) and mis07a are the car (drivecfg.def). Eight missions are on foot, their maps chained by Assets/global/twine.flo.
- The camera: the view manager [0x1F6ED8], +4 player 1's RPlayerCamera (0x310 bytes). +0x10 its matrix in the world, a row an axis (+0x30 the way it looks, +0x40 where it is), +0x60 the half angle, +0x70 the view matrix, +0x130 the mode in use, +0x134 the one before, +0x138 the last one the button reaches. y is up.
- The modes: the table [0x1F6D88], 0x18 bytes an entry, [0x1F6DB4] of them, from data\render\camera.ini (31 sections, the same file in every level; only those of the level's car are taken, so an index means nothing across levels). The word at +8 is the kind and +0x14 its update: 0x01 Bumper (0x743D0), 0x02 Heli, the chase camera (0x7A240), 0x04 Spline (0x76AB0), 0x08 Ellipse, the pause's (0x799A0), 0x10 Fixed (0x771C0), 0x20 Tumble (0x79670), 0x40 animated (0x74520, 0x75C10), 0x80 Collision (0x792D0), 0x100 AutoDrive, a mounted gun's (0x778B0, PAL 0x77A20: so that function is the gun's only, not the car's as noted above). The dispatcher is 0x79230. Bytes at 0x1CE914..0x1CE91B are the special cameras' indices (0x1CE91B the cut scenes' Heli).
- A change of camera: 0x74F60 (thiscall: the mode, the word at 0x1CE424, five zeros), as the camera button's 0x751F0 calls it. The button (0x75020, 0x750B0) passes over the Bumper kind.
- Fixed matrices, of whichever view was made active last (0x1025A0): 0x253930 view, 0x2539F0 projection, 0x253A70 view projection, 0x253A30 model. The view builder 0x102400 takes a horizontal angle.
- Bindings in the levels' .viv: drivecfg.def has CHANGECAMERA on button 3 and CAMLOOKBACK on the right stick's y; povcfg0.def has CAMSPIN on button 3 (the 0.7 s spin noted earlier), CAMCENTRE on button 13, POVRELOAD on button 0.
