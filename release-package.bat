@echo off
setlocal
rem This disc's package for a release, from build.bat's output (run that first, with the disc in assets): the runtime and the
rem compiler, no game code. Run it once on each disc, each in a checkout of its own, and attach both zips and their .json.
cd /d "%~dp0"
if exist assets\bond_eng.xbe (
    call sdk\scripts\release-package.bat AgentUnderFire-PAL-package.zip AgentUnderFire AgentUnderFireBond AgentUnderFireBondFre AgentUnderFireBondGer AgentUnderFireDriving
) else (
    call sdk\scripts\release-package.bat AgentUnderFire-USA-package.zip AgentUnderFire AgentUnderFireBond AgentUnderFireDriving
)
endlocal
