#include "scconsole.h"

#include "offsets.h"
#include "draw.h"

#include "constants/weapon.h"
#include "ai.h"
#include "limits.h"
#include "pathing.h"
#include "player.h"
#include "resolution.h"
#include "selection.h"
#include "sprite.h"
#include "strings.h"
#include "tech.h"
#include "unit.h"
#include "upgrade.h"
#include "yms.h"
#include "order.h"
#include "constants/order.h"


#include <string>
#include <algorithm>
#include <unordered_set>

using namespace Common;
using std::get;


#pragma pack(push, 1)
struct Location
{
    Rect32 area;
    uint16_t unk;
    uint16_t flags;
};
#pragma pack(pop)

const char* OrderIdToName[] = {
    "Die", "Stop", "Guard", "PlayerGuard", "TurretGuard", "BunkerGuard", "Move", "ReaverStop", "Attack1", "Attack2",
    "AttackUnit", "AttackFixedRange", "AttackTile", "Hover", "AttackMove", "InfestedCommandCenter", "UnusedNothing", "UnusedPowerup",
    "TowerGuard", "TowerAttack", "VultureMine", "StayInRange", "TurretAttack", "Nothing", "Unused_24", "DroneStartBuild", "DroneBuild", "CastInfestation",
    "MoveToInfest", "InfestingCommandCenter", "PlaceBuilding", "PlaceProtossBuilding", "CreateProtossBuilding", "ConstructingBuilding",
    "Repair", "MoveToRepair", "PlaceAddon", "BuildAddon", "Train", "RallyPointUnit", "RallyPointTile",
    "ZergBirth", "ZergUnitMorph", "ZergBuildingMorph", "IncompleteBuilding", "IncompleteMorphing", "BuildNydusExit",
    "EnterNydusCanal", "IncompleteWarping", "Follow", "Carrier", "ReaverCarrierMove", "CarrierStop", "CarrierAttack", "CarrierMoveToAttack", "CarrierIgnore2",
    "CarrierFight", "CarrierHoldPosition", "Reaver", "ReaverAttack", "ReaverMoveToAttack", "ReaverFight", "ReaverHoldPosition", "TrainFighter", "InterceptorAttack", "ScarabAttack",
    "RechargeShieldsUnit", "RechargeShieldsBattery", "ShieldBattery", "InterceptorReturn", "DroneLand",
    "BuildingLand", "BuildingLiftOff", "DroneLiftOff", "LiftingOff", "ResearchTech", "Upgrade",
    "Larva", "SpawningLarva", "Harvest1", "Harvest2", "MoveToGas", "WaitForGas", "HarvestGas",
    "ReturnGas", "MoveToMinerals", "WaitForMinerals", "MiningMinerals", "Harvest3", "Harvest4",
    "ReturnMinerals", "Interrupted", "EnterTransport", "PickupIdle", "PickupTransport", "PickupBunker",
    "Pickup4", "PowerupIdle", "Sieging", "Unsieging", "WatchTarget", "InitCreepGrowth", "SpreadCreep",
    "StoppingCreepGrowth", "GuardianAspect", "ArchonWarp", "CompletingArchonSummon", "HoldPosition",
    "QueenHoldPosition", "Cloak", "Decloak", "Unload", "MoveUnload", "FireYamatoGun", "MoveToFireYamatoGun", "CastLockdown", "Burrowing",
    "Burrowed", "Unburrowing", "CastDarkSwarm", "CastParasite", "CastSpawnBroodlings", "CastEMPShockwave",
    "NukeWait", "NukeTrain", "NukeLaunch", "NukePaint", "NukeUnit", "CastNuclearStrike", "NukeTrack", "InitializeArbiter",
    "CloakNearbyUnits", "PlaceMine", "RightClickAction", "SuicideUnit", "SuicideLocation", "SuicideHoldPosition", "CastRecall", "Teleport",
    "CastScannerSweep", "Scanner", "CastDefensiveMatrix", "CastPsionicStorm", "CastIrradiate",
    "CastPlague", "CastConsume", "CastEnsnare", "CastStasisField", "CastHallucination", "Hallucination2",
    "ResetCollision", "ResetHarvestCollision", "Patrol", "CTFCOPInit", "CTFCOPStarted", "CTFCOP2", "ComputerAI", "AtkMoveEP",
    "HarassMove", "AIPatrol", "GuardPost", "RescuePassive", "Neutral", "ComputerReturn", "InitializePsiProvider",
    "SelfDestructing", "Critter", "HiddenGun", "OpenDoor", "CloseDoor", "HideTrap", "RevealTrap",
    "EnableDoodad", "DisableDoodad", "WarpIn", "Medic", "MedicHeal", "HealMove", "MedicHoldPosition", "MedicHealToIdle",
    "CastRestoration", "CastDisruptionWeb", "CastMindControl", "DarkArchonMeld", "CastFeedback",
    "CastOpticalFlare", "CastMaelstrom", "JunkYardDog", "Fatal", "None", "Unknown" };

struct TextLayout
{
    TextLayout(Common::Surface* surface) : surface(surface) {
    }

    void Draw(Font* font, const std::string& str, const Point32& default_pos, uint8_t color) {
        auto width = font->TextLength(str);
        auto height = 10;
        Point32 pos;
        for (int i = 0; i < 60; i++) {
            auto pos = Point32(default_pos.x, default_pos.y + i * 10);
            Rect32 suggest(pos.x, pos.y, pos.x + width, pos.y + height);
            if (TryDrawAt(font, str, suggest, color)) {
                return;
            }
            pos = Point32(default_pos.x, default_pos.y - i * 10);
            suggest = Rect32(pos.x, pos.y, pos.x + width, pos.y + height);
            if (TryDrawAt(font, str, suggest, color)) {
                return;
            }
        }
    }

    bool TryDrawAt(Font* font, const std::string& str, const Rect32& suggest, uint8_t color) {
        if (suggest.top < 0 || suggest.bottom >= surface->height) {
            return false;
        }
        auto collide = std::any_of(blocks.begin(), blocks.end(), [&](const auto& block) {
            return block.left < suggest.right&&
                block.right > suggest.left &&
                block.top < suggest.bottom&&
                block.bottom > suggest.top;
            });
        if (!collide) {
            surface->DrawText(font, str, Point32(suggest.left, suggest.top), color);
            blocks.push_back(suggest);
            return true;
        }
        else {
            return false;
        }
    }

    Common::Surface* surface;
    vector<Rect32> blocks;
};

// In ScConsole constructor, make sure cursor_pos is initialized:
ScConsole::ScConsole()
{
    show_fps = true;
    show_frame = false;
    draw_info = false;
    draw_locations = false;
    draw_crects = false;
    draw_ai_towns = false;
    draw_orders = OrderDrawMode::All;
    draw_ai_data = true;
    draw_ai_full = false;
    draw_ai_named = true;
    draw_ai_unit_homes = false;
    draw_ai_guards = false;
    draw_ai_regions = false;
    for (int i = 0; i < Limits::Players; i++)
        show_ai[i] = 1;
    draw_coords = false;
    draw_range = false;
    draw_resource_areas = false;
    cursor_pos = 0;  // Initialize cursor position here

    AddCommand("heal", &ScConsole::Heal);
    AddCommand("kill", &ScConsole::Kill);
    AddCommand("give", &ScConsole::Give);
    AddCommand("gsw", &ScConsole::Gsw);
    AddCommand("vis", &ScConsole::Vision);
    AddCommand("vision", &ScConsole::Vision);
    AddCommand("ally", &ScConsole::Alliance);
    AddCommand("alliance", &ScConsole::Alliance);
    AddCommand("supplymax", &ScConsole::SupplyMax);
    AddCommand("airegion", &ScConsole::AiRegion);
    AddCommand("aireg", &ScConsole::AiRegion);
    AddCommand("player", &ScConsole::Player);
    AddCommand("u", &ScConsole::UnitCmd);
    AddCommand("unit", &ScConsole::UnitCmd);
    AddCommand("money", &ScConsole::Money);
    AddCommand("resources", &ScConsole::Money);
    AddCommand("supply", &ScConsole::Supply);
    AddCommand("self", &ScConsole::Self);
    AddCommand("frame", &ScConsole::Frame);
    AddCommand("pause", &ScConsole::Pause);
    AddCommand("show", &ScConsole::Show);
    AddCommand("sai", &ScConsole::ShowAi);
    AddCommand("saip", &ScConsole::ShowAiPlayer);
    AddCommand("saig", &ScConsole::ShowAiGuards);
    AddCommand("saiu", &ScConsole::ShowAiUnits);
    AddCommand("grid", &ScConsole::Cmd_Grid);
    AddCommand("spawn", &ScConsole::Spawn);
    AddCommand("unitcount", &ScConsole::UnitCount);
    AddCommand("ff", &ScConsole::FastForward);
    commands["dc?"] = [this](const auto& a) { return this->Death(a, true, false); };
}

ScConsole::~ScConsole()
{
}

void ScConsole::Hide()
{
    Console::Hide();
    *bw::needs_full_redraw = true;
}

bool ScConsole::Cmd_Grid(const CmdArgs& args)
{
    int width = atoi(args[2]);
    int height = atoi(args[3]);
    if (width == 0)
        return false;
    if (height == 0)
        height = width;
    int color = atoi(args[4]);
    if (color == 0)
        color = 0x98;

    Grid grid(width, height, color);
    if (strcmp(args[1], "-") == 0)
    {
        if (args[2][0] == 0)
        {
            grids.clear();
            return true;
        }
        for (int i = 0; i < grids.size(); i++)
        {
            if (grids[i].width == width && grids[i].height == height)
            {
                grids.erase(grids.begin() + i);
                return true;
            }
        }
        return false;
    }
    if (strcmp(args[1], "+") == 0)
    {
        for (int i = 0; i < grids.size(); i++)
        {
            if (grids[i].width == width && grids[i].height == height)
                return false;
        }
        grids.emplace_back(grid);
        return true;
    }
    else
    {
        Printf("grid (+|-) w [h] [color]");
        return false;
    }
}

static vector<UnitType> FindUnitFromName(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), tolower);
    vector<UnitType> results;
    for (auto unit_id : UnitType::All())
    {
        std::string unit_name = (*bw::stat_txt_tbl)->GetTblString(unit_id.Raw() + 1);
        std::transform(unit_name.begin(), unit_name.end(), unit_name.begin(), tolower);
        if (unit_name.find(name) != std::string::npos)
            results.emplace_back(unit_id);
    }
    return results;
}

vector<UnitType> ScConsole::ParseUnitId(const char* unit_str, int max_amt)
{
    vector<UnitType> unit_ids;
    char* unit_str_end;
    int unit_id = strtoul(unit_str, &unit_str_end, 16);
    if (unit_str_end[0] != 0 || (unit_id == 0 && unit_str[0] != '0'))
    {
        unit_ids = FindUnitFromName(unit_str);
        if (unit_ids.size() > 30)
        {
            Printf("%d units match '%s'", unit_ids.size(), unit_str);
            return vector<UnitType>();
        }
        else if (unit_ids.size() > max_amt)
        {
            char buf[128];
            snprintf(buf, sizeof buf, "Too many candidates for '%s':", unit_str);
            std::string msg(buf);
            bool first = true;
            for (auto cand : unit_ids)
            {
                snprintf(buf, sizeof buf, " %s (%x)",
                    (*bw::stat_txt_tbl)->GetTblString(cand.Raw() + 1),
                    cand.Raw());
                if (!first)
                    msg.push_back(',');
                msg += buf;
                first = false;
            }
            Printf(msg.c_str());
            return vector<UnitType>();
        }
    }
    else
    {
        unit_ids.emplace_back(UnitType(unit_id));
    }
    if (unit_ids.empty())
        Printf("'%s' is not valid unit id or unit name", unit_str);
    return unit_ids;
}

bool ScConsole::Heal(const CmdArgs& args)
{
    if (!IsInGame())
        return false;

    for (Unit* unit : client_select)
    {
        if (unit->hitpoints)
        {
            unit->hitpoints = unit->GetMaxHitPoints() << 8;
            unit->shields = unit->GetMaxShields() << 8;
        }
    }
    return true;
}

bool ScConsole::Kill(const CmdArgs& args)
{
    if (!IsInGame())
        return false;

    for (Unit* unit : client_select)
    {
        unit->order = 0;
        //unit->Kill(nullptr);
    }
    return true;
}

bool ScConsole::Give(const CmdArgs& args)
{
    if (!IsInGame())
        return false;

    int player;
    if (!isdigit(*args[1]))
        player = *bw::local_player_id;
    else
        player = atoi(args[1]);

    for (Unit* unit : client_select)
    {
        bw::GiveUnit(unit, player, false);
    }
    return true;
}

bool ScConsole::Death(const CmdArgs& args, bool print, bool clear)
{
    if (!IsInGame())
        return false;
    auto unit_str = args[1];
    if (unit_str[0] == 0)
    {
        Printf("dc? <unit name or hex id> [player id, player id, ...]");
        return false;
    }
    vector<UnitType> unit_ids = ParseUnitId(unit_str, 1);

    if (unit_ids.empty())
    {
        return false;
    }
    auto unit_deaths = *bw::unit_deaths;

    for (auto unit_id : unit_ids)
    {
        auto end = std::remove_if(death_counters.begin(), death_counters.end(), [unit_id](const auto& tp) {
            return get<1>(tp) == unit_id;
            });
        death_counters.erase(end, death_counters.end());
        if (!clear)
        {
            uint16_t player_mask = 0;
            int pos = 2;
            while (args[pos][0] != 0)
            {
                player_mask |= 1 << atoi(args[pos]);
                pos++;
            }
            if (player_mask == 0)
                player_mask = 0xff;


            if (print)
            {
                std::string msg;
                for (int i = 0; i < Limits::Players; i++)
                {
                    if (player_mask & 1 << i)
                    {
                        char buf[16];
                        Assert(unit_id.IsValid());
                        if (i >= Limits::Players)
                            continue;
                        snprintf(buf, sizeof buf, "%d ", unit_deaths[unit_id.Raw() * Limits::Players + i]);
                        msg += buf;
                    }
                }
                Printf(msg.c_str());
            }
            else
                death_counters.emplace_back(player_mask, unit_id);
        }
    }
    return true;
}

bool ScConsole::Spawn(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (!IsInGame())
        return false;
    auto unit_str = args[1];
    if (unit_str[0] == 0)
    {
        Printf("spawn <unit name or hex id> [amount] [player id]");
        return false;
    }
    vector<UnitType> unit_ids = ParseUnitId(unit_str, 1);

    if (unit_ids.empty())
    {
        return false;
    }
    int amount = 1;
    if (args[2][0] != 0)
        amount = atoi(args[2]);
    int player = *bw::local_player_id;
    if (args[3][0] != 0)
        player = atoi(args[3]);

    Point16 pos = Point16(*bw::screen_x + *bw::mouse_clickpos_x, *bw::screen_y + *bw::mouse_clickpos_y);
    for (int i = 0; i < amount; i++)
    {
        Unit* unit = bw::CreateUnit(unit_ids[0], pos.x, pos.y, player);
        if (unit == nullptr)
            return false;
        bw::FinishUnit_Pre(unit);
        bw::FinishUnit(unit);
        bw::GiveAi(unit);
    }
    return true;
}

Unit* ScConsole::GetUnit()
{
    if (!IsInGame())
        return 0;
    return *bw::primary_selected;
}

array_offset <Unit*, 12> ScConsole::GetSelectedUnits()
{
    if (!IsInGame())
        return 0;
    return bw::client_selection_group;
}

bool ScConsole::SupplyMax(const CmdArgs& args)
{
    if (!isdigit(*args[1]))
        return false;

    int max = atoi(args[1]);
    for (unsigned int i = 0; i < Limits::Players; i++)
    {
        bw::zerg_supply_max[i] = max;
        bw::protoss_supply_max[i] = max;
        bw::terran_supply_max[i] = max;
    }
    return true;
}

// Single Player only!
int ScConsole::GetConsolePlayer() {
    for (int i = 0; i < Limits::ActivePlayers; i++) {
        if (IsHumanPlayer(i)) {
            return i;
        }
    }
    return -1; // If this happens, something went wrong!
}

bool ScConsole::CanUseSingleplayerCommand() {
    return true;

    if (!IsInGame()) return true;
    int humanPlayerCount = 0;
    for (int i = 0; i < Limits::ActivePlayers; i++) {
        if (IsHumanPlayer(i)) {
            humanPlayerCount++;
        }
    }
    return humanPlayerCount <= 1;
}

void ScConsole::PrintSingleplayerOnlyError() {
    Print("This command is reserved for single player usage only!");
}

extern bool all_visions;
bool ScConsole::Vision(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (args[0] == "" || args[1] == "") {
        Print("Usage: vis <player|all> [on|off]");
        return false;
    }

    int consolePlayer = GetConsolePlayer();
    bool enabled = true; // defaults to on
    if (args[2] != "")
        enabled = stricmp(args[2], "on") == 0; // ignore case
    std::vector<int> players;
    if (stricmp(args[1], "all") == 0)
    {
        for (int player = 0; player < Limits::ActivePlayers; player++) {
            if (!IsComputerPlayer(player))
                continue;

            players.push_back(player);
        }
    }
    else if (isdigit(*args[1]))
    {
        players.push_back(atoi(args[1]));
    }

    for (const int& player : players)
    {
        if (enabled)
            bw::visions[player] |= 1 << consolePlayer;
        else
            bw::visions[player] &= ~(1 << consolePlayer);

        Printf("Vision sharing %s for player %d", enabled ? "enabled" : "disabled", player);
    }
    return true;
}

bool ScConsole::Alliance(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (args[0] == "" || args[1] == "") {
        Print("Usage: alliance <player|all> [on|off]");
        return false;
    }

    int consolePlayer = GetConsolePlayer();
    bool enabled = true; // defaults to on
    if (args[2] != "")
        enabled = stricmp(args[2], "on") == 0; // ignore case
    std::vector<int> players;
    if (stricmp(args[1], "all") == 0)
    {
        for (int player = 0; player < Limits::ActivePlayers; player++) {
            if (!IsComputerPlayer(player))
                continue;

            players.push_back(player);
        }
    }
    else if (isdigit(*args[1]))
    {
        players.push_back(atoi(args[1]));
    }

    for (const int& player : players)
    {
        if (enabled) {
            bw::alliances[player][consolePlayer] = true;
            bw::alliances[consolePlayer][player] = true;
        }
        else {
            bw::alliances[player][consolePlayer] = false;
            bw::alliances[consolePlayer][player] = false;
        }

        Printf("Alliance %s for player %d", enabled ? "enabled" : "disabled", player);
    }
    return true;
}

bool ScConsole::Gsw(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (!IsInGame() || !isdigit(*args[1]))
        return false;

    bw::game_speed_waits[*bw::game_speed] = atoi(args[1]);
    return true;
}

bool ScConsole::AiRegion(const CmdArgs& args)
{
    if (!IsInGame() || !isdigit(*args[1]) || !isxdigit(*args[2]))
        return false;

    unsigned int player = atoi(args[1]), region_id = strtoul(args[2], 0, 16);
    if (player >= Limits::ActivePlayers || region_id > (*bw::pathing)->region_count)
        return false;

    Ai::Region* region = bw::player_ai_regions[player] + region_id;
    char buf[64];
    sprintf(buf, "State %d, flags %02x", region->state, region->flags);
    Print(buf);
    return true;
}


bool ScConsole::Player(const CmdArgs& args)
{
    Unit* unit = GetUnit();
    if (!unit)
        return false;

    Printf("%d", unit->player);
    return true;
}

const char* flag_desc[] =
{
    "Completed", "Building", "Air", "Disabled?", "Burrowed", "In building", "In transport", "Unk", "Invisibility done", "Begin invisibility", "Disabled",
    "Free invisibility", "Uninterruptable order", "Nobrkcodestart", "Has disappearing creep", "Under disruption web", "Auto attack?", "Reacts",
    "Ignore collision?", "Move target moved?", "Collides?", "No collision", "Enemy collision?", "Harvesting", "Unk", "Unk", "Invincible",
    "Hold position", "Movement speed upgrade", "Attack speed upgrade", "Hallucination", "Self destructing"
};

bool ScConsole::UnitCmd(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    array_offset <Unit*, 12> selectedUnits = GetSelectedUnits();
    if (selectedUnits.size() == 0) {
        Print("No units selected");
        return false;
    }

    std::string cmd = args[1];
    std::string val = args[2];
    for (int i = 0; i < selectedUnits.size(); i++)
    {
        Unit* unit = selectedUnits[i];
        if (!unit) {
            continue;
        }
        if (cmd == "ai")
        {
            if (!unit->ai)
                Print("Unit has no AI");
            else
                Printf("%d", unit->ai->type);
        }
        else if (cmd == "hp" || cmd == "h")
        {
            if (isdigit(*args[2]))
            {
                unit->hitpoints = int32_t(std::min(unit->GetMaxHitPoints() * 256, atoi(args[2]) * 256));
            }
            Printf("Unit HP: %d", unit->hitpoints / 256);
        }
        else if (cmd == "energy" || cmd == "e")
        {
            if (isdigit(*args[2]))
            {
                unit->energy = int32_t(std::min(unit->GetMaxEnergy() * 256, atoi(args[2]) * 256));
            }
            Printf("Unit Energy: %d", unit->energy / 256);
        }
        else if (cmd == "shields" || cmd == "s")
        {
            if (isdigit(*args[2]))
            {
                unit->shields = int32_t(std::min(unit->GetMaxShields() * 256, atoi(args[2]) * 256));
            }
            Printf("Unit Shields: %d", unit->shields / 256);
        }
    }

    if (cmd != "ai"
        && cmd != "hp" && cmd != "h"
        && cmd != "energy" && cmd != "e"
        && cmd != "shields" && cmd != "s")
    {
        Printf("unit ai|hp|shields|energy");
    }

    return true;
}

bool ScConsole::Money(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (!IsInGame())
        return false;

    if (strcmp(args[1], "set") == 0)
    {
        if (!isdigit(*args[2]) || !isdigit(*args[3]) || !isdigit(*args[4]))
            return false;
        int player = atoi(args[2]), minerals = atoi(args[3]), gas = atoi(args[4]);
        if (!IsActivePlayer(player))
            return false;
        bw::minerals[player] = minerals;
        bw::gas[player] = gas;
    }
    else
    {
        int player;
        if (isdigit(*args[1]))
        {
            player = atoi(args[1]);
        }
        else
        {
            if (!GetUnit())
                return false;
            player = GetUnit()->player;
        }
        if (!IsActivePlayer(player))
            return false;

        Printf("Minerals: %d, Gas %d", bw::minerals[player], bw::gas[player]);
    }
    return true;
}

#include "offsets.h"
bool ScConsole::UnitCount(const CmdArgs& args) {
    if (!IsInGame()) {
        return false;
    }
    char buf[64];
    int count = *((uint32_t*)0x006283F0);
    sprintf(buf, "Total Unit Count: %d", count);
    Printf(buf);
    return true;
}

bool ScConsole::FastForward(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (isFastForwarding)
        return false;
    if (!isdigit(*args[1])) {
        Print("Usage: ff <seconds>");
        return false;
    }
    int frames = atoi(args[1]) * 24;
    fastForwardStartFrames = *bw::frame_count;
    fastForwardEndFrames = fastForwardStartFrames + frames;
    return true;
}

static void PrintSupply(char* buf, int used, int available)
{
    sprintf(buf + strlen(buf), " %d", used / 2);
    if (used & 1)
        strcat(buf, ".5");
    sprintf(buf + strlen(buf), "/%d", available / 2);
    if (used & 1)
        strcat(buf, ".5");
}

bool ScConsole::Supply(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (!IsInGame())
        return false;

    int player;
    if (isdigit(*args[1]))
    {
        player = atoi(args[1]);
    }
    else
    {
        if (!GetUnit())
            return false;
        player = GetUnit()->player;
    }
    if (!IsActivePlayer(player))
        return false;

    char buf[64] = "Zerg";
    PrintSupply(buf, bw::zerg_supply_used[player], bw::zerg_supply_available[player]);
    strcat(buf, ", Terran");
    PrintSupply(buf, bw::terran_supply_used[player], bw::terran_supply_available[player]);
    strcat(buf, ", Protoss");
    PrintSupply(buf, bw::protoss_supply_used[player], bw::protoss_supply_available[player]);
    Printf(buf);
    return true;
}

bool ScConsole::Self(const CmdArgs& args)
{
    if (*bw::local_player_id == *bw::local_unique_player_id)
        Printf("Game %d, Net %d", *bw::local_player_id, *bw::self_net_player);
    else
        Printf("Shared %d, Unique %d, Net %d", *bw::local_player_id, *bw::local_unique_player_id, *bw::self_net_player);

    return true;
}

bool ScConsole::Frame(const CmdArgs& args)
{
    if (!IsInGame())
        return false;

    Printf("Frame %d", *bw::frame_count);
    return true;
}

bool ScConsole::Pause(const CmdArgs& args)
{
    if (!CanUseSingleplayerCommand()) {
        PrintSingleplayerOnlyError();
        return false;
    }

    if (!IsInGame())
        return false;

    *bw::is_paused ^= 1;
    return true;
}

void ScConsole::ConstructInfoLines()
{
    info_lines.clear();
    if (show_frame)
    {
        char str[32];
        sprintf(str, "Frame: %d", *bw::frame_count);
        info_lines.emplace_back(str);
    }
}

void ScConsole::DrawLocations(uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_locations)
        return;

    Common::Surface surface(framebuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (Location& location : bw::locations)
    {
        Rect32& area = location.area;
        surface.DrawLine(Point32(area.left, area.top) - screen_pos, Point32(area.right, area.top) - screen_pos, 0x7c,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(area.left, area.top) - screen_pos, Point32(area.left, area.bottom) - screen_pos, 0x7c,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(area.right, area.top) - screen_pos, Point32(area.right, area.bottom) - screen_pos, 0x7c,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(area.left, area.bottom) - screen_pos, Point32(area.right, area.bottom) - screen_pos, 0x7c,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
    }
}

void ScConsole::DrawCrects(uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_crects)
        return;

    Common::Surface surface(framebuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (Unit* unit : *bw::first_active_unit)
    {
        Rect16 crect = unit->GetCollisionRect();
        surface.DrawLine(Point32(crect.left, crect.top) - screen_pos, Point32(crect.right, crect.top) - screen_pos, 0x74,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(crect.left, crect.top) - screen_pos, Point32(crect.left, crect.bottom) - screen_pos, 0x74,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(crect.right, crect.top) - screen_pos, Point32(crect.right, crect.bottom) - screen_pos, 0x74,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        surface.DrawLine(Point32(crect.left, crect.bottom) - screen_pos, Point32(crect.right, crect.bottom) - screen_pos, 0x74,
            [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
    }
}

static const char* RequestStr(int req)
{
    switch (req)
    {
    case 1:
        return "Train";
    case 2:
        return "Guard";
    case 3:
        return "Build";
    case 4:
        return "Worker";
    case 5:
        return "Upgrade";
    case 6:
        return "Tech";
    case 7:
        return "Addon";
    case 8:
        return "Observer";
    default:
        return "Error";
    }
}

static int CountScripts(int player)
{
    int count = 0;
    for (Ai::Script* script : *bw::first_active_ai_script)
    {
        if (script->player == player)
            count++;
    }
    return count;
}

void ScConsole::GetTownRequests(uint32_t* out, int len, uint32_t* in)
{
    int pos = 0;
    while (pos != len && in[pos] != 0)
    {
        auto req = in[pos++];
        auto unit_id = req >> 16;
        auto amt = (req & 0xf8) >> 3;
        bool skip = false;
        for (int i = 0; i < len && in[i] != 0; i++)
        {
            auto other_amt = (in[i] & 0xf8) >> 3;
            if (in[i] >> 16 == unit_id && amt < other_amt && (in[i] & 0x6) == (req & 0x6))
            {
                if (~in[i] & 0x1 || req & 0x1)
                {
                    skip = true;
                    break;
                }
            }
        }
        if (!skip || draw_ai_full)
            *out++ = req;
    }
    *out = 0;
}

void ScConsole::DrawAiRegions(int player, Common::Surface* text_surf, const Point32& pos)
{
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (int i = 0; i < (*bw::pathing)->region_count; i++)
    {
        Pathing::Region* p_region = (*bw::pathing)->regions + i;
        Point32 draw_pos = Point32(p_region->x / 0x100, p_region->y / 0x100) - screen_pos + pos;
        if (draw_pos.x < 0 || draw_pos.y < 0)
            continue;
        if (draw_pos.x >= resolution::game_width || draw_pos.y >= resolution::game_height)
            continue;
        Ai::Region* region = Ai::GetRegion(player, i);
        char buf[128];
        snprintf(buf, sizeof buf, "State %x target %x", region->state, region->target_region_id);
        text_surf->DrawText(&font, buf, draw_pos, 0x55);
        draw_pos += Point32(0, 10);
        snprintf(buf, sizeof buf, "Defense priority %d", region->defense_priority);
        text_surf->DrawText(&font, buf, draw_pos, 0x55);
        draw_pos += Point32(0, 10);
        snprintf(buf, sizeof buf, "Need %d/%d, Current %d/%d",
            region->needed_ground_strength, region->needed_air_strength,
            region->local_ground_strength, region->local_air_strength);
        text_surf->DrawText(&font, buf, draw_pos, 0x55);
        draw_pos += Point32(0, 10);
        snprintf(buf, sizeof buf, "All %d/%d, Enemy %d/%d",
            region->all_ground_strength, region->all_air_strength,
            region->enemy_ground_strength, region->enemy_air_strength);
        text_surf->DrawText(&font, buf, draw_pos, 0x55);
    }
}

#include "Iquare_ShortUnitNames.h"
//IQUARE
void ScConsole::DrawDeaths(uint8_t* textbuf, uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_deaths)
        return;
    Common::Surface surface(framebuf, w, h);
    Common::Surface text_surface(textbuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    Point32 info_pos(10, 10);
    for (unsigned int i = 0; i < Limits::ActivePlayers; i++)
    {
        if (show_deaths[i] == 1)
        {
            int row = 0;
            int column = 0;
            for (int j = 0; j < 228; j++)
            {
                uint32_t* deaths = (uint32_t*)0x0058A364 + ((j * 12) + i);
                uint32_t result = *deaths;

                char buf[128];
                char name_buf[128];
                sprintf(buf, "%d", result);

                snprintf(name_buf, sizeof name_buf, "%s", ShortUnitName[j]);



                //snprintf(name_buf, sizeof name_buf, "%s", (*bw::stat_txt_tbl)->GetTblString(j + 1));
                text_surface.DrawText(&font, name_buf, Point32(10 + (100 * column), 10 + (10 * row)), 0x55);
                if (result != 0)
                    text_surface.DrawText(&font, buf, Point32(90 + (100 * column), 10 + (10 * row)), 0x80);

                row++;
                if (row == 40)
                {
                    row = 0;
                    column++;
                }

            }
            /*
            char str[128];
        auto &ai_data = bw::player_ai[i];
        snprintf(str, sizeof str, "Player %d: money %d / %d - need %d / %d / %d - available %d / %d / %d", i,
                bw::minerals[i], bw::gas[i], ai_data.mineral_need, ai_data.gas_need, ai_data.supply_need,
                ai_data.minerals_available, ai_data.gas_available, ai_data.supply_available);
        text_surface.DrawText(&font, str, info_pos, 0x55);
            */
        }
    }
}

void ScConsole::DrawAiInfo(uint8_t* textbuf, uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_ai_towns)
        return;
    Common::Surface surface(framebuf, w, h);
    Common::Surface text_surface(textbuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    Point32 info_pos(10, 30);
    Point32 region_text_pos(0, 10);
    for (unsigned int i = 0; i < Limits::ActivePlayers; i++)
    {
        if (bw::players[i].type != 1 || show_ai[i] == 0)
            continue;
        for (Ai::Town* town = bw::active_ai_towns[i]; town; town = town->list.next)
        {
            Rect32 rect = Rect32(Point32(town->position), 5).OffsetBy(screen_pos.Negate());
            surface.DrawRect(rect, 0xb9);
            if (town->mineral != nullptr)
                surface.DrawLine(town->mineral->sprite->position - screen_pos, town->position - screen_pos, 0x80);
            for (int j = 0; j < 3; j++)
            {
                if (town->gas_buildings[j] != nullptr)
                    surface.DrawLine(town->gas_buildings[j]->sprite->position - screen_pos, town->position - screen_pos, 0xba);
            }
            if (town->building_scv != nullptr)
                surface.DrawLine(town->building_scv->sprite->position - screen_pos, town->position - screen_pos, 0x71);
            char str[64];
            snprintf(str, sizeof str, "Inited: %d, workers %d / %d", town->inited, town->worker_count, town->unk1b);
            Point32 draw_pos = town->position - screen_pos + Point32(0 - strlen(str) * 3, 20);
            text_surface.DrawText(&font, str, draw_pos, 0x55);
            draw_pos += Point32(0, 10);
            std::string req_str = "Requests: ";
            uint32_t requests[0x65];
            requests[0x64] = 0;
            GetTownRequests(requests, 0x64, town->build_requests);
            for (int i = 0; requests[i] != 0; )
            {
                int line_len = draw_ai_named ? 4 : 8;
                for (int j = 0; j < line_len && requests[i] != 0; j++, i++)
                {
                    if (j != 0)
                        req_str.append(", ");
                    char buf[128];
                    char name_buf[128];
                    uint32_t request = requests[i];
                    int unit_id = request >> 16;
                    if (draw_ai_named && request & 0x2)
                        snprintf(name_buf, sizeof name_buf, "%s", UpgradeType(unit_id).Name());
                    else if (draw_ai_named && request & 0x4)
                        snprintf(name_buf, sizeof name_buf, "%s", TechType(unit_id).Name());
                    else if (draw_ai_named)
                        snprintf(name_buf, sizeof name_buf, "%s", (*bw::stat_txt_tbl)->GetTblString(unit_id + 1));
                    else
                        snprintf(name_buf, sizeof name_buf, "%x:%x", (request & 0x6) >> 1, unit_id);
                    if (request & 1)
                        snprintf(buf, sizeof buf, "%s (%d, if needed)", name_buf, (request & 0xf8) >> 3);
                    else
                        snprintf(buf, sizeof buf, "%s (%d)", name_buf, (request & 0xf8) >> 3);
                    req_str.append(buf);
                }
                text_surface.DrawText(&font, req_str, draw_pos + Point32(10, 0), 0x55);
                draw_pos += Point32(0, 10);
                req_str = "";
            }
        }
        char str[128];
        auto& ai_data = bw::player_ai[i];
        snprintf(str, sizeof str, "Player %d: money %d / %d - need %d / %d / %d - available %d / %d / %d", i,
            bw::minerals[i], bw::gas[i], ai_data.mineral_need, ai_data.gas_need, ai_data.supply_need,
            ai_data.minerals_available, ai_data.gas_available, ai_data.supply_available);
        text_surface.DrawText(&font, str, info_pos, 0x55);
        info_pos += Point32(0, 10);
        snprintf(str, sizeof str, "Request count %d, training unit %d, Script count %d",
            ai_data.request_count, ai_data.wanted_unit, CountScripts(i));
        text_surface.DrawText(&font, str, info_pos, 0x55);
        info_pos += Point32(0, 10);
        if (ai_data.attack_grouping_region != 0)
        {
            snprintf(
                str,
                sizeof str,
                "Attack region %x, started %d ago, failed %d",
                ai_data.attack_grouping_region - 1,
                *bw::elapsed_seconds - ai_data.last_attack_seconds,
                ai_data.attack_failed
            );
            text_surface.DrawText(&font, str, info_pos, 0x55);
            info_pos += Point32(0, 10);
        }
        if (ai_data.request_count)
        {
            std::string str = "Requests: ";
            std::unordered_set<uint32_t> collapsed_requests;
            for (int i = 0; i < ai_data.request_count;)
            {
                int line_len = draw_ai_named ? 4 : 8;
                for (int j = 0; j < line_len && i < ai_data.request_count;)
                {
                    bool skip = false;
                    auto unit_id = ai_data.requests[i].unit_id;
                    auto type = ai_data.requests[i].type;
                    int amt = 1;
                    uint32_t hashset_key = (unit_id << 16) | type;
                    if (i >= 4 && !draw_ai_full) {
                        if (collapsed_requests.count(hashset_key) != 0) {
                            skip = true;
                        }
                        else {
                            for (int k = i + 1; k < ai_data.request_count; k++) {
                                auto other_req = ai_data.requests[k];
                                if (other_req.unit_id == unit_id && other_req.type == type) {
                                    amt += 1;
                                }
                            }
                            collapsed_requests.emplace(hashset_key);
                        }
                    }
                    if (!skip) {
                        if (j != 0)
                            str.append(", ");
                        char buf[64];
                        const char* desc = RequestStr(type);
                        if (draw_ai_named && ai_data.requests[i].type == 5) {
                            snprintf(buf, sizeof buf, "%s %s", desc, UpgradeType(unit_id).Name());
                        }
                        else if (draw_ai_named && ai_data.requests[i].type == 6) {
                            snprintf(buf, sizeof buf, "%s %s", desc, TechType(unit_id).Name());
                        }
                        else if (draw_ai_named) {
                            auto name = (*bw::stat_txt_tbl)->GetTblString(unit_id + 1);
                            snprintf(buf, sizeof buf, "%s %s", desc, name);
                        }
                        else {
                            snprintf(buf, sizeof buf, "%s %x", desc, unit_id);
                        }
                        str.append(buf);
                        if (amt != 1) {
                            snprintf(buf, sizeof buf, " (x%d)", amt);
                            str.append(buf);
                        }
                        j += 1;
                    }
                    i += 1;
                }
                text_surface.DrawText(&font, str, info_pos + Point32(10, 0), 0x55);
                info_pos += Point32(0, 10);
                str = "";
            }
        }
        if (show_ai[i] == 2 && draw_ai_regions)
        {
            DrawAiRegions(i, &text_surface, region_text_pos);
            region_text_pos += Point32(0, 30);
        }
    }
}

void ScConsole::DrawAiUnitHomes(uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_ai_unit_homes)
        return;

    Common::Surface surface(framebuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (Unit* unit : *bw::first_active_unit)
    {
        if (unit->ai != nullptr && show_ai[unit->player] != 0)
        {
            auto sprite_pos_screen = unit->sprite->position - screen_pos;
            switch (unit->ai->type)
            {
            case 1: {
                auto ai = (Ai::GuardAi*)unit->ai;
                surface.DrawLine(ai->home - screen_pos, sprite_pos_screen, 0x80);
                if (ai->home != ai->unk_pos)
                {
                    surface.DrawLine(ai->unk_pos - screen_pos, sprite_pos_screen, 0xba);
                }
            } break;
            case 2: {
                auto ai = (Ai::WorkerAi*)unit->ai;
                surface.DrawLine(ai->town->position - screen_pos, sprite_pos_screen, 0x74);
            } break;
            case 3: {
                auto ai = (Ai::BuildingAi*)unit->ai;
                surface.DrawLine(ai->town->position - screen_pos, sprite_pos_screen, 0x74);
            } break;
            case 4: {
                auto ai = (Ai::MilitaryAi*)unit->ai;
                auto region = (*bw::pathing)->regions + ai->region->region_id;
                auto region_pos = Point32(region->x / 0x100, region->y / 0x100);
                surface.DrawLine(region_pos - screen_pos, sprite_pos_screen, 0x7c);
            } break;
            }
        }
    }
}

void ScConsole::DrawGuardAi(
    Common::Surface* surface,
    TextLayout* text_layout,
    Ai::GuardAi* ai,
    int player,
    bool alive
) {
    char str[128];
    char unit_name[64];
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    auto ai_pos = Point32(ai->home) - screen_pos;
    if (alive)
    {
        surface->DrawLine(ai->parent->sprite->position - screen_pos, ai_pos, 0x80);
    }

    Rect32 rect = Rect32(Point32(ai->home), 9).OffsetBy(screen_pos.Negate());
    surface->DrawRect(rect, 0xb9);
    if (ai->home != ai->unk_pos) {
        Rect32 rect = Rect32(Point32(ai->unk_pos), 9).OffsetBy(screen_pos.Negate());
        surface->DrawRect(rect, 0xb9);
    }
    if (ai->home != ai->unk_pos) {
        surface->DrawLine(ai->unk_pos - screen_pos, ai_pos, 0xba);
    }

    // Early exit if text is outside screen bounds
    int w = resolution::screen_width;
    int h = resolution::screen_height;
    if (ai_pos.x < -300 || ai_pos.y < 0 || ai_pos.x >= w + 100 || ai_pos.y >= h)
    {
        return;
    }
    if (draw_ai_named)
    {
        auto name = (*bw::stat_txt_tbl)->GetTblString(ai->unit_id + 1);
        snprintf(unit_name, sizeof unit_name, "%s", name);
    }
    else
    {
        snprintf(unit_name, sizeof unit_name, "%x", ai->unit_id);
    }
    if (alive)
    {
        snprintf(
            str,
            sizeof str,
            "Player %d, alive %s, deaths %d",
            player,
            unit_name,
            ai->times_died
        );
    }
    else
    {
        snprintf(
            str,
            sizeof str,
            "Player %d, needed %s, deaths %d",
            player,
            unit_name,
            ai->times_died
        );
    }
    if (ai->previous_update != 0)
    {
        char buf2[sizeof str];
        auto time = *bw::elapsed_seconds - ai->previous_update;
        snprintf(buf2, sizeof buf2, "%s, requested %d ago", str, time);
        strcpy(str, buf2);
    }
    text_layout->Draw(&font, str, ai_pos + Point32(-100, 10), 0x55);
}

void ScConsole::DrawAiGuards(uint8_t* text_buf, uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_ai_guards)
        return;

    Common::Surface surface(framebuf, w, h);
    Common::Surface text_surface(text_buf, w, h);
    TextLayout text(&text_surface);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (Unit* unit : *bw::first_active_unit)
    {
        if (unit->ai != nullptr && show_ai[unit->player] != 0 && unit->ai->type == 1)
        {
            DrawGuardAi(&surface, &text, (Ai::GuardAi*)unit->ai, unit->player, true);
        }
        if (unit->ai != nullptr && show_ai[unit->player] != 0 && unit->ai->type == 3)
        {
            auto ai = (Ai::BuildingAi*)unit->ai;
            for (int i = 0; i < 5; i++)
            {
                if (ai->train_queue_types[i] == 2 && ai->train_queue_values[i] != nullptr)
                {
                    auto guard_ai = (Ai::GuardAi*)ai->train_queue_values[i];
                    auto ai_pos = guard_ai->home - screen_pos;
                    surface.DrawLine(unit->sprite->position - screen_pos, ai_pos, 0xa4);
                }
            }
        }
    }
    for (Unit* unit : *bw::first_hidden_unit)
    {
        if (unit->ai != nullptr && show_ai[unit->player] != 0 && unit->ai->type == 1)
        {
            DrawGuardAi(&surface, &text, (Ai::GuardAi*)unit->ai, unit->player, true);
        }
    }
    for (int i = 0; i < Limits::ActivePlayers; i++)
    {
        if (show_ai[i])
        {
            for (Ai::GuardAi* ai : bw::first_guard_ai[i])
            {
                if (ai->parent == nullptr)
                {
                    DrawGuardAi(&surface, &text, ai, i, false);
                }
            }
        }
    }
}

void ScConsole::DrawResourceAreas(uint8_t* textbuf, uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_resource_areas)
        return;

    Common::Surface surface(framebuf, w, h);
    Common::Surface text_surface(textbuf, w, h);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (int i = 0; i < bw::resource_areas->used_count; i++)
    {
        // First entry is not used
        const auto& area = bw::resource_areas->areas[i + 1];
        int x = area.position.x - screen_pos.x;
        int y = area.position.y - screen_pos.y;
        if (x >= 0 && x < w && y >= 0 && y < h)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Area %x [%d]: Mine %d in %d, Gas %d in %d, flags %02x", i + 1, i + 1,
                area.total_minerals, area.mineral_field_count,
                area.total_gas, area.geyser_count, area.flags);
            text_surface.DrawText(&font, buf, Point32(x - 50, y + 20), 0x55);
            snprintf(buf, sizeof buf, "Unk: %02x %08x %08x %08x %08x", area.is_start_location,
                area.unk10[0], area.unk10[1], area.unk10[2], area.unk10[3]);
            text_surface.DrawText(&font, buf, Point32(x - 50, y + 30), 0x55);
            snprintf(buf, sizeof buf, "%08x %08x %08x %08x",
                area.unk10[4], area.unk10[5], area.unk10[6], area.unk10[7]);
            text_surface.DrawText(&font, buf, Point32(x - 50, y + 40), 0x55);
            Rect32 rect = Rect32(Point32(area.position), 15).OffsetBy(screen_pos.Negate());
            surface.DrawRect(rect, 0xb9);
        }
    }
}

void ScConsole::DrawOrders(uint8_t* textbuf, uint8_t* framebuf, xuint w, yuint h)
{
    if (draw_orders == OrderDrawMode::None)
        return;

    Common::Surface surface(framebuf, w, h);
    Common::Surface text_surface(textbuf, w, h);

    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    if (draw_orders == OrderDrawMode::All)
    {
        vector<int> ignoredOrders = { 79, 88, 89, 90, 150, 151, 2, 84 };  // Spam internal AI orders for workers that nobody cares about: Harvest1/Harvest3/Harvest4/ReturnMinerals/ResetHarvestCollision/ResetCollision/Guard/ReturnGas
        for (Unit* unit : *bw::first_active_unit)
        {
            // We don't want to draw main orders, since CoachAI already do that
            //if (unit->target)
            //    surface.DrawLine(unit->target->sprite->position - screen_pos, unit->sprite->position - screen_pos, 0xa4);
            //else if (unit->order_target_pos != Point16(0, 0))
            //    surface.DrawLine(unit->order_target_pos - screen_pos, unit->sprite->position - screen_pos, 0xa4);

            // draw queued orders
            std::string tempQueue = "";
            auto currentOrder = unit->target ? unit->target->sprite->position - screen_pos : unit->order_target_pos - screen_pos;
            for (Order* queuedOrder : unit->order_queue_begin)
            {
                if (std::find(ignoredOrders.begin(), ignoredOrders.end(), queuedOrder->order_id) != ignoredOrders.end())
                    continue;

                tempQueue += std::string(OrderIdToName[queuedOrder->order_id]) + ", "; // bf that I was using queuedOrder->Type().Name(queuedOrder->order_id) which allowed me to get orderIdToName from .dat files
                // color 0x80=Cyan, 9c=Orange, get the rest from "CoachAId colors.png"
                text_surface.DrawText(&font, tempQueue, unit->sprite->position - screen_pos + Point32(0, 30), 0x80, [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });

                if (queuedOrder->position == Point16(0, 0)) // SCV while building, HoldPosition, or any order that has no target.. all points to 0,0
                    continue;

                if (currentOrder.x < 0 || currentOrder.y < 0)
                    ;
                else
                    surface.DrawDottedLine(currentOrder, queuedOrder->position - screen_pos, 0x80);

                currentOrder = queuedOrder->position - screen_pos;
            }
        }
    }
    else if (draw_orders == OrderDrawMode::Selected)
    {
        for (Unit* unit : client_select)
        {
            if (unit->target)
                surface.DrawLine(unit->target->sprite->position - screen_pos, unit->sprite->position - screen_pos, 0xa4);
            else if (unit->order_target_pos != Point16(0, 0))
                surface.DrawLine(unit->order_target_pos - screen_pos, unit->sprite->position - screen_pos, 0xa4);
        }
    }
}

void ScConsole::DrawCoords(uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_coords)
        return;

    char buf[32];
    snprintf(
        buf, sizeof buf, "Game: %04dx%04d",
        *bw::screen_x + *bw::mouse_clickpos_x,
        *bw::screen_y + *bw::mouse_clickpos_y
    );
    info_lines.emplace_back(buf);
    snprintf(buf, sizeof buf, "Mouse: %04dx%04d", (int)*bw::mouse_clickpos_x, (int)*bw::mouse_clickpos_y);
    info_lines.emplace_back(buf);
    snprintf(buf, sizeof buf, "Screen: %04dx%04d", (int)*bw::screen_x, (int)*bw::screen_y);
    info_lines.emplace_back(buf);
}

void ScConsole::DrawRange(uint8_t* framebuf, xuint w, yuint h)
{
    if (!draw_range)
        return;

    Common::Surface surface(framebuf, w, resolution::game_height);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    for (Unit* unit : *bw::first_active_unit)
    {
        Point32 pos = Point32(unit->sprite->position) - screen_pos;
        WeaponType ground_weapon = unit->GetGroundWeapon();
        WeaponType air_weapon = unit->GetAirWeapon();
        const auto dbox = unit->Type().DimensionBox();
        int unit_radius_approx = (dbox.top + dbox.bottom + dbox.left + dbox.right) / 4 + 1;
        if (ground_weapon != WeaponId::None)
            surface.DrawCircle(pos, unit->GetWeaponRange(true) + unit_radius_approx, 0x75,
                [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
        if (air_weapon != WeaponId::None && ground_weapon != air_weapon)
            surface.DrawCircle(pos, unit->GetWeaponRange(false) + unit_radius_approx, 0x7a,
                [](int x, int y) { return !bw::IsOutsideGameScreen(x, y); });
    }
}

void ScConsole::DrawGrids(uint8_t* framebuf, xuint w, yuint h)
{
    if (grids.empty())
        return;

    Common::Surface surface(framebuf, w, resolution::game_height);
    Point32 screen_pos(*bw::screen_x, *bw::screen_y);
    x32 x_end = screen_pos.x + w;
    y32 y_end = screen_pos.y + h;
    for (const auto& grid : grids)
    {
        x32 x = 0 - screen_pos.x % grid.width - 1;
        while (x < x_end)
        {
            surface.DrawLine(Point32(x, 0), Point32(x, h), grid.color);
            x += grid.width;
        }
        y32 y = 0 - screen_pos.y % grid.height - 1;
        while (y < y_end)
        {
            surface.DrawLine(Point32(0, y), Point32(w, y), grid.color);
            y += grid.height;
        }
    }
}

void ScConsole::DrawDebugInfo(uint8_t* framebuf, xuint w, yuint h)
{
    ConstructInfoLines();
    uint8_t buffer[resolution::screen_width * resolution::screen_height];
    uint8_t text_buf[resolution::screen_width * resolution::screen_height];
    memset(buffer, 0, sizeof buffer);
    memset(text_buf, 0, sizeof text_buf);
    DrawLocations(framebuf, w, h);
    DrawCrects(framebuf, w, h);
    DrawGrids(buffer, resolution::screen_width, resolution::screen_height);
    DrawAiInfo(text_buf, buffer, resolution::screen_width, resolution::screen_height);
    //IQUARE
    DrawDeaths(text_buf, buffer, resolution::screen_width, resolution::screen_height);
    DrawAiUnitHomes(buffer, resolution::screen_width, resolution::screen_height);
    DrawAiGuards(text_buf, buffer, resolution::screen_width, resolution::screen_height);
    DrawResourceAreas(text_buf, buffer, resolution::screen_width, resolution::screen_height);
    DrawOrders(text_buf, buffer, resolution::screen_width, resolution::screen_height);
    DrawCoords(buffer, resolution::screen_width, resolution::screen_height);
    DrawRange(buffer, resolution::screen_width, resolution::screen_height);
    if (draw_region_data)
    {
        DrawRegionData(text_buf, w, h);
    }
    if (!info_lines.empty())
    {
        int info_lines_width = font.TextLength(*std::max_element(info_lines.begin(), info_lines.end(),
            [this](const auto& a, const auto& b) {
                return font.TextLength(a) < font.TextLength(b);
            }));
        Point32 info_pos(resolution::screen_width - info_lines_width - 10, 40);
        Common::Surface surf(buffer, resolution::screen_width, resolution::screen_height);
        for (const auto& line : info_lines)
        {
            surf.DrawText(&font, line, info_pos, 0x55);
            info_pos += Point32(0, 10);
        }
    }
    for (unsigned y = 0; y < resolution::game_height; y++)
    {
        for (unsigned x = 0; x < resolution::game_width; x += 4)
        {
            if (*(uint32_t*)(buffer + y * resolution::screen_width + x) == 0)
                continue;
            if (buffer[y * resolution::screen_width + x] != 0 && !bw::IsOutsideGameScreen(x, y))
                framebuf[y * w + x] = buffer[y * resolution::screen_width + x];
            if (buffer[y * resolution::screen_width + x + 1] != 0 && !bw::IsOutsideGameScreen(x + 1, y))
                framebuf[y * w + x + 1] = buffer[y * resolution::screen_width + x + 1];
            if (buffer[y * resolution::screen_width + x + 2] != 0 && !bw::IsOutsideGameScreen(x + 2, y))
                framebuf[y * w + x + 2] = buffer[y * resolution::screen_width + x + 2];
            if (buffer[y * resolution::screen_width + x + 3] != 0 && !bw::IsOutsideGameScreen(x + 3, y))
                framebuf[y * w + x + 3] = buffer[y * resolution::screen_width + x + 3];
        }
    }
    for (unsigned y = 1; y < resolution::screen_height - 1; y++)
    {
        for (unsigned x = 1; x < resolution::screen_width - 1; x++)
        {
            auto color = text_buf[y * resolution::screen_width + x];
            if (color != 0)
            {
                if (text_buf[y * resolution::screen_width + x - 1] == 0)
                {
                    framebuf[y * w + x - 1] = 0;
                    if (x > 1 && text_buf[y * resolution::screen_width + x - 2] == 0)
                        framebuf[y * w + x - 2] = 0;
                }
                if (text_buf[y * resolution::screen_width + x + 1] == 0)
                    framebuf[y * w + x + 1] = 0;
                if (text_buf[(y + 1) * resolution::screen_width + x] == 0)
                    framebuf[(y + 1) * w + x] = 0;
                if (text_buf[(y - 1) * resolution::screen_width + x] == 0)
                    framebuf[(y - 1) * w + x] = 0;
                framebuf[y * w + x] = text_buf[y * resolution::screen_width + x];
            }
        }
    }
    UpdateFastForwardProgress();
}

void ScConsole::UpdateFastForwardProgress() {
    if (isFastForwarding) {
        int duration = fastForwardEndFrames - fastForwardStartFrames;
        int part = *bw::frame_count - fastForwardStartFrames;
        float progress = std::roundf(((float)part / (float)duration) * 100);
        Clear();
        Printf("Fast forwarding %d game seconds... %2.0f%%.", duration / 24, progress);
        Print("To stop, press ESC.");

    }

    if (!isFastForwarding && fastForwardEndFrames > 0) {
        EndFastForward();
    }
}

void ScConsole::EndFastForward() {
    isFastForwarding = false;
    fastForwardStartFrames = 0;
    fastForwardEndFrames = 0;
    bw::game_speed_waits[*bw::game_speed] = 42;
    Clear();
}

bool ScConsole::ShowAi(const CmdArgs& args)
{
    draw_ai_towns = !draw_ai_towns;
    draw_ai_data = !draw_ai_data;
    return true;
}

bool ScConsole::ShowAiGuards(const CmdArgs& args)
{
    draw_ai_guards = !draw_ai_guards;
    draw_ai_towns = true;
    draw_ai_data = true;
    return true;
}

bool ScConsole::ShowAiUnits(const CmdArgs& args)
{
    draw_ai_unit_homes = !draw_ai_unit_homes;
    return true;
}

bool ScConsole::ShowAiPlayer(const CmdArgs& args)
{
    if (std::string(args[1]) == "all")
    {
        for (int i = 0; i < Limits::Players; i++)
            show_ai[i] = 1;
    }
    else
    {
        int player = atoi(args[1]);
        if (IsActivePlayer(player))
        {
            for (int i = 0; i < Limits::Players; i++)
                show_ai[i] = 0;
            show_ai[player] = 2;
        }
    }
    draw_ai_towns = true;
    draw_ai_data = true;
    return true;
}

bool ScConsole::Show(const CmdArgs& args)
{
    std::string what(args[1]);

    if (what == "nothing")
    {
        //IQUARE
        draw_locations = draw_paths = draw_crects = draw_coords = draw_info =
            draw_range = draw_region_borders = draw_region_data = draw_ai_data = draw_ai_towns =
            show_fps = show_frame = draw_resource_areas = draw_deaths = false;
        draw_orders = OrderDrawMode::None;
    }
    else if (what == "frame")
        show_frame = !show_frame;
    else if (what == "locations")
        draw_locations = !draw_locations;
    else if (what == "paths")
        draw_paths = !draw_paths;
    else if (what == "collision")
        draw_crects = !draw_crects;
    else if (what == "coords")
        draw_coords = !draw_coords;
    else if (what == "info")
        draw_info = !draw_info;
    else if (what == "range")
        draw_range = !draw_range;
    else if (what == "resareas")
        draw_resource_areas = !draw_resource_areas;
    else if (what == "regions")
    {
        draw_region_borders = !draw_region_borders;
        draw_region_data = draw_region_borders;
    }
    else if (what == "orders")
    {
        std::string more(args[2]);
        if (more == "selected")
            draw_orders = OrderDrawMode::Selected;
        else if (more == "all")
            draw_orders = OrderDrawMode::All;
        else if (more != "")
            return false;
        else
        {
            if (draw_orders == OrderDrawMode::None)
                draw_orders = OrderDrawMode::All;
            else
                draw_orders = OrderDrawMode::None;
        }
    }
    else if (what == "deaths")
    {
        //IQUARE
        std::string more(args[2]);
        if (more == "")
        {
            draw_deaths = false;
        }
        else
        {
            int player = atoi(args[2]);
            draw_deaths = true;
            for (int i = 0; i < Limits::Players; i++)
                show_deaths[i] = 0;
            show_deaths[player] = 1;
        }
    }
    else if (what == "ai")
    {
        std::string more(args[2]);
        if (more == "full")
            draw_ai_full = true;
        else if (more == "simple")
            draw_ai_full = false;
        else if (more == "named")
            draw_ai_named = true;
        else if (more == "raw")
            draw_ai_named = false;
        else if (more == "player")
        {
            if (std::string(args[3]) == "all")
            {
                for (int i = 0; i < Limits::Players; i++)
                    show_ai[i] = 1;
            }
            else
            {
                int player = atoi(args[3]);
                if (IsActivePlayer(player))
                {
                    for (int i = 0; i < Limits::Players; i++)
                        show_ai[i] = 0;
                    show_ai[player] = 2;
                }
            }
        }
        else if (more == "units")
        {
            draw_ai_unit_homes = !draw_ai_unit_homes;
        }
        else if (more == "guards")
        {
            draw_ai_guards = !draw_ai_guards;
        }
        else if (more == "regions")
        {
            draw_ai_regions = !draw_ai_regions;
        }
        else if (more == "")
        {
            draw_ai_towns = !draw_ai_towns;
            draw_ai_data = !draw_ai_data;
        }
        else
        {
            return false;
        }
        if (more != "" && more != "units")
        {
            draw_ai_towns = true;
            draw_ai_data = true;
        }
    }
    else
    {
        Printf("show <nothing|frame|regions|locations|paths|collision|coords|range|info|resareas>");
        Printf("show ai [full|simple|named|raw|units|guards|(player <player|all>>)]");
        Printf("show orders [all|selected]");
        return false;
    }
    return true;
}

static void DrawHook(uint8_t* framebuf, xuint w, yuint h)
{
    if (console)
    {
        // Only draw console if shown
        if (((ScConsole*)console)->state == 2)  // ← Add this check
        {
            if (IsInGame())
                ((ScConsole*)console)->DrawDebugInfo(framebuf, w, h);
            console->Draw(framebuf, w, h);
        }
    }
}
void PatchConsole()
{
    console = new ScConsole;
    if (!console->IsOk())
        return;
    AddDrawHook(&DrawHook, 500);
    AddDrawHook(&DrawPathingInfo, 450);
}

bool ScConsole::Sc_KeyDown(int key, int scan)
{
    // Handle cursor movement and text editing when console IS VISIBLE AND SHOWN
    if (state == shown) {
        switch (key)
        {
        case VK_LEFT:
            if (cursor_pos > 0) {
                cursor_pos--;
            }
            return true;  // Consume the key
        case VK_RIGHT:
            if (cursor_pos < current_cmd.length()) {
                cursor_pos++;
            }
            return true;
        case VK_HOME:
            cursor_pos = 0;
            return true;
        case VK_END:
            cursor_pos = current_cmd.length();
            return true;
        }
    }

    // Handle other keys when console is HIDDEN (game is running)
    if (state != shown) {
        switch (key)
        {
        case VK_OEM_PLUS:
        case VK_ADD:
            frame_skip_ms = 360;
            bw::game_speed_waits[*bw::game_speed] = 0;
            break;
        }
    }

    switch (key)
    {
    case VK_ESCAPE:
        if (isFastForwarding)
            EndFastForward();
        break;
    }

    return false;
}

bool ScConsole::Sc_KeyUp(int key, int scan)
{
    if (state != shown) {
        switch (key)
        {
        case VK_OEM_PLUS:
        case VK_ADD:
            frame_skip_ms = 0;
            bw::game_speed_waits[*bw::game_speed] = 42;
            break;
        }
    }
    return false;
}

bool ScConsole::KeyHook(int key, int scan)
{
    // Handle cursor movement and other keys in console
    if (state == shown) {
        switch (scan)
        {
        case 0x4B: // Left arrow
            if (cursor_pos > 0) {
                cursor_pos--;
                dirty = true;
            }
            return true;
        case 0x4D: // Right arrow
            if (cursor_pos < current_cmd.length()) {
                cursor_pos++;
                dirty = true;
            }
            return true;
        case 0x47: // Home
            cursor_pos = 0;
            dirty = true;
            return true;
        case 0x4F: // End
            cursor_pos = current_cmd.length();
            dirty = true;
            return true;
        }
    }

    // Call parent's KeyHook for other keys (up/down history, ~)
    return Console::KeyHook(key, scan);
}

// Updated CharHook version 2 (simpler):
bool ScConsole::CharHook(wchar_t chr)
{
    if (state != shown)
        return false;

    dirty = true;

    switch (chr)
    {
    case 0x8: // Backspace
        if (cursor_pos > 0) {
            current_cmd.erase(cursor_pos - 1, 1);
            cursor_pos--;
        }
        break;
    case 0x1b: // Escape
        current_cmd.clear();
        cursor_pos = 0;
        ResetHistoryNavigation();
        break;
    case 0xd: // Enter
        ProcessCommand();
        cursor_pos = 0;
        break;
    default:
    {
        char buf[8];
        int count = WideCharToMultiByte(CP_UTF8, 0, &chr, 1, buf, sizeof buf, NULL, NULL);

        // Prevent buffer overflow - limit to 512 chars
        const size_t MAX_CMD_LENGTH = 512;
        if (current_cmd.length() + count < MAX_CMD_LENGTH)
        {
            for (int i = 0; i < count; i++)
                current_cmd.insert(cursor_pos + i, 1, buf[i]);
            cursor_pos += count;
        }
    }
    break;
    }

    return true;
}
