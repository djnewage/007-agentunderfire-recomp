// The PC Settings page's rows: the same in bond_eng.xbe's menus and DRIVING.XBE's pause. Its first row chooses the tab; a row whose
// label ends in * applies at the next start. Each changes what the settings window changes, as it does. The games' fonts are Latin-1.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "app/app.h"
#include "common/auf.h"

namespace auf {
namespace {
enum Id {
    kRenderScale, kMsaa, kFxaa, kSharpening, kFiltering, kSharpText, kFrameCounter,
    kDisplayMode, kVsync, kFrameCap, kAspect, kRenderer, kFps60, kFov,
    kMouseSpeed, kInvertMouse, kAutoAim,
    kLanguage, kSkipIntro, kPauseUnfocused, kMore, kIds
};
const char* const kRowNames[3] = {"PC Settings", "Param\xE8tres PC", "PC-Einstellungen"};
const char* const kTabLabel[3] = {"Page", "Page", "Seite"};
const char* const kTabNames[3][4] = {{"Graphics", "Display", "Controls", "Game"},
                                     {"Graphismes", "Affichage", "Commandes", "Jeu"},
                                     {"Grafik", "Anzeige", "Steuerung", "Spiel"}};
const char* const kLabels[3][kIds] = {
    {"Render Scale", "MSAA", "FXAA", "Sharpening", "Texture Filtering", "Sharp Text and HUD", "Frame Counter",
     "Display Mode", "Vertical Sync", "Frame Rate Cap", "Aspect Ratio *", "Renderer *", "60 FPS on Foot *", "Field of View",
     "Mouse Sensitivity", "Invert Mouse", "Auto-aim",
     "Language *", "Skip Intro Movies", "Pause in Background", "More Settings"},
    {"\xC9" "chelle de rendu", "MSAA", "FXAA", "Nettet\xE9", "Filtrage des textures", "Texte et ATH nets", "Compteur d'images",
     "Mode d'affichage", "Synchro verticale", "Limite d'images", "Format d'image *", "Moteur de rendu *", "60 i/s \xE0 pied *", "Champ de vision",
     "Sensibilit\xE9 souris", "Inverser la souris", "Vis\xE9" "e auto",
     "Langue *", "Passer les intros", "Pause en arri\xE8re-plan", "Plus d'options"},
    {"Renderskalierung", "MSAA", "FXAA", "Sch\xE4rfe", "Texturfilterung", "Scharfer Text und HUD", "Bildz\xE4hler",
     "Anzeigemodus", "V-Sync", "Bildratenlimit", "Seitenverh\xE4ltnis *", "Renderer *", "60 FPS zu Fu\xDF *", "Sichtfeld",
     "Mausempfindlichkeit", "Maus invertieren", "Zielhilfe",
     "Sprache *", "Intros \xFC" "berspringen", "Pause im Hintergrund", "Weitere Optionen"}};
const char* const kOff[3] = {"Off", "Non", "Aus"};
const char* const kOn[3] = {"On", "Oui", "An"};
const char* const kGames[3] = {"Game's", "Du jeu", "Vom Spiel"};
const char* const kNoCap[3] = {"None", "Aucune", "Keins"};
const char* const kModes[3][3] = {{"Fullscreen", "Borderless", "Window"}, {"Plein \xE9" "cran", "Sans bordure", "Fen\xEA" "tre"}, {"Vollbild", "Rahmenlos", "Fenster"}};
const char* const kAspects[3][4] = {{"Screen's", "4:3", "16:9", "Fill"}, {"De l'\xE9" "cran", "4:3", "16:9", "Remplir"}, {"Vom Bildschirm", "4:3", "16:9", "F\xFCllen"}};

struct Tab {
    int count;
    int ids[kPcRows - 1];
};
const Tab kTabs[4] = {{7, {kRenderScale, kMsaa, kFxaa, kSharpening, kFiltering, kSharpText, kFrameCounter}},
                      {7, {kDisplayMode, kVsync, kFrameCap, kAspect, kRenderer, kFps60, kFov}},
                      {4, {kMouseSpeed, kInvertMouse, kAutoAim, kMore}},
                      {4, {kLanguage, kSkipIntro, kPauseUnfocused, kMore}}};
int g_tab = 0;

int IdOf(int row) { return kTabs[g_tab].ids[(row - 1) % kTabs[g_tab].count]; }
template <class T, size_t N>
T Next(const T (&order)[N], T now, int dir) {
    int at = 0;
    for (size_t i = 0; i < N; i++)
        if (order[i] == now) at = int(i);
    return order[(at + int(N) + dir) % int(N)];
}

std::string Plain(int id, int lang) {
    const app::Settings& s = app::Get();
    char b[32] = "";
    auto onoff = [&](bool v) { return std::string(v ? kOn[lang] : kOff[lang]); };
    switch (id) {
        case kRenderScale: std::snprintf(b, sizeof b, "%u%%", s.render_percent); break;
        case kMsaa: {
            const char* names[] = {kOff[lang], "2x", "4x", "8x"};
            return names[s.msaa % 4];
        }
        case kFxaa: return onoff(s.fxaa);
        case kSharpening:
            if (!s.sharpen) return kOff[lang];
            std::snprintf(b, sizeof b, "%u%%", s.sharpen);
            break;
        case kFiltering:
            if (!s.aniso) return kGames[lang];
            std::snprintf(b, sizeof b, "%ux", s.aniso);
            break;
        case kSharpText: return onoff(s.sharp_2d);
        case kFrameCounter: return onoff(s.show_fps);
        case kDisplayMode: return kModes[lang][int(s.display_mode) % 3];
        case kVsync: return onoff(s.vsync);
        case kFrameCap:
            if (!s.fps_cap) return kNoCap[lang];
            std::snprintf(b, sizeof b, "%u", s.fps_cap);
            break;
        case kAspect: return kAspects[lang][s.aspect % 4];
        case kRenderer: return s.renderer % 2 ? "Direct3D 11" : "Direct3D 12";
        case kFps60: return onoff(uncapped);
        case kFov: std::snprintf(b, sizeof b, "%u%%", fov_percent); break;
        case kMouseSpeed: std::snprintf(b, sizeof b, "%u%%", s.mouse_sensitivity); break;
        case kInvertMouse: return onoff(s.mouse_invert_y);
        case kAutoAim: return onoff(auto_aim);
        case kLanguage: return s.language == 3 ? "Deutsch" : s.language == 4 ? "Fran\xE7" "ais" : "English";
        case kSkipIntro: return onoff(skip_intro);
        case kPauseUnfocused: return onoff(s.pause_unfocused);
        default: break;
    }
    return b;
}
}  // namespace

const char* PcTabName() { return "RECOMP"; }
const char* PcRowName(int lang) { return kRowNames[lang % 3]; }
int PcRows() { return kTabs[g_tab].count + 1; }
const char* PcLabel(int row, int lang) { return row ? kLabels[lang % 3][IdOf(row)] : kTabLabel[lang % 3]; }

// the arrows are for the mouse: a click on the left one lowers (PcClickDir)
std::string PcValue(int row, int lang) {
    lang %= 3;
    const std::string v = row ? Plain(IdOf(row), lang) : kTabNames[lang][g_tab];
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
    if (!row) {
        g_tab = (g_tab + 4 + dir) % 4;
        return;
    }
    app::Settings& s = app::Get();
    switch (IdOf(row)) {
        case kRenderScale: s.render_percent = uint32_t(std::clamp(int(s.render_percent) + dir * 25, 25, 200)); break;
        case kMsaa: s.msaa = (s.msaa + 4 + uint32_t(dir)) % 4; break;
        case kFxaa: s.fxaa = !s.fxaa; break;
        case kSharpening: s.sharpen = uint32_t(std::clamp(int(s.sharpen) + dir * 10, 0, 100)); break;
        case kFiltering: {
            const uint32_t order[] = {0, 2, 4, 8, 16};
            s.aniso = Next(order, s.aniso, dir);
            break;
        }
        case kSharpText: s.sharp_2d = !s.sharp_2d; break;
        case kFrameCounter: s.show_fps = !s.show_fps; break;
        case kDisplayMode:
            s.display_mode = app::DisplayMode((int(s.display_mode) + 3 + dir) % 3);
            app::Save();
            app::ApplyDisplay();
            return;
        case kVsync: s.vsync = !s.vsync; break;
        case kFrameCap: {
            const uint32_t order[] = {30, 60, 120, 144, 240, 0};
            s.fps_cap = Next(order, s.fps_cap, dir);
            break;
        }
        case kAspect: s.aspect = (s.aspect + 4 + uint32_t(dir)) % 4; break;
        case kRenderer: s.renderer = (s.renderer + 1) % 2; break;
        case kFps60: uncapped = !uncapped; break;
        case kFov: fov_percent = uint32_t(std::clamp(int(fov_percent) + dir * 5, 50, 150)); break;
        case kMouseSpeed: s.mouse_sensitivity = uint32_t(std::clamp(int(s.mouse_sensitivity) + dir * 10, 10, 500)); break;
        case kInvertMouse: s.mouse_invert_y = !s.mouse_invert_y; break;
        case kAutoAim: auto_aim = !auto_aim; break;
        case kLanguage: {
            const uint32_t order[] = {1, 3, 4};
            s.language = Next(order, s.language, dir);
            break;
        }
        case kSkipIntro: skip_intro = !skip_intro; break;
        case kPauseUnfocused: s.pause_unfocused = !s.pause_unfocused; break;
        case kMore: app::ToggleOverlay(); return;
        default: return;
    }
    app::Save();
}
}  // namespace auf
