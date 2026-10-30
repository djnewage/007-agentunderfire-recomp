#include <algorithm>
#include <array>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <toml++/toml.hpp>
#endif

#include "app/app.h"
#include "common/auf.h"
#include "core/input.h"
#include "core/log.h"

namespace auf {
bool skip_intro = false;
uint32_t fov_percent = 100;

float FovScale() {
    uint32_t pct = std::clamp<uint32_t>(fov_percent, 50, 150);
    static uint32_t said = 100;
    if (said != pct) {
        said = pct;
        LOGR("auf: field of view %u%% of the game's", pct);
    }
    return float(pct) / 100;
}

float FrameWidth() {
    static const float forced = [] {  // AUF_WIDE=k headless (no screen)
        const char* e = std::getenv("AUF_WIDE");
        return e && *e ? float(std::atof(e)) : 0.0f;
    }();
    float k = forced > 0 ? forced : app::FrameAspect() / (4.0f / 3.0f);
    static float said = 0;
    if (k != said) {
        said = k;
        LOGR("auf: the 3D views are %.3f times the game's 4:3", k);
    }
    return k;
}
}  // namespace auf

namespace {
const bool kDrv = auf::kImage == auf::kDriving;

// after the settings load: the harness's environment overrides them; each setting in effect is logged
void Init() {
    if (const char* e = std::getenv("AUF_SKIP_INTRO"); e && *e) auf::skip_intro = *e == '1';
    if (const char* e = std::getenv("AUF_FOV"); e && *e) auf::fov_percent = uint32_t(std::atoi(e));
    if (auf::skip_intro) LOGR("auf: the logo movies are skipped (skip_intro)");
}

// keyboard and mouse (Windows virtual key codes: this file builds for the headless harness too)
constexpr uint16_t kMouseLeft = 0x01, kMouseRight = 0x02, kMouseMiddle = 0x04, kTab = 0x09, kEnter = 0x0D,
                   kEscape = 0x1B, kSpace = 0x20, kArrowLeft = 0x25, kArrowUp = 0x26, kArrowRight = 0x27, kArrowDown = 0x28,
                   kLeftShift = 0xA0, kLeftCtrl = 0xA2, kBackspace = 0x08;
// on foot, as the game's Controller Setup page lists it: R fire, L manual aim, Y jump, White cycle weapon, Black alt.
// fire, B use gadget, X action, reload and open, A crouch, the d-pad weapon and gadget, Back center view; the mouse looks
std::array<uint16_t, 2> ShooterKeys(int c) {
    using namespace input;
    switch (c) {
        case kLSUp: return {'W', 0};
        case kLSDown: return {'S', 0};
        case kLSLeft: return {'A', 0};
        case kLSRight: return {'D', 0};
        case kRT: return {kMouseLeft, 0};    // fire
        case kLT: return {kMouseRight, 0};   // manual aim
        case kA: return {'C', kLeftCtrl};    // crouch (Enter selects in the menus: menu_mouse.cpp)
        case kX: return {'E', 'R'};          // action, reload, open
        case kY: return {kSpace, 0};         // jump
        case kB: return {'Q', kBackspace};   // use gadget; back in the menus
        case kWhite: return {'X', 0};        // cycle weapon
        case kBlack: return {'F', kMouseMiddle};  // alt. fire
        case kUp: return {kArrowUp, '3'};    // the d-pad: the gadget up and down, the weapon left and right (the wheel too)
        case kDown: return {kArrowDown, '4'};
        case kLeft: return {kArrowLeft, '1'};
        case kRight: return {kArrowRight, '2'};
        case kBack: return {kTab, 'V'};      // center view
        case kStart: return {kEscape, 'P'};
        default: return {0, 0};
    }
}
// driving, the layout of NightFire's later version of this engine: R gas, L brake, the left stick steers, A fire, B
// handbrake, Y camera, X gadget, Black and White the weapon (the wheel too), the right stick's click looks back. On a mounted
// gun the mouse plays the right stick, its buttons fire and zoom and R reloads (src/common/menu_mouse.cpp)
std::array<uint16_t, 2> DrivingKeys(int c) {
    using namespace input;
    switch (c) {
        case kRT: return {'W', 0};
        case kLT: return {'S', 0};
        case kLSLeft: return {'A', kArrowLeft};
        case kLSRight: return {'D', kArrowRight};
        case kLSUp: return {kArrowUp, kLeftShift};
        case kLSDown: return {kArrowDown, kLeftCtrl};
        case kA: return {kMouseLeft, kEnter};
        case kB: return {kSpace, kBackspace};
        case kX: return {kMouseRight, 'E'};
        case kY: return {'C', 0};
        case kBlack: return {'Q', 0};
        case kWhite: return {'Z', 0};
        case kRThumb: return {'V', kMouseMiddle};
        case kLeft: return {'1', 0};
        case kRight: return {'2', 0};
        case kUp: return {'3', 0};
        case kDown: return {'4', 0};
        case kStart: return {kEscape, 'P'};
        case kBack: return {kTab, 0};
        default: return {0, 0};
    }
}
std::array<uint16_t, 2> Keys(int c) { return kDrv ? DrivingKeys(c) : ShooterKeys(c); }

#ifdef _WIN32  // the settings file (the headless harness has none)
// One settings file for the exes, each with its own keys: the runtime reads and writes [keys] as this exe's (only
// what differs from its defaults); DRIVING.XBE's are kept as [driving_keys] instead, and each exe writes the other's
// table back as it read it (EoN's way)
constexpr const char* kOtherKeys[2] = {"driving_keys", "keys"};  // by kDriving: the table this exe keeps unread
toml::table g_other_keys;

void LoadKeys(const void* table) {
    const toml::table& t = *static_cast<const toml::table*>(table);
    const toml::table* other = t[kOtherKeys[kDrv]].as_table();
    g_other_keys = other ? *other : toml::table{};
    if (!kDrv) return;
    // the runtime took [keys], the game's, as these: the driving defaults and [driving_keys] instead
    app::Settings& s = app::Get();
    for (int c = 0; c < input::kControls && c < int(s.keys.size()); c++) s.keys[c] = DrivingKeys(c);
    if (const toml::table* mine = t["driving_keys"].as_table())
        for (auto& [name, v] : *mine) {
            int c = input::ControlByName(std::string(name.str()).c_str());
            const toml::array* a = v.as_array();
            if (c < 0 || c >= int(s.keys.size()) || !a) continue;
            s.keys[c] = {0, 0};
            for (size_t i = 0; i < 2 && i < a->size(); i++) s.keys[c][i] = app::KeyFromName((*a)[i].value_or(std::string()));
        }
}

void SaveKeys(void* table) {
    toml::table& t = *static_cast<toml::table*>(table);
    if (kDrv) {
        // the runtime wrote this exe's as [keys]
        if (toml::table* mine = t["keys"].as_table()) {
            toml::table copy = *mine;
            t.erase("keys");
            t.insert("driving_keys", copy);
        }
    }
    if (!g_other_keys.empty()) t.insert_or_assign(kOtherKeys[kDrv], g_other_keys);
}
#else
constexpr void (*LoadKeys)(const void*) = nullptr;
constexpr void (*SaveKeys)(void*) = nullptr;
#endif
}  // namespace

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"007: Agent Under Fire",    // window and splash
    .short_title = L"Agent Under Fire",   // message boxes
    .name = L"AgentUnderFire",            // window classes, crash dumps, handover event
    .log = "auf",                         // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    // default.xbe launches the language's xbe; the runtime matches names lower case
    .images = {{"default.xbe", "AgentUnderFire"},
               {"bond_eng.xbe", "AgentUnderFireBond"},
               {"driving.xbe", "AgentUnderFireDriving"}},
    .options = {{"skip_intro", &auf::skip_intro, nullptr, "Skip intro movies", "Starts without the EA, MGM and logo movies"},
                {"fov_percent", nullptr, &auf::fov_percent, "Field of view (%)",
                 "50 to 150; 100 is the game's. On foot and driving; the scope zooms in proportion"}},
    .load = LoadKeys,                     // DRIVING.XBE's keys apart (DrivingKeys)
    .save = SaveKeys,
    .init = Init,                         // after the settings load, before the window and boot
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = auf::MenuMouse,                // changes a polled pad: the mouse in the menus
    .rumble = nullptr,
    .hud_aspect = 4.0f / 3.0f,            // the game lays out its 2D for 4:3: kept in the middle of a wider frame
    .squeeze_screen_programs = true,      // DRIVING.XBE draws its HUD and menus through vertex programs
    .sharpen_2d = true,                   // text and HUD pictures enlarged sharp (Settings::sharp_2d turns it off)
    .pal_console = true,                  // a PAL disc (region 4)
    .mouse_look = true,                   // the mouse looks and aims
    .mouse_direct = auf::kImage == auf::kBond,  // src/bond/mouse.cpp; a mounted gun takes it as the right stick
    .default_keys = Keys,                 // a shooter's keys on foot, a driving layout in DRIVING.XBE
};
