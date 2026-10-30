#include "app/app.h"

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
    .options = {},
    .load = nullptr,
    .save = nullptr,
    .init = nullptr,
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = nullptr,
    .rumble = nullptr,
    .pal_console = true,                  // a PAL disc (region 4)
};
