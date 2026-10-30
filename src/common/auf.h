#pragma once
// Agent Under Fire's own settings and what its executables share (default.xbe: the logos and movies; bond_eng.xbe: the
// game; DRIVING.XBE: the driving levels)
#include <cstdint>

#include "core/input.h"

namespace auf {
enum Image { kDefault, kBond, kDriving };
extern const int kImage;      // which exe this is (each image's hooks.cpp)
extern bool skip_intro;
extern bool uncapped;         // on foot paced by the frame rate cap, not the console's 30 (src/bond/framerate.cpp)
extern uint32_t fov_percent;  // the 3D views' field of view, percent of the game's
float FovScale();             // fov_percent as a factor, 0.5 to 1.5
float FrameWidth();           // the frame's width over the game's 4:3, 1 on a 4:3 screen
bool MenuUp();                // a menu has the controller (each image's own test): the mouse plays A, B and the d-pad
bool OnGun();                 // DRIVING.XBE: a mounted gun's controls are in force, not the car's
bool Zoomed();                // bond_eng.xbe: through a scope
void ScopeZoom(int notches);  // bond_eng.xbe: the wheel's notches, up nearer
bool PlayerFree();            // bond_eng.xbe: Bond under the player's control, not a cutscene or a hint waiting for A
bool ButtonPictures(uint32_t header, uint32_t palette);  // GameInfo::texture_bound (src/common/buttons.cpp)
void RefreshButtons();        // each poll
void MenuMouse(int port, input::Pad& p);
}  // namespace auf
