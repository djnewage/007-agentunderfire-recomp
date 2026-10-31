# Lessons from the earlier ports

What the PGR2 (XDK 5659), 007: Everything or Nothing (5659 QFE 4, two xbes), PGR1 (3948) and Forza (5849) ports
learned, as a checklist for this one. Collected 2026-10-27 from their PROGRESS.md and FINDINGS.md; their logs have the
evidence. **[console]** items are fixed in the SDK already unless marked; a new game meets them as "already handled"
or as the same class of bug in a new place.

## How long things took
- PGR2: day 1 XDK survey + signatures + first frame (the "dirty disc" screen) + title; day 2 menus, a race, first
  Windows build, smoke test; release 1.0 on day 6.
- EoN (closest to this game): skeleton to 120 s headless without a trap in about 45 minutes once the QFE's
  functions were named; level, characters and menus on day 2; Driving reached on Windows day 3; release day 4.
- PGR1 (a new XDK build, like 4831 here): disc to the 3948 signatures and layout, first boot, a race, smoke OK and a
  Windows build in one day. The first 3948 scan found 20 of 250 functions; the hand work brought it to 137.
- Forza (signatures existed): boot, sound, title, fibers, menus and a lit race in about a day.

## A new XDK build directory (PGR1's 3948 is the model)
- `sdk/xdk/<build>/` holds `signatures.toml` and `layout.toml` only. Seed from the nearest build (here 5659: 54 of 251
  functions, against 23 from 3948), then name the misses by hand, sign, rescan until the scan file reproduces.
- Name a missed function by matching its body against the same function in a game already named (numbers masked;
  PGR2's 5659, PGR1's 3948, EoN's QFE 4 are all in Ghidra: `~/ghidra-projects/{pgr2,pgr1,eon}`). Where bodies look
  alike, link order decides (libraries keep it). Otherwise name it by the push buffer methods it writes, the device
  fields it touches and its ret size.
- DSOUND/XAPI misses: `.XTLID` export ids are alphabetical by export name within a library, so the neighbours of an
  unknown id name it. Tiny thunks give false matches (EoN's PlayEx matched StopEx's thunk): check every by-constants
  match. NightFire's first scan offered a 0.60 "XInputGetCapabilities" that popped 4 bytes, not 8; the scanner now
  drops such matches in images without LTCG.
- Leave unlinked libraries at 0 (no XNet/XOnline here). A 0 symbol traps only if called: confirm "not linked" by no
  callers and no gap of the right size.
- `layout.toml`: every key cites the instruction of *this* game that proves it (setter stores and compares:
  SetRenderTarget, SetTexture, SetIndices, SetViewport, SetTransform, Swap/Present, Release, device teardown).
  Offsets move in bands (EoN's QFE 4: unchanged below +0x780, +4 to +0x7A0, +0x10 from +0x900; PGR2's 5659 tail was
  -0x10 from 5849).
- Behaviour that differs by build gets a layout key with a default for the others, never a hard-coded branch.
  3948 vs 5659 differences to place 4831 on: render/texture state numbering (`StateNumbering`, see
  `sdk/runtime/xdk/state_numbering.h`), frame surfaces inside the device vs pointers, Present(stdcall 4) vs Swap,
  GetBackBuffer/GetDepthStencilSurface out-pointers vs returns, SetVertexShaderConstant register + 96, present interval
  as a device field, D3DPRESENT_PARAMETERS ending at +0x34 (`PresentParams_BufferSurfaces = 0xFFFFFFFF`: reading past
  an old XDK's struct end froze PGR1's screen while draws kept counting).
- Some names cannot be signed (not on the linear sweep, a device field): write them down as such.

## Two xbes (EoN)
- One `add_xbe_image` per xbe, each with its scan file, hand config and hooks folder; `src/common` goes into both.
  `GameInfo::images` lists both; each exe knows which it is (EoN: `extern const bool kDriving` in each hooks.cpp).
- `launcher/game.json` needs `xbes` and `exes`, `release.bat` must zip both exes, and the release workflow's title
  must be quoted (the colon in "007:" broke EoN's YAML and nothing was published).
- Driving.xbe alone boots a developer fallback level and crashes: it needs the launch data default.xbe writes
  (`hdd/launch.bin`, deleted by the booted image). Keep a copy from a real handover in `.local/`; changing one byte
  picked another level in EoN. Headless: `sdk/harness/build.sh -r . -g generated/driving -o .local/harness_drv -n
  driving`, then `sdk/harness/run.sh -x .local/harness_drv/driving -g assets -o .local/run -t SECS -s PAD`.
- The second exe needs its own hooks, key table (`[driving_keys]`), HUD aspect; nothing carries over by itself.
- Name the second xbe's functions from the first's hand names, its data by the same instructions (check every use).
  Scans miss things that only matter later (EoN Driving's SetPalette: P8 textures had no palette): when something
  draws wrong, list the D3D calls that are not replaced.

## Check on day 1
- **Push buffers** (Harry asks for this at once): CreatePushBuffer, Begin/EndPushBuffer, RunPushBuffer and
  BeginFixup callers, plus game code writing the device's push pointer directly (inlined MakeSpace, hand-pushed
  constants as in PGR2). A raw NV2A header scan alone gave PGR1 a wrong "none". EoN used both kinds: commands pushed
  while recording, and prebuilt buffers replayed with fixups (bone matrices); the runtime handles both now.
- Disc facts in PROGRESS: title id, region, version, xbe SHA-1s, every library version.
- Region: a PAL disc needs `GameInfo::pal_console` or PAL-only movies and cutscenes are skipped (EoN).
- Media: sector size and cdrom-vs-hdd decide how a game mounts its archives (EoN skipped its .viv archives until
  XGetDiskSectorSize("D:\\") said 2048, and an .iso's handles were not cdrom). Test from an .iso
  (`extract-xiso -c`) as well as a folder. NightFire's `eurocom/filesys.d00`-`d10` and `driving/*.viv` are archives:
  `XBE_FILES_LOG=1` shows missing or short reads.
- Fibers (Forza), direct hardware register reads (XACT's APU counter, DirectSoundGetSampleTime: PGR2's menus froze),
  overlay video (UpdateOverlay), XMV movies, a second utility drive.

## Recurring failure classes
- **Unnamed XDK functions** run library code against hardware state the host never set up, or trap. Fix one at a
  time as the game hits them; host what the runtime lacks, then sign it.
- **Host stubs that skip side effects** the game relies on: tables filled (DirectSoundUseFullHRTF), slots recorded
  (XMountUtilityDrive), the device's texture slot on Swap(BYPASSCOPY).
- **Files:** a read at or past EOF succeeds with 0 bytes; CREATE_ALWAYS + SetEndOfFile keeps old clusters (FATX does
  not clear); `*.*` arrives as an empty name; ReadFileEx completions need the APC dispatcher; the first boot formats
  the cache partition (about 100 s; `KEEP_HDD=1` skips it). A game's fatal-error routine that formats the cache and
  shows a "dirty disc" screen: break on it and log its message and caller.
- **Timing:** honour the present interval (0x80000000 + vblanks) in Swap, or races ran 1.84x; instant disc reads
  overfilled EoN's streaming ring (`GameInfo::disc_rate = 6` MB/s); pad scripts count vblanks (the harness is paced).
- **Pushed state ordering:** commands pushed raw are read at the next draw, so every setter must walk pending pushes
  first (PGR2's car body off screen, EoN's MultiSample setters).
- **Depth:** W-buffering needs perspective-correct w scaled by ZScale; W-buffered draws are not far-clipped; depth
  bigger than the colour target is legal; a title can make its own frame buffers (EoN: no auto depth).
- **Vertices:** SetVertexShaderInput replaces streams and declaration; a declaration longer than the stride reads the
  next vertex; point sprites; skip draws whose strides read outside guest memory.
- **Visibility tests** are polled inside a frame: answer with the last count, never stall on a fence.
- **Fixed-size game tables** overrun by content far from the cause (PGR2): diff against a title update.
- **Memory:** contiguous allocations fail on fragmentation (best-fit now); a new shader object per select leaked
  12.5 GB an hour in EoN (soak found it).
- **Saves:** signatures are keyed with the title key (zero on the host); real-console saves need the loader's check
  overridden. A tester's saves (`E:\UDATA\<titleid>`) were the only way to reach locked missions headless in EoN.

## Debugging that worked
- `REF=N` frames (use 600+ or a `snap` file), `CAP=N` draw lists, `AUDIT=1`, frame log diffs (`XBE_FRAMELOG`,
  `framelog.py`), `XBE_DRAW_SKIP` / `XBE_DRAW_TEST` layer bisection, `memdump` / `poke`, `XBE_FILES_LOG`,
  `XBE_DSOUND_LOG`, `XBE_MEM_LOG`, `XBE_PERF`, F3 Windows captures with `XBE_CAPTURE_DIR` (read with
  `sdk/scripts/guestmem.py`).
- gdb: start the harness inside gdb (ptrace attach is refused on the VM); hardware watchpoints on 0x100000000 + guest
  address; recompiled functions are `__imp__sub_X(ctx, base)`, guest esp is the dword at ctx+16.
- Hook the game's own debug print behind an env var. Count draws instead of eyeballing.
- The reference renderer has bugs of its own (Forza's fog): compare with the GPU renderers or the HLSL before blaming
  the game. Harness-only artefacts are not worth chasing before a Windows report.

## Things that wasted time
- Chasing a symptom's data (PGR2's black car as lighting) when state ordering was the cause; workarounds made before a
  fundamental fix (re-check them after it); captures that flush every frame never catch timing flicker.
- Stray env vars in the tester's shell (the logs now list `XBE_*` and the game's own at start), stale SDK builds
  (build.bat updates the submodule), info-level diagnostics missing from release logs.
- SMOKE OK is not visual correctness: look at the frame shown at key screens after every SDK update.

## Windows vs the harness
- Static initializers in otherwise unreferenced files never run in the Windows exe: register from `GameInfo::init`
  and log a line that proves it.
- `scripts/wincheck.sh` and `scripts/shadercheck.sh` before sending a build; UNICODE is defined, NOMINMAX first.
- PowerShell sets environment variables with `$env:X="..."`, not `set`.
- A renderer that fails to start falls back to D3D11, or the player never reaches F1.

## From this port
- A new XDK build can be named in about an hour: align each library section with every earlier game's in link order and
  take names where the references agree and ret sizes match (.local/tools/align.py here). Then list the library functions
  game code calls that have no name and do those by hand: only those matter first.
- Host DirectSound objects are not the library's: any DSOUND function left unnamed crashes on them when called.
- The launch data file is deleted by the image that boots: copy it back before each gdb run, or the run shows a
  different problem (it cost a wrong trail here).
- A hardware watchpoint can stay silent; dumping the damaged memory and measuring how far it reaches found the row.
- F3 capture BMPs carry alpha: flatten them (`convert x.bmp -alpha off y.png`) or a surface with low alpha looks white.
- Check a "Windows only" bug in a full size headless frame before theorising; a small montage hid that the harness tore too.
- Fixed function features can simply be missing (vertex blending until this game): when geometry is wrong, compare what the
  vertex format asks for with what the generated shader reads.
- Test renderer changes in the harness built as the Windows exe too (`RT_THREAD=1 RAW=1`, its own `-o` folder): the render
  thread sends the backend only the state it lists, so a new field or transform in use can arrive as zeros.
- The harness's null backend answers visibility reports at once, so a poll that depends on the renderer being kicked only hangs
  on Windows. Holding reports until Flush in a temporary build reproduces such a hang headless.
- gdb slows a run several times: check a window of frames really overlaps the input before believing "nothing changed". Memory dumps cost nothing.
- A flag found by one memory diff is a guess: log when it flips across a whole run before relying on it.
- A file a running harness polls must appear whole (write elsewhere, then mv).
- Try what a button does in play and read the HUD before binding keys to it: names from code guessed the d-pad backwards.
- A test source that always answers hides code that only runs when input stops: make it report "no movement" as the real one does.
- A log line is a claim: read the run's output before writing it, not after. And a pad script's press can make the very state it then "finds".
- wincheck with no arguments checks only sdk/runtime: name the game's Windows code too (scripts/wincheck.sh src/common/menu_mouse.cpp).
- A P8 atlas in false colours shows no truth about which cell is which: get its palettes first, and test a theory (a poke) before blaming a tool.
- Open every page and level at least once in a headless sweep: Multiplayer's crash sat one menu away from the title.
- Check a test's window is in the state it claims to test: look at a frame of it. A number from the mission's intro passed twice as a confirmed fix, and Harry's mouse found it.
- When a measured fix and a player's report disagree, doubt the measurement first.
- wincheck compiles with exceptions on, and names no src/ file by default: check every src/ file with -fno-exceptions before saying a new image builds for Windows.
