// Agent Under Fire's achievements. Conditions are in RetroAchievements' syntax, on bond_eng.xbe's memory: a mission's end is
// scored there, driving ones too, after DRIVING.XBE hands back.
//
// The mission played is 0x2A9F98 (0 to 11). A mission's end stores the medal of the run at 0x3B2D08 (0x45322: 1 up, 3 gold, 4
// platinum) and raises the best at 0x2A9C24 + 4 mission (0x45369). Loading a save restores the best but not the run's, and the
// run's stays 0 until a mission is scored, so each achievement asks for the run's: a finished save awards nothing. The other
// exes get the same list with a condition that never holds.
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "common/auf.h"
#include "core/achievements.h"

namespace auf {
namespace {
constexpr uint32_t kMission = 0x2A9F98, kRunMedal = 0x3B2D08, kBestMedals = 0x2A9C24;
constexpr int kMissions = 12;
struct Named {
    const char* id;
    const char* name;
};
constexpr Named kNames[kMissions] = {
    {"trouble_in_paradise", "Trouble in Paradise"}, {"precious_cargo", "Precious Cargo"},
    {"dangerous_pursuit", "Dangerous Pursuit"},     {"bad_diplomacy", "Bad Diplomacy"},
    {"cold_reception", "Cold Reception"},           {"night_of_the_jackal", "Night of the Jackal"},
    {"streets_of_bucharest", "Streets of Bucharest"}, {"fire_and_water", "Fire & Water"},
    {"forbidden_depths", "Forbidden Depths"},       {"poseidon", "Poseidon"},
    {"mediterranean_crisis", "Mediterranean Crisis"}, {"evil_summit", "Evil Summit"},
};
// the Defs hold pointers; the list is made as GameInfo is initialised, so this is made on first use
const char* Keep(std::string s) {
    static std::deque<std::string> kept;
    return kept.emplace_back(std::move(s)).c_str();
}
std::string Hex(uint32_t a) {
    char b[16];
    std::snprintf(b, sizeof b, "%x", a);
    return b;
}
std::string Run() { return "0xX" + Hex(kRunMedal); }
std::string Best(int i) { return "0xX" + Hex(kBestMedals + uint32_t(i) * 4); }
// this frame the run's medal was given: it changed, or the mission's first best rose (the same medal as the last mission's run)
std::string Scored(int i, int medal) {
    return "0xX" + Hex(kMission) + "=" + std::to_string(i) + "_" + Run() + ">=" + std::to_string(medal) + "Sd" + Run() + "!=" + Run() +
           "Sd" + Best(i) + "<" + std::to_string(medal) + "_" + Best(i) + ">=" + std::to_string(medal);
}
std::string AnyMission(int medal) {
    std::string c = Run() + ">=" + std::to_string(medal) + "Sd" + Run() + "<" + std::to_string(medal);
    for (int i = 0; i < kMissions; i++) c += "S0xX" + Hex(kMission) + "=" + std::to_string(i) + "_d" + Best(i) + "<" + std::to_string(medal) + "_" + Best(i) + ">=" + std::to_string(medal);
    return c;
}
std::string Every(int medal) {
    std::string c = Run() + ">=1";
    for (int i = 0; i < kMissions; i++) c += "_" + Best(i) + ">=" + std::to_string(medal);
    c += "Sd" + Run() + "!=" + Run();
    for (int i = 0; i < kMissions; i++) c += "Sd" + Best(i) + "<" + std::to_string(medal);
    return c;
}
}  // namespace

std::vector<achievements::Def> AchievementSet() {
    std::vector<achievements::Def> set;
    const bool here = kImage == kBond;
    auto when = [&](std::string c) { return here ? Keep(std::move(c)) : "0=1"; };
    for (int i = 0; i < kMissions; i++)
        set.push_back({kNames[i].id, kNames[i].name, Keep(std::string("Complete ") + kNames[i].name + "."), 10, when(Scored(i, 1))});
    set.push_back({"gold_medal", "On Her Majesty's Service", "Earn a gold medal on any mission.", 15, when(AnyMission(3))});
    set.push_back({"platinum_medal", "The Name's Bond", "Earn a platinum medal on any mission.", 25, when(AnyMission(4))});
    set.push_back({"all_gold", "Goldfinger", "Hold a gold medal on every mission.", 50, when(Every(3))});
    set.push_back({"all_platinum", "Licence to Kill", "Hold a platinum medal on every mission.", 100, when(Every(4))});
    return set;
}
}  // namespace auf
