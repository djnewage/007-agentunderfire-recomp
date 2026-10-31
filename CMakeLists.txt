# 007: Agent Under Fire (original Xbox), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the SDK: sdk/ here, two folders up inside the SDK's templates, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/hk21x/XboxXBERecomp.git GIT_TAG main)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/(game|bond|bond_fre|bond_ger|driving)/")

# partitions as xbe_symbols wrote them into each scan file (none yet: build.bat writes them, the change reconfigures)
function(xbe_partitions scan out)
    set(n 8)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/${scan})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${scan})
        file(STRINGS config/${scan} line REGEX "^partitions *=")
        string(REGEX MATCH "[0-9]+" found "${line}")
        if(found)
            set(n ${found})
        endif()
    endif()
    set(${out} ${n} PARENT_SCOPE)
endfunction()
xbe_partitions(game.toml GAME_PARTITIONS)
xbe_partitions(bond_game.toml BOND_PARTITIONS)
xbe_partitions(driving_game.toml DRIVING_PARTITIONS)
xbe_partitions(bond_fre_game.toml BOND_FRE_PARTITIONS)
xbe_partitions(bond_ger_game.toml BOND_GER_PARTITIONS)

# target name = GAME_EXE in build.bat, xbe = GAME_XBE: default.xbe is the disc's launcher and movie player
add_xbe_image(AgentUnderFire default.xbe auf.toml ${GAME_PARTITIONS} game)
# each language's game is an xbe default.xbe launches (XLaunchNewImage): an exe of its own. src/bond_fre and src/bond_ger are
# src/bond again, at those builds' addresses (tools/map_bond.py)
add_xbe_image(AgentUnderFireBond bond_eng.xbe bond.toml ${BOND_PARTITIONS} bond)
add_xbe_image(AgentUnderFireBondFre bond_fre.xbe bond_fre.toml ${BOND_FRE_PARTITIONS} bond_fre)
add_xbe_image(AgentUnderFireBondGer bond_ger.xbe bond_ger.toml ${BOND_GER_PARTITIONS} bond_ger)
# the driving levels are another
add_xbe_image(AgentUnderFireDriving DRIVING.XBE driving.toml ${DRIVING_PARTITIONS} driving)

# launcher/icon.ico, when there is one: the game's icon too (resource 1, which the window class loads)
if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/launcher/icon.ico)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/game_icon.rc "1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/launcher/icon.ico\"\n")
    foreach(t AgentUnderFire AgentUnderFireBond AgentUnderFireBondFre AgentUnderFireBondGer AgentUnderFireDriving)
        target_sources(${t} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/game_icon.rc)
    endforeach()
endif()
