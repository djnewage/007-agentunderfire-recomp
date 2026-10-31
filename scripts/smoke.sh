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
# moved into place whole: the harness reads and removes poke at the next present, even an empty one
( sleep 30; printf '2A9F98 4 1\n2A5D38 4 1\n' > .local/run_bond/poke.tmp && mv .local/run_bond/poke.tmp .local/run_bond/1/poke ) &
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
# French and German: bond_eng.xbe's sources at those builds' addresses (tools/map_bond.py). Each must reach its first mission, its own
# files open, the mouse's hook in force, PC Settings' row made; and the sources must be as the tool writes them.
python3 tools/map_bond.py fre ger > .local/smoke_lang.out 2>&1 || fail "tools/map_bond.py left an address unmapped (.local/smoke_lang.out)"
git diff --quiet -- src/bond_fre src/bond_ger config/bond_fre.toml config/bond_ger.toml src/bond/addr_fre.h src/bond/addr_ger.h || fail "the language sources are not as tools/map_bond.py writes them: commit them"
cmake --build build/linux --target recompile_AgentUnderFireBondFre recompile_AgentUnderFireBondGer >> .local/smoke_lang.out 2>&1 || fail "the language xbes do not recompile"
for l in fre ger; do
    RT_THREAD=1 RAW=1 bash sdk/harness/build.sh -r . -g generated/bond_$l -o .local/harness_$l -n bond_$l >> .local/smoke_lang.out 2>&1 || fail "bond_$l does not build"
done
for l in fre ger; do
    d=.local/run_$l; rm -rf $d && mkdir -p $d/1 && cp -r .local/run/1/hdd $d/1/ || fail "no hard disk to carry over"
    python3 - "$l" unused $d/1/hdd/launch.bin <<'PY' || fail "no launch data for $l"
import sys
l=sys.argv[1]; d=bytearray(open('.local/hand/launch_bond.bin','rb').read())
o=d.index(b'BOND_ENG.XBE'); d[o:o+12]=('BOND_%s.XBE'%l.upper()).encode()
loc={'fre':b'IDL_FR.LOC','ger':b'IDL_GER.LOC'}[l]; o=d.index(b'IDL_ENG.LOC'); d[o:o+len(loc)+1]=loc+b'\0'
open(sys.argv[3],'wb').write(d)
PY
    AUF_MOUSE_TEST=20,0,100,200 bash sdk/harness/run.sh -x .local/harness_$l/bond_$l -g "${GAME:-assets}" -o $d -t 110 -s scripts/pad/explore.txt >> .local/smoke_lang.out 2>&1 &
done
wait
for l in fre ger; do
    L=.local/run_$l/1/logs/auf_001.log; U=$(echo $l | tr a-z A-Z)
    grep -q crash: .local/run_$l/1/run.out && fail "bond_$l crashed"
    grep -q "1P_$U.VIV" $L || fail "bond_$l did not open its own files"
    grep -q 'PC Settings is a row' $L || fail "bond_$l: no PC Settings row"
    grep -q 'the mouse turns the view directly' $L || fail "bond_$l: the mouse's hook was not reached"
    D=$(grep -h 'renderer: frame' $L | tail -1 | grep -oE '[0-9]+ draws' | cut -d' ' -f1); [ "${D:-0}" -gt 500000 ] || fail "bond_$l: only ${D:-0} draws (no mission?)"
    echo "SMOKE OK: bond_$l reaches its mission, $D draws"
done

