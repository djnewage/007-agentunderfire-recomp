# 007: Agent Under Fire (original Xbox) - static recompilation

Goal: a native Windows port built with the XboxXBE-Recomp toolkit in `sdk/` (a git submodule of the fork).
Read `sdk/README.md`, `sdk/harness/README.md`, `LESSONS.md`, `PROGRESS.md` and `FINDINGS.md` at the start of every
session. LESSONS.md is what the PGR2, EoN, PGR1, Forza and NightFire ports learned; check it before chasing a problem.

## Environment
- Headless Linux VM: build the Linux tools, regenerate code, run the game in the headless harness. The real Windows
  exe is built and play-tested on Windows separately (Harry).
- Game files (Harry's own disc, /home/harry/agentunderfire.iso) are in `assets/`. Never commit, upload or paste them,
  or generated code. `assets/`, `generated/`, `build/`, `.local/` and `captures/` are git-ignored; keep it that way.
- Signatures in `sdk/xdk/*/signatures.toml` are hashes and lengths only. Never store library bytes.

## Commands
`scripts/*.sh` are wrappers for the shared scripts in `sdk/scripts`; this game's values are in `scripts/game.env`.
- `scripts/linux-tools.sh`   configure for Linux, build `xbe_symbols` and `xbe_recomp`
- Scans (output only, never edited; hand entries go in the hand config that includes each):
  `build/linux/xbe_symbols assets/default.xbe sdk/xdk --out config/game.toml` (hand: config/auf.toml)
  `build/linux/xbe_symbols assets/bond_eng.xbe sdk/xdk --out config/bond_game.toml --name bond` (hand: config/bond.toml)
  `build/linux/xbe_symbols assets/DRIVING.XBE sdk/xdk --out config/driving_game.toml --name driving` (hand: config/driving.toml)
- Signing: `build/linux/xbe_symbols --sign config/bond.toml config/driving.toml config/auf.toml --out sdk/xdk/4039/signatures.toml`,
  then every rescan must reproduce its scan file.
- `scripts/recompile.sh` regenerates `generated/game`; `cmake --build build/linux --target recompile_AgentUnderFireBond`
  (and `recompile_AgentUnderFireDriving`) the others.
- `scripts/harness.sh [secs] [pad] [ENV=..]` builds + runs default.xbe headless (`REF=N`, `KEEP_HDD=1`, `snap`/`memdump`/
  `poke` files). The other images: `sdk/harness/build.sh -r . -g generated/bond -o .local/harness_bond -n bond`, then
  `sdk/harness/run.sh -x .local/harness_bond/bond -g assets -o .local/run -t SECS -s PAD`.
- Ghidra: `python3 sdk/tools/symbols/xbe2pe.py config/auf.toml .local/game.exe` (and bond, driving), imported into
  `~/ghidra-projects/auf` as game.exe, bond.exe, driving.exe; `scripts/ghidra.sh Decomp|Disasm|FindOperand|Callers|
  DumpRange|PushBufferScan|DeviceRefs ...` reads game.exe, `GHIDRA_PROGRAM=bond.exe scripts/ghidra.sh ...` the others.

## The game
- PAL disc, title 0x4541000D (EA), version 2. XDK 4039 for every library.
- `default.xbe`: launcher and `.mpc` movie player (target `AgentUnderFire`, hooks `src/game`). `bond_eng.xbe`,
  `bond_fre.xbe`, `bond_ger.xbe`: the game (a Quake III derivative), one build per language, code at different addresses
  (target `AgentUnderFireBond`, hooks `src/bond`, English only so far). `DRIVING.XBE`: EA's driving levels, EAGL as in
  NightFire's Driving.xbe (target `AgentUnderFireDriving`, hooks `src/driving`, data in `driving.dat/`).

## Plan
1. XDK 4039 (SDK branch `agentunderfire`): name the functions the three xbes call and sign them into `sdk/xdk/4039`;
   re-derive every `layout.toml` key from this game's code, citing the instruction. For each 3948/4831 difference find
   which side 4039 is on (LESSONS.md).
2. Push buffer check: callers of the functions, not only NV2A headers. Report it to Harry at once.
3. Boot default.xbe, hand over to bond_eng.xbe; menus, the first mission, a driving mission, smoke test.
4. French and German builds, Windows parity, PC features, achievements, launcher, release.

## Working rules
- One problem at a time. After each meaningful step, a dated entry at the top of PROGRESS.md's log (what changed,
  evidence, next step), its Status block updated, and a commit.
- Game fixes go in `src/game`, `src/bond`, `src/driving` (X86_FUNC overrides, [[hook]]/[[function]]/[[switch]] in the
  hand configs), not in generated code. SDK changes go in `sdk/` as their own commits on branch `agentunderfire` of the
  fork, merged into its `main` only after every game's smoke test passes (PGR2, PGR1 plus its frame check, EoN,
  NightFire, this one once it has one); fetch + merge (never rebase), publish the SDK first, then this repo's pointer.
- A change to xbe_symbols itself must leave every other game's scan unchanged.
- Game code registers from `GameInfo::init`, never a static initializer (one never ran in a Windows exe).
- Diagnostics a player's log must show are report level (LOGR) or errors: release logs drop info lines.
- Never kill a harness by name (other sessions run theirs): by PID.
- If the same problem survives 3 genuinely different attempts, stop: write up what was tried in PROGRESS.md, commit,
  and say so.
- One branch per task; when it ends and everything builds, fast-forward `main` (`git merge --ff-only`).
- Never force-push. Never rewrite history.
