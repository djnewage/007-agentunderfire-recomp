#!/usr/bin/env bash
# The shared smoke test (sdk/scripts/smoke.sh, this game's values in scripts/game.env): default.xbe -> bond_eng.xbe's first
# mission. Then this game's driving part, built as the Windows exe: bond_eng.xbe's menu relaunches into DRIVING.XBE's mis02a
# (the mission poked, as it is locked on a new game), which must keep drawing with visibility reports held (HOLD_REPORTS).
set -uo pipefail
"$(dirname "$0")/../sdk/scripts/smoke.sh" "$@" || exit 1
cd "$(dirname "$0")/.."
fail() { echo "SMOKE FAIL: $*"; exit 1; }
cmake --build build/linux --target recompile_AgentUnderFireDriving > .local/smoke_drv.out 2>&1 || fail "DRIVING.XBE does not recompile"
RT_THREAD=1 RAW=1 bash sdk/harness/build.sh -r . -g generated/driving -o .local/harness_drvrt -n driving >> .local/smoke_drv.out 2>&1 ||
  fail "DRIVING.XBE's harness did not build (.local/smoke_drv.out)"
D=.local/run_drv
rm -rf .local/run_bond && mkdir -p .local/run_bond/1 && cp -r .local/run/1/hdd .local/run_bond/1/ || fail "no hard disk to carry over"
( sleep 30; printf '2A9F98 4 1\n2A5D38 4 1\n' > .local/run_bond/1/poke ) &
bash sdk/harness/run.sh -x .local/harness_next/bond -g "${GAME:-assets}" -o .local/run_bond -t 45 -s scripts/pad/explore.txt >> .local/smoke_drv.out 2>&1
wait
L=.local/run_bond/1/logs/auf_001.log
grep -q 'relaunching into .*DRIVING.XBE' "$L" || fail "bond_eng.xbe did not relaunch into DRIVING.XBE"
rm -rf $D && mkdir -p $D/1 && cp -r .local/run_bond/1/hdd $D/1/ || fail "no hard disk to carry over"
HOLD_REPORTS=1 bash sdk/harness/run.sh -x .local/harness_drvrt/driving -g "${GAME:-assets}" -o $D -t 60 -s scripts/pad/drive.txt >> .local/smoke_drv.out 2>&1
L=$D/1/logs/auf_001.log
[ -f "$L" ] || fail "no log at $L"
grep -q 'launch data from the previous run' "$L" || fail "DRIVING.XBE did not get the launch data"
t=$(grep -E 'guest trap|is not implemented|trapping stub|unknown code' "$L" | head -3); [ -z "$t" ] || fail "traps: $t"
last=$(grep 'renderer: frame' "$L" | tail -1 | awk '{ t = $2; sub(/\]/, "", t); print int(t) }')
[ "${last:-0}" -ge 45 ] || fail "DRIVING.XBE's frames stopped at ${last:-0}s of 60s"
read -r f0 d0 < <(grep 'renderer: frame' "$L" | tail -6 | head -1 | sed -E 's/.*frame ([0-9]+), ([0-9]+) draws.*/\1 \2/')
read -r f1 d1 < <(grep 'renderer: frame' "$L" | tail -1 | sed -E 's/.*frame ([0-9]+), ([0-9]+) draws.*/\1 \2/')
per=$(( (d1 - d0) / (f1 - f0) ))
[ "$per" -gt 100 ] || fail "DRIVING.XBE not drawing: $per draws a frame"
echo "SMOKE OK: DRIVING.XBE's mis02a draws $per a frame with reports held, as the Windows exe builds"
