// The PC Settings page's rows: the same in bond_eng.xbe's menus and DRIVING.XBE's pause. The games' fonts are Latin-1.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "app/app.h"
#include "common/auf.h"

namespace auf {
namespace {
const char* const kRowNames[3] = {"PC Settings", "Param\xE8tres PC", "PC-Einstellungen"};
const char* const kLabels[3][kPcRows] = {
    {"Field of View", "Mouse Sensitivity", "Render Scale", "MSAA", "FXAA", "Sharpening", "Texture Filtering", "More Settings"},
    {"Champ de vision", "Sensibilit\xE9 souris", "\xC9" "chelle de rendu", "MSAA", "FXAA", "Nettet\xE9", "Filtrage des textures", "Plus d'options"},
    {"Sichtfeld", "Mausempfindlichkeit", "Renderskalierung", "MSAA", "FXAA", "Sch\xE4rfe", "Texturfilterung", "Weitere Optionen"}};
const char* const kOff[3] = {"Off", "Non", "Aus"};
const char* const kOn[3] = {"On", "Oui", "An"};
const char* const kGames[3] = {"Game's", "Du jeu", "Vom Spiel"};
}  // namespace

const char* PcRowName(int lang) { return kRowNames[lang % 3]; }
const char* PcLabel(int row, int lang) { return kLabels[lang % 3][row % kPcRows]; }

namespace {
std::string Plain(int row, int lang) {
    const app::Settings& s = app::Get();
    lang %= 3;
    char b[32] = "";
    switch (row) {
        case 0: std::snprintf(b, sizeof b, "%u%%", fov_percent); break;
        case 1: std::snprintf(b, sizeof b, "%u%%", s.mouse_sensitivity); break;
        case 2: std::snprintf(b, sizeof b, "%u%%", s.render_percent); break;
        case 3: {
            const char* names[] = {kOff[lang], "2x", "4x", "8x"};
            return names[s.msaa % 4];
        }
        case 4: return s.fxaa ? kOn[lang] : kOff[lang];
        case 5:
            if (!s.sharpen) return kOff[lang];
            std::snprintf(b, sizeof b, "%u%%", s.sharpen);
            break;
        case 6:
            if (!s.aniso) return kGames[lang];
            std::snprintf(b, sizeof b, "%ux", s.aniso);
            break;
        default: break;
    }
    return b;
}
}  // namespace

// the arrows are for the mouse: a click on the left one lowers (PcClickDir)
std::string PcValue(int row, int lang) {
    std::string v = Plain(row, lang);
    return v.empty() ? v : "< " + v + " >";
}

int PcClickDir(float value_x) {
    float x, y;
    return (PointerButtons() & 1) && PointerAt(x, y) && x < value_x + 22 ? -1 : 1;
}

int Language() {
    const uint32_t l = app::Get().language;  // GameInfo::languages' numbers; XBE_LANGUAGE to test one
    const char* e = std::getenv("XBE_LANGUAGE");
    const uint32_t v = e && *e ? uint32_t(std::atoi(e)) : l;
    return v == 4 ? 1 : v == 3 ? 2 : 0;
}

void PcStep(int row, int dir) {
    app::Settings& s = app::Get();
    switch (row) {
        case 0: fov_percent = uint32_t(std::clamp(int(fov_percent) + dir * 5, 50, 150)); break;
        case 1: s.mouse_sensitivity = uint32_t(std::clamp(int(s.mouse_sensitivity) + dir * 10, 10, 500)); break;
        case 2: s.render_percent = uint32_t(std::clamp(int(s.render_percent) + dir * 25, 25, 200)); break;
        case 3: s.msaa = (s.msaa + 4 + uint32_t(dir)) % 4; break;
        case 4: s.fxaa = !s.fxaa; break;
        case 5: s.sharpen = uint32_t(std::clamp(int(s.sharpen) + dir * 10, 0, 100)); break;
        case 6: {
            const uint32_t order[] = {0, 2, 4, 8, 16};
            int at = 0;
            for (int i = 0; i < 5; i++)
                if (order[i] == s.aniso) at = i;
            s.aniso = order[(at + 5 + dir) % 5];
            break;
        }
        case 7: app::ToggleOverlay(); return;
        default: return;
    }
    app::Save();
}
}  // namespace auf
