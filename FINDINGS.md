# 007: Agent Under Fire recomp - findings

What the work has established, by topic. PROGRESS.md has the dated log with the evidence for each item. Each item is
marked **[console]** (how the Xbox, its XDK or the runtime behaves: the fix is in the SDK) or **[game]** (this game's
own code and data, with its addresses).

## The image
- **[game]** Five xbes: default.xbe (launcher, `.mpc` movies), bond_eng/fre/ger.xbe (the game, a Quake III derivative
  by its credits and cvars), DRIVING.XBE (EA's driving levels, EAGL). Their strings also name BOND.XBE,
  MOVIEXBOX_RELEASE.XBE and ASSET_*.BIG, which the disc does not have.
