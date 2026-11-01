// Keyboard prompts: while the keyboard and mouse play, the button pictures of bond_eng.xbe's menus and hints show the keys.
//
// Its front end and pause draw the prompts from a 256x256 A8R8G8B8 atlas (A, B, X, Y and the d-pads among other art); "Press A"
// hints and the front end's Back and Select from 32x64 DXT3 pictures of an A and a B. When one is bound (GameInfo::texture_bound) and as the disc has it (its bytes' FNV-1a), it
// gets a replacement four times larger with those cells drawn as keys: the first binding of each control, A as Enter and B as
// Backspace where those select and go back (a menu, or Bond not the player's to move), the d-pads as arrow keys. With a controller
// the replacement goes and the disc's pictures are drawn. The guest's bytes are left alone. Shapes: src/common/strokes.h.
// DRIVING.XBE's menus draw theirs from a 256x256 P8 atlas: its guest bytes are rewritten, coded to its own palette, and put
// back for a controller (src/common/buttons.cpp's P8 part). Headless: AUF_BUTTON_DUMP=1 writes each drawing to buttons_<data>_<w>.rgba.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "common/auf.h"
#include "common/strokes.h"
#include "core/input.h"
#include "core/log.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"

namespace auf {
namespace {
constexpr int kSharp = 4;
constexpr uint16_t kVkLeft = 0x25, kVkUp = 0x26, kVkRight = 0x27, kVkDown = 0x28, kEnter = 0x0D, kBackspace = 0x08;
constexpr Colour kWhite{240, 240, 240, 255}, kDark{24, 26, 30, 255}, kEdge{200, 205, 215, 255};

struct Cell {
    Area area;
    int control;  // input::k*, or -1 with arrows
    uint16_t arrow0, arrow1;
};
const std::vector<Cell> kAtlasCells = {
    {{1, 90, 23, 112}, input::kY, 0, 0},       {{24, 90, 47, 111}, input::kA, 0, 0},
    {{64, 93, 80, 108}, -1, kVkRight, 0},      {{87, 93, 103, 108}, -1, kVkUp, 0},
    {{129, 93, 145, 108}, -1, kVkLeft, kVkRight}, {{229, 93, 245, 108}, -1, kVkUp, kVkDown},
    {{195, 118, 211, 133}, -1, kVkUp, kVkDown},
    {{6, 150, 24, 168}, input::kB, 0, 0},      {{27, 150, 45, 168}, input::kY, 0, 0},
    {{48, 150, 66, 168}, input::kA, 0, 0},     {{69, 150, 87, 168}, input::kX, 0, 0},
};
const std::vector<Cell> kPressACells = {{{0, 0, 32, 32}, input::kA, 0, 0}};  // drawn square: the hint's quad is
const std::vector<Cell> kPressBCells = {{{0, 0, 32, 32}, input::kB, 0, 0}};

struct Known {
    uint32_t format, w, h, bytes;
    uint64_t disc;
    const std::vector<Cell>* cells;
    uint32_t canvas;  // texels across and down of the drawing
};
const Known kKnown[] = {{0x06, 256, 256, 256 * 256 * 4, 0xC3A73051EF13D69Full, &kAtlasCells, 256},
                        {0x0E, 32, 64, 32 * 64, 0xA17873E88E43A396ull, &kPressACells, 32},
                        {0x0E, 32, 64, 32 * 64, 0xE8116B93FAE4A5F7ull, &kPressBCells, 32}};

// DRIVING.XBE's atlas in its icons' palette: the tall ovals the pause menu's line draws (squeezed to a 12x20 quad), A and B
// small, the rings, and the d-pads: the round pictures beside them are that line's change page and scroll
const std::vector<Cell> kP8Cells = {
    {{99, 100, 131, 164}, input::kA, 0, 0},    {{99, 34, 131, 98}, input::kB, 0, 0},
    {{132, 34, 164, 98}, input::kX, 0, 0},     {{132, 100, 164, 164}, input::kY, 0, 0},
    {{167, 3, 182, 18}, input::kA, 0, 0},      {{167, 21, 182, 37}, input::kB, 0, 0},
    {{100, 200, 119, 220}, input::kY, 0, 0},   {{125, 200, 145, 220}, input::kA, 0, 0},
    {{152, 215, 170, 234}, input::kX, 0, 0},   {{94, 235, 112, 253}, input::kA, 0, 0},
    {{113, 235, 131, 253}, input::kY, 0, 0},   {{133, 235, 151, 253}, input::kX, 0, 0},
    {{153, 235, 171, 253}, input::kB, 0, 0},   {{202, 150, 218, 166}, -1, kVkLeft, kVkRight},
    {{202, 176, 218, 192}, -1, kVkUp, kVkDown},
    {{168, 117, 184, 132}, -1, kVkUp, kVkDown}, {{168, 135, 184, 150}, -1, kVkLeft, kVkRight},
};
constexpr uint64_t kP8Disc = 0x0D17DFB8FC1761F7ull;
constexpr int kP8GreenX = 104, kP8GreenY = 132;  // on the A oval's rim: green only in the icons' palette

struct Look {
    bool keys = false, select = false;
    std::array<uint16_t, 4> bound{};
    bool operator==(const Look&) const = default;
};
struct Picture {
    const Known* known = nullptr;
    gpu::TextureDesc desc;
    Look drawn;
};
std::mutex g_lock;
std::map<uint32_t, Picture> g_pictures;
std::map<uint32_t, uint64_t> g_not;  // what else was looked at, by its first bytes

uint64_t Fnv(const uint8_t* p, size_t n) {
    uint64_t h = 0xCBF29CE484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

Look Current() {
    Look l;
    static const bool forced = std::getenv("AUF_KEYS_TEST") != nullptr;  // headless: the reference renderer keeps a texture's first look
    l.keys = forced || input::UsingKeyboard();
    l.select = MenuUp() || !PlayerFree();
    const int controls[4] = {input::kA, input::kB, input::kX, input::kY};
    for (int i = 0; i < 4; i++) l.bound[size_t(i)] = input::ControlKeys(controls[i])[0];
    return l;
}

std::string KeyLabel(uint16_t vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, char(vk));
    switch (vk) {
        case 0x01: return "LMB";
        case 0x02: return "RMB";
        case 0x04: return "MMB";
        case 0x08: return "BKSP";
        case 0x09: return "TAB";
        case 0x0D: return "ENTER";
        case 0x1B: return "ESC";
        case 0x20: return "SPACE";
        case 0xA0: case 0xA1: case 0x10: return "SHIFT";
        case 0xA2: case 0xA3: case 0x11: return "CTRL";
        default: return "";
    }
}

void KeyCap(Canvas& c, Area cap, uint16_t vk) {
    float w = cap.x1 - cap.x0, h = cap.y1 - cap.y0, r = std::min(w, h) * 0.2f, edge = std::max(0.5f, std::min(w, h) * 0.06f);
    Fill(c, cap, kEdge, [&](float x, float y) { return BoxDistance(x, y, cap, r, r); });
    Fill(c, cap, kDark, [&](float x, float y) { return BoxDistance(x, y, cap, r, r) + edge; });
    float cx = (cap.x0 + cap.x1) / 2, cy = (cap.y0 + cap.y1) / 2;
    if (vk >= kVkLeft && vk <= kVkDown) {
        float k = std::min(w, h) * 0.25f;
        std::vector<Pt> head;
        if (vk == kVkUp) head = {{cx, cy - k}, {cx + k, cy + k * 0.7f}, {cx - k, cy + k * 0.7f}};
        if (vk == kVkDown) head = {{cx, cy + k}, {cx - k, cy - k * 0.7f}, {cx + k, cy - k * 0.7f}};
        if (vk == kVkLeft) head = {{cx - k, cy}, {cx + k * 0.7f, cy - k}, {cx + k * 0.7f, cy + k}};
        if (vk == kVkRight) head = {{cx + k, cy}, {cx - k * 0.7f, cy + k}, {cx - k * 0.7f, cy - k}};
        Fill(c, cap, kWhite, [&](float x, float y) { return PolygonDistance(x, y, head); });
        return;
    }
    std::string label = KeyLabel(vk);
    if (label.empty()) return;
    float height = std::min(h * 0.5f, (w - 2 * edge - std::max(1.0f, w * 0.12f)) / TextWidth(label.c_str(), 1));
    Text(c, label.c_str(), cx, cy, height, std::max(0.45f, height * 0.2f), kWhite);
}

uint16_t KeyOf(int control, const Look& l) {
    if (control == input::kA) return l.select ? kEnter : l.bound[0];
    if (control == input::kB) return l.select ? kBackspace : l.bound[1];
    return control == input::kX ? l.bound[2] : l.bound[3];
}

void Apply(uint32_t data, Picture& pic, const Look& look) {
    pic.drawn = look;
    if (!look.keys) {
        gpu::ClearReplacement(data);
        LOGR("auf: the button pictures (%08X) as the disc has them", data);
        return;
    }
    const Known& k = *pic.known;
    std::vector<uint32_t> disc;
    const auto* guest = static_cast<const uint8_t*>(kernel::HostPtr(data));
    if (!gpu::DecodeBgra(pic.desc, 0, guest, disc)) return;
    const uint32_t W = k.canvas * kSharp;
    std::vector<uint32_t> out = k.canvas == k.w && k.canvas == k.h ? gpu::UpscalePicture(disc, k.w, k.h, kSharp)
                                                                   : std::vector<uint32_t>(size_t(W) * W, 0);
    Canvas c;
    c.scale = kSharp;
    c.size = int(W);
    c.px.assign(size_t(W) * W, {0, 0, 0, 0});
    for (const Cell& cell : *k.cells) {
        const Area& a = cell.area;
        for (int y = int(a.y0) * kSharp; y < int(a.y1) * kSharp; y++)
            for (int x = int(a.x0) * kSharp; x < int(a.x1) * kSharp; x++) out[size_t(y) * W + x] = 0;
        if (cell.control >= 0) {
            KeyCap(c, a, KeyOf(cell.control, look));
        } else if (!cell.arrow1) {
            KeyCap(c, a, cell.arrow0);
        } else {  // two arrows side by side
            float mid = (a.x0 + a.x1) / 2;
            KeyCap(c, {a.x0, a.y0 + 3, mid, a.y1 - 3}, cell.arrow0);
            KeyCap(c, {mid, a.y0 + 3, a.x1, a.y1 - 3}, cell.arrow1);
        }
    }
    for (uint32_t i = 0; i < W * W; i++) {
        const auto& p = c.px[i];
        if (p[3] <= 0) continue;
        auto ch = [&](int n) { return uint32_t(std::clamp(p[size_t(n)] / p[3], 0.0f, 1.0f) * 255 + 0.5f); };
        out[i] = ch(2) | ch(1) << 8 | ch(0) << 16 | uint32_t(std::clamp(p[3], 0.0f, 1.0f) * 255 + 0.5f) << 24;
    }
    if (std::getenv("AUF_BUTTON_DUMP")) {
        char name[64];
        std::snprintf(name, sizeof name, "buttons_%08X_%u.rgba", data, W);
        if (FILE* f = std::fopen(name, "wb")) {
            std::fwrite(out.data(), 4, out.size(), f);
            std::fclose(f);
        }
    }
    gpu::SetReplacement(data, k.bytes, W, W, std::move(out));
    LOGR("auf: the button pictures (%08X) drawn as keys%s", data, look.select ? ", Enter and Backspace" : "");
}

// the P8 atlas: one at a time, the disc's bytes kept, its palette read when it is first bound. Redrawn only as it is bound:
// the renderers read a texture again when texture_bound says it changed
struct P8Atlas {
    uint32_t data = 0;
    std::vector<uint8_t> disc;
    std::vector<std::array<uint8_t, 4>> palette;  // R G B A
    uint64_t written = 0;
    Look drawn;
};
P8Atlas g_p8;

uint32_t Swizzled(int x, int y) {
    uint32_t r = 0;
    for (int b = 0; b < 8; b++) r |= uint32_t(x >> b & 1) << (2 * b) | uint32_t(y >> b & 1) << (2 * b + 1);
    return r;
}

void ApplyP8(const Look& look) {
    auto* bytes = static_cast<uint8_t*>(kernel::HostPtr(g_p8.data));
    std::vector<uint8_t> out = g_p8.disc;
    if (look.keys) {
        Canvas c;
        c.size = 256;
        c.px.assign(size_t(256) * 256, {0, 0, 0, 0});
        for (const Cell& cell : kP8Cells) {
            const Area& a = cell.area;
            const float w = a.x1 - a.x0, cx = (a.x0 + a.x1) / 2, cy = (a.y0 + a.y1) / 2;
            auto shape = [&](std::vector<Pt> pts) { Fill(c, a, kWhite, [pts](float x, float y) { return PolygonDistance(x, y, pts); }); };
            auto arrow = [&](uint16_t vk) {
                const float k = w * 0.24f, far = w * 0.46f, near = w * 0.08f;
                if (vk == kVkUp) shape({{cx, cy - far}, {cx + k, cy - near}, {cx - k, cy - near}});
                if (vk == kVkDown) shape({{cx, cy + far}, {cx - k, cy + near}, {cx + k, cy + near}});
                if (vk == kVkLeft) shape({{cx - far, cy}, {cx - near, cy - k}, {cx - near, cy + k}});
                if (vk == kVkRight) shape({{cx + far, cy}, {cx + near, cy + k}, {cx + near, cy - k}});
            };
            const uint16_t key = cell.control < 0 ? 0 : KeyOf(cell.control, look);
            if (cell.control < 0) {
                arrow(cell.arrow0);
                arrow(cell.arrow1);
            } else if (key == kEnter) {  // too small for the word: its bent arrow
                shape({{cx + w * 0.26f, cy - w * 0.36f}, {cx + w * 0.42f, cy - w * 0.36f}, {cx + w * 0.42f, cy + w * 0.16f}, {cx + w * 0.26f, cy + w * 0.16f}});
                shape({{cx - w * 0.14f, cy}, {cx + w * 0.42f, cy}, {cx + w * 0.42f, cy + w * 0.16f}, {cx - w * 0.14f, cy + w * 0.16f}});
                shape({{cx - w * 0.44f, cy + w * 0.08f}, {cx - w * 0.1f, cy - w * 0.22f}, {cx - w * 0.1f, cy + w * 0.38f}});
            } else if (key == kBackspace) {
                shape({{cx - w * 0.44f, cy}, {cx - w * 0.06f, cy - w * 0.3f}, {cx - w * 0.06f, cy + w * 0.3f}});
                shape({{cx - w * 0.06f, cy - w * 0.1f}, {cx + w * 0.42f, cy - w * 0.1f}, {cx + w * 0.42f, cy + w * 0.1f}, {cx - w * 0.06f, cy + w * 0.1f}});
            } else {
                KeyCap(c, {a.x0, cy - w * 0.6f, a.x1, cy + w * 0.6f}, key);
            }
        }
        int clear = 0;
        for (size_t i = 0; i < g_p8.palette.size(); i++)
            if (g_p8.palette[i][3] == 0) {
                clear = int(i);
                break;
            }
        for (const Cell& cell : kP8Cells)
            for (int y = int(cell.area.y0); y < int(cell.area.y1); y++)
                for (int x = int(cell.area.x0); x < int(cell.area.x1); x++) {
                    const auto& p = c.px[size_t(y) * 256 + x];
                    int best = clear;
                    if (p[3] > 0.03f) {
                        float px[4] = {p[0] / p[3] * 255, p[1] / p[3] * 255, p[2] / p[3] * 255, p[3] * 255}, lowest = 1e12f;
                        for (size_t k = 0; k < g_p8.palette.size(); k++) {
                            float dist = 0;
                            for (int n = 0; n < 4; n++) dist += (px[n] - g_p8.palette[k][size_t(n)]) * (px[n] - g_p8.palette[k][size_t(n)]);
                            if (dist < lowest) lowest = dist, best = int(k);
                        }
                    }
                    out[Swizzled(x, y)] = uint8_t(best);
                }
    }
    std::memcpy(bytes, out.data(), out.size());
    g_p8.written = Fnv(bytes, out.size());
    g_p8.drawn = look;
    LOGR("auf: the driving menus' atlas (%08X) %s", g_p8.data, look.keys ? "drawn as keys" : "as the disc has it");
}

bool ButtonsP8(const gpu::TextureDesc& d, uint32_t palette) {
    constexpr uint32_t kSize = 256 * 256;
    if (!mem::IsCommitted(d.data) || !mem::IsCommitted(d.data + kSize - 1)) return false;
    const auto* bytes = static_cast<const uint8_t*>(kernel::HostPtr(d.data));
    std::lock_guard<std::mutex> hold(g_lock);
    if (g_p8.data != d.data || Fnv(bytes, kSize) != g_p8.written) {
        // the whole hash once per address and contents: fonts share this shape, bound many times a frame
        uint64_t head = Fnv(bytes, 256);
        auto no = g_not.find(d.data);
        if (no != g_not.end() && no->second == head) return false;
        if (Fnv(bytes, kSize) != kP8Disc) {
            g_not[d.data] = head;
            return false;
        }
        g_p8 = P8Atlas{};
        g_p8.data = d.data;
        g_p8.disc.assign(bytes, bytes + kSize);
        g_p8.written = kP8Disc;
    }
    if (g_p8.palette.empty()) {
        if (!palette) return false;
        const uint32_t colours = 256u >> (kernel::R32(palette) >> 30), pal = mem::VirtualFromPhysical(kernel::R32(palette + 4));
        if (!mem::IsCommitted(pal) || !mem::IsCommitted(pal + colours * 4 - 1)) return false;
        const auto* bgra = static_cast<const uint8_t*>(kernel::HostPtr(pal));
        // bound with several palettes: the icons' is the one that shows the A oval's rim green
        const uint8_t rim = g_p8.disc[Swizzled(kP8GreenX, kP8GreenY)];
        if (rim >= colours) return false;
        const uint8_t* e = bgra + rim * 4;
        if (!(e[1] > e[2] + 24 && e[1] > e[0] + 24)) return false;
        for (uint32_t i = 0; i < colours; i++) g_p8.palette.push_back({bgra[i * 4 + 2], bgra[i * 4 + 1], bgra[i * 4], bgra[i * 4 + 3]});
        LOGR("auf: the driving menus' atlas's palette: %u colours at %08X", colours, pal);
    }
    Look look = Current();
    if (look == g_p8.drawn) return false;
    ApplyP8(look);
    return true;
}
}  // namespace

// GameInfo::texture_bound
bool ButtonPictures(uint32_t header, uint32_t palette) {
    gpu::TextureDesc d;
    if (!gpu::Describe(header, d)) return false;
    if (kImage == kDriving) return d.format == 0x0B && d.width == 256 && d.height == 256 && ButtonsP8(d, palette);
    const Known* k = nullptr;  // the first of this shape: enough to look at its bytes
    for (const Known& c : kKnown)
        if (!k && c.format == d.format && c.w == d.width && c.h == d.height) k = &c;
    if (!k || !mem::IsCommitted(d.data) || !mem::IsCommitted(d.data + k->bytes - 1)) return false;
    std::lock_guard<std::mutex> hold(g_lock);
    const auto* bytes = static_cast<const uint8_t*>(kernel::HostPtr(d.data));
    auto it = g_pictures.find(d.data);
    if (it == g_pictures.end()) {
        // the whole hash once per address and contents: many textures share this shape, bound many times a frame
        uint64_t head = Fnv(bytes, 256);
        auto no = g_not.find(d.data);
        if (no != g_not.end() && no->second == head) return false;
        const uint64_t hash = Fnv(bytes, k->bytes);
        const Known* same = nullptr;
        for (const Known& c : kKnown)
            if (c.format == d.format && c.w == d.width && c.h == d.height && c.disc == hash) same = &c;
        if (!same) {
            g_not[d.data] = head;
            return false;
        }
        it = g_pictures.emplace(d.data, Picture{same, d, {}}).first;
    } else if (it->second.known->format != k->format || it->second.known->w != k->w || it->second.known->h != k->h) {  // the memory holds something else now
        g_pictures.erase(it);
        return false;
    }
    Look look = Current();
    if (look == it->second.drawn) return false;
    Apply(d.data, it->second, look);
    return true;
}

// each poll of controller 1: drawn again when what they should show changes
void RefreshButtons() {
    std::lock_guard<std::mutex> hold(g_lock);
    if (g_pictures.empty()) return;
    Look look = Current();
    for (auto& [data, pic] : g_pictures)
        if (!(look == pic.drawn) && mem::IsCommitted(data) && mem::IsCommitted(data + pic.known->bytes - 1) &&
            Fnv(static_cast<const uint8_t*>(kernel::HostPtr(data)), pic.known->bytes) == pic.known->disc)
            Apply(data, pic, look);
}
}  // namespace auf
