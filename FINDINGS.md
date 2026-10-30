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

## Push buffers
- **[game]** None in any of the three xbes.
