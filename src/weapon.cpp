#include <helper.hpp>
#include <game.h>
#include "component_loader.h"
#include "cevar.h"
#include <Hooking.Patterns.h>
#include "utils/hooking.h"

#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

#include <optional>
#include "GMath.h"

namespace weapon {
    cevar_t* cg_weaponBobAmplitudeSprinting_horz;
    cevar_t* cg_weaponBobAmplitudeSprinting_vert;

    cevar_t* cg_bobAmplitudeSprinting_horz;

    cevar_t* cg_bobAmplitudeSprinting_vert;

    cevar_t* cg_weaponSprint_mod;

    cevar_t* player_sprintSpeedScale;

    struct eWeaponDef {
        std::string weaponName;
        float vSprintBob[2];
        float sprintSpeedScale;
        std::optional<std::array<float, 3>> vSprintRot;
        std::optional<std::array<float, 3>> vSprintMove;

        eWeaponDef() : weaponName(""), vSprintBob{ 0.0f, 0.0f }, sprintSpeedScale{ 1.f }, vSprintRot{}, vSprintMove{} {}
    };

    std::unordered_map<std::string, eWeaponDef> g_eWeaponDefs;


    bool sprint_rotate_is_sprinting() {
        return (player_flags && (*player_flags & 0x10000) != 0) && (cg_weaponSprint_mod && cg_weaponSprint_mod->base->integer);
    }

    const char* GetCurrentWeaponName() {
        uintptr_t current_weapon = *(uintptr_t*)cg(0x30275DF0);
        if (!current_weapon)
            return nullptr;

        if (!*(uintptr_t*)(current_weapon + 0x4))
            return nullptr;

        return *(const char**)(current_weapon + 0x4);

    }

    const eWeaponDef* GetEWeapon(const char* weaponName) {
        if (!weaponName || g_eWeaponDefs.empty()) return nullptr;
        auto it = g_eWeaponDefs.find(weaponName);
        if (it != g_eWeaponDefs.end()) {
            return &it->second;
        }

        return nullptr;
    }

    const eWeaponDef* GetCurrentEWeapon() {
        return GetEWeapon(GetCurrentWeaponName());
    }

    double __cdecl CG_GetWeaponVerticalBobFactor(float a1, float a2, float a3)
    {
        double v3; // st7
        double v4; // st7
        float v6; // [esp+0h] [ebp-4h]



        if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF40))
        {
            v3 = (double)*(float*)cg(0x30256828);
        }
        else if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF44))
        {
            v3 = *(float*)cg(0x3022C9A8);
        }
        else
        {
            v3 = *(float*)cg(0x30255CE8);
        }

        if (sprint_rotate_is_sprinting()) {
            const char* weaponName = GetCurrentWeaponName();
            if (weaponName) {
                auto eWeapon = GetEWeapon(weaponName);
                if (eWeapon) {
                    float vert_bob = cg_weaponBobAmplitudeSprinting_vert->base->value * eWeapon->vSprintBob[1];
                    v3 = vert_bob;
                }
            }
        }

        v4 = v3 * a2;
        v6 = v4;
        if (v4 > a3)
            v6 = a3;
        return (sin(a1 * 4.0 + 1.5707964) * 0.2 + sin(a1 + a1)) * v6 * 0.75;
    }

    double __cdecl CG_GetViewVerticalBobFactor_sprint(float a1, float a2, float a3)
    {
        double v3; // st7
        double v4; // st7
        float v6; // [esp+0h] [ebp-4h]



        if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF40))
        {
            v3 = (double)*(float*)cg(0x30256828);
        }
        else if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF44))
        {
            v3 = *(float*)cg(0x3022C9A8);
        }
        else
        {
            v3 = *(float*)cg(0x30255CE8);
        }

        if (sprint_rotate_is_sprinting()) {
            v3 = cg_bobAmplitudeSprinting_vert->base->value;
        }

        v4 = v3 * a2;
        v6 = v4;
        if (v4 > a3)
            v6 = a3;
        return (sin(a1 * 4.0 + 1.5707964) * 0.2 + sin(a1 + a1)) * v6 * 0.75;
    }

    double __cdecl CG_GetViewGetHorizontalBobFactor_sprint(float a1, float a2, float a3)
    {
        double v3; // st7
        double v4; // st7
        float v6; // [esp+0h] [ebp-4h]



        if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF40))
        {
            v3 = (double)*(float*)cg(0x30256828);
        }
        else if (*(uintptr_t*)cg(0x3026DAC8) == *(uintptr_t*)cg(0x3026DF44))
        {
            v3 = *(float*)cg(0x3022C9A8);
        }
        else
        {
            v3 = *(float*)cg(0x30255CE8);
        }

        if (sprint_rotate_is_sprinting()) {
            v3 = cg_bobAmplitudeSprinting_horz->base->value;
        }

        v4 = v3 * a2;
        v6 = v4;
        if (v4 > a3)
            v4 = a3;
        return v4 * sin(a1);
    }

    struct six_long {
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
        uint8_t b4;
        uint8_t b5;
        uint8_t b6;
    };

    six_long original_reads;

    six_long original_reads_1;

    six_long original_reads_2;

    void SprintT4_lol(HMODULE handle) {
        if (!sp_mp(1))
            return;

        auto pattern = hook::pattern(handle, "A1 ? ? ? ? A9 ? ? ? ? 0F 84");

        if (!pattern.empty()) {
            player_flags = *pattern.get_first<uint32_t*>(1);
        }

        pattern = hook::pattern(handle, "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? EB ? C7 44 24 ? 00 00 00 00 C7 44 24 ? 00 00 00 00 C7 44 24 ? 00 00 00 00 ? ? 5B");

        //sprint_cevar_assembly(NULL, "hi");
        DWORD old_protect = 0;
        VirtualProtect((void*)cg(0x30031DF0), 6, PAGE_EXECUTE_READWRITE, &old_protect);
        VirtualProtect((void*)cg(0x30031DFE), 6, PAGE_EXECUTE_READWRITE, &old_protect);
        VirtualProtect((void*)cg(0x30031E10), 6, PAGE_EXECUTE_READWRITE, &old_protect);
        //if (!pattern.empty())
        //    CreateMidHook(pattern.get_first(), [](SafetyHookContext& ctx) {

        //    if ((!(player_flags && (*player_flags & 0x10000) != 0) && (cg_weaponSprint_mod && cg_weaponSprint_mod->base->integer)) || !cg_weaponSprint_mod->base->integer) {

        //        Memory::Patch(cg(0x30031DF0), original_reads);
        //        Memory::Patch(cg(0x30031DFE), original_reads_1);
        //        Memory::Patch(cg(0x30031E10), original_reads_2);

        //    }
        //    else {
        //        Memory::Patch(cg(0x30031DF0), { 0xD9, 0x82, 0x28, 0x01, 0x00, 0x00 });
        //        Memory::Patch(cg(0x30031DFE), { 0xD9, 0x82, 0x2C, 0x01, 0x00, 0x00 });
        //        Memory::Patch(cg(0x30031E10), { 0xD9, 0x82, 0x30, 0x01, 0x00, 0x00 });
        //    }


        //        });




        Memory::VP::Read(cg(0x30031DF0), original_reads);
        Memory::VP::Read(cg(0x30031DFE), original_reads_1);
        Memory::VP::Read(cg(0x30031E10), original_reads_2);

        Memory::VP::Nop((void*)cg(0x30031DF0), 6);
        Memory::VP::Nop((void*)cg(0x30031DFE), 6);
        Memory::VP::Nop((void*)cg(0x30031E10), 6);

        CreateMidHook(cg(0x30031DF0), [](SafetyHookContext& ctx) {
            vmCvar_t* cg_gun_rot_p = (vmCvar_t*)cg(0x30258E60);
            vector3* game_sprint_rot = (vector3*)(ctx.edx + 0x128);
            auto eWeapon = GetCurrentEWeapon();
            if (sprint_rotate_is_sprinting()) {

                
                if (eWeapon && eWeapon->vSprintRot && eWeapon->vSprintRot.has_value()) {
                    FPU::FLD((*eWeapon->vSprintRot)[0]);
                    return;
                }
                    FPU::FLD(game_sprint_rot->x);
                    return;
            }

            FPU::FLD(cg_gun_rot_p->value);
            });

        CreateMidHook(cg(0x30031DFE), [](SafetyHookContext& ctx) {
            vmCvar_t* cg_gun_rot_y = (vmCvar_t*)cg(0x30256CA0);
            vector3* game_sprint_rot = (vector3*)(ctx.edx + 0x128);
            auto eWeapon = GetCurrentEWeapon();
            if (sprint_rotate_is_sprinting()) {


                if (eWeapon && eWeapon->vSprintRot && eWeapon->vSprintRot.has_value()) {
                    FPU::FLD((*eWeapon->vSprintRot)[1]);
                    return;
                }
                FPU::FLD(game_sprint_rot->y);
                return;
            }

            FPU::FLD(cg_gun_rot_y->value);
            });

        CreateMidHook(cg(0x30031E10), [](SafetyHookContext& ctx) {
            vmCvar_t* cg_gun_rot_r = (vmCvar_t*)cg(0x30252A40);

            vector3* game_sprint_rot = (vector3*)(ctx.edx + 0x128);
            auto eWeapon = GetCurrentEWeapon();
            if (sprint_rotate_is_sprinting()) {
                if(eWeapon)

                if (eWeapon && eWeapon->vSprintRot && eWeapon->vSprintRot.has_value()) {
                    FPU::FLD((*eWeapon->vSprintRot)[2]);
                    return;
                }
                FPU::FLD(game_sprint_rot->z);
                return;
            }

            FPU::FLD(cg_gun_rot_r->value);
            });

        CreateMidHook(cg(0x3003104E), [](SafetyHookContext& ctx) {

            if (player_flags && (*player_flags & 0x10000) != 0) {
                const char* weaponName = GetCurrentWeaponName();
                if (weaponName) {
                    auto eWeapon = GetEWeapon(weaponName);
                    if (eWeapon && cg_weaponSprint_mod->base->value) {
                        float horz_bob = cg_weaponBobAmplitudeSprinting_horz->base->value * eWeapon->vSprintBob[0];
                        //printf("horz_bob %f\n", horz_bob);
                        FPU::FLD(horz_bob);
                        ctx.eip = cg(0x30031054);
                    }
                }
            }

            });

        Memory::VP::Nop(cg(0x30031A25), 6);
        Memory::VP::Nop(cg(0x30031A2D), 6);
        Memory::VP::Nop(cg(0x30031A35), 6);

        CreateMidHook(cg(0x30031A25), [](SafetyHookContext& ctx) {
            vector3* game_sprintMove = (vector3*)(ctx.edx + 0x11C);

            auto eWeapon = GetCurrentEWeapon();
            if (eWeapon && eWeapon->vSprintMove && eWeapon->vSprintMove.has_value() && cg_weaponSprint_mod->base->value) {
                FPU::FMUL((*eWeapon->vSprintMove)[0]);
                return;
            }

            FPU::FMUL(game_sprintMove->x);
            });

        CreateMidHook(cg(0x30031A2D), [](SafetyHookContext& ctx) {
            vector3* game_sprintMove = (vector3*)(ctx.edx + 0x11C);

            auto eWeapon = GetCurrentEWeapon();
            if (eWeapon && eWeapon->vSprintMove && eWeapon->vSprintMove.has_value() && cg_weaponSprint_mod->base->value) {
                FPU::FMUL((*eWeapon->vSprintMove)[1]);
                return;
            }

            FPU::FMUL(game_sprintMove->y);
            });

        CreateMidHook(cg(0x30031A35), [](SafetyHookContext& ctx) {
            vector3* game_sprintMove = (vector3*)(ctx.edx + 0x11C);


            auto eWeapon = GetCurrentEWeapon();
            if (eWeapon && eWeapon->vSprintMove && eWeapon->vSprintMove.has_value() && cg_weaponSprint_mod->base->value) {
                FPU::FMUL((*eWeapon->vSprintMove)[2]);
                return;
            }

            FPU::FMUL(game_sprintMove->z);
            });


        Memory::VP::InjectHook(cg(0x30031010), CG_GetWeaponVerticalBobFactor);

    }


    void LoadEWeaponsFromDirectory(const std::filesystem::path& eWeaponsDir, bool overwrite = true) {
        if (!std::filesystem::exists(eWeaponsDir)) {
            Com_Printf("eWeapons directory not found: %s\n", eWeaponsDir.string().c_str());
            return;
        }

        Com_Printf("Loading eWeapons from: %s\n", eWeaponsDir.string().c_str());

        for (const auto& entry : std::filesystem::directory_iterator(eWeaponsDir)) {
            if (entry.path().extension() != ".json") continue;

            std::string weaponName = entry.path().stem().string();

            if (!overwrite && g_eWeaponDefs.find(weaponName) != g_eWeaponDefs.end()) {
                Com_Printf("Skipping '%s' (already loaded with higher priority)\n", weaponName.c_str());
                continue;
            }

            try {
                std::ifstream file(entry.path());
                nlohmann::json j = nlohmann::json::parse(file);

                eWeaponDef weaponDef;
                weaponDef.weaponName = weaponName;

                if (j.contains("sprintBobH")) {
                    weaponDef.vSprintBob[0] = j["sprintBobH"].get<float>();
                }

                if (j.contains("sprintBobV")) {
                    weaponDef.vSprintBob[1] = j["sprintBobV"].get<float>();
                }

                if (j.contains("sprintSpeedScale")) {
                    weaponDef.sprintSpeedScale = j["sprintSpeedScale"].get<float>();
                }

                if (j.contains("SprintRot") && j["SprintRot"].is_array() && j["SprintRot"].size() == 3) {
                    weaponDef.vSprintRot = std::array<float, 3>{
                        j["SprintRot"][0].get<float>(),
                        j["SprintRot"][1].get<float>(),
                        j["SprintRot"][2].get<float>()
                    };
                }

                

                if (j.contains("SprintMove") && j["SprintMove"].is_array() && j["SprintMove"].size() == 3) {
                    weaponDef.vSprintMove = std::array<float, 3>{
                        j["SprintMove"][0].get<float>(),
                        j["SprintMove"][1].get<float>(),
                        j["SprintMove"][2].get<float>()
                    };
                }

                g_eWeaponDefs[weaponName] = weaponDef;

                Com_Printf("Loaded eWeapon '%s' - sprintBobH: %.3f, sprintBobV: %.3f, sprintSpeedScale %.3f\n",
                    weaponName.c_str(),
                    weaponDef.vSprintBob[0],
                    weaponDef.vSprintBob[1],
                    weaponDef.sprintSpeedScale);

            }
            catch (const std::exception& e) {
                Com_Printf("Failed to parse %s: %s\n",
                    entry.path().string().c_str(), e.what());
            }
        }
    }

    void loadEWeapons() {
        if (!sp_mp(1, 0))
            return;
        g_eWeaponDefs.clear();
        char modulePath[MAX_PATH];
        GetModuleFileNameA(NULL, modulePath, MAX_PATH);
        std::filesystem::path exePath(modulePath);
        std::filesystem::path baseDir = exePath.parent_path();
        cvar_s* fs_game = Cvar_Find("fs_game");
        cvar_s* fs_basegame = Cvar_Find("fs_basegame");

        std::filesystem::path baseEWeaponsDir = baseDir / "eWeapons";
        LoadEWeaponsFromDirectory(baseEWeaponsDir, true);

        if (fs_basegame && fs_basegame->string && fs_basegame->string[0] != '\0') {
            std::filesystem::path fsBaseGameDir = baseDir / fs_basegame->string;
            std::filesystem::path fsBaseGameEWeaponsDir = fsBaseGameDir / "eWeapons";
            LoadEWeaponsFromDirectory(fsBaseGameEWeaponsDir, true);
        }

        if (fs_game && fs_game->string && fs_game->string[0] != '\0') {
            std::filesystem::path fsGameDir = baseDir / fs_game->string;
            std::filesystem::path fsGameEWeaponsDir = fsGameDir / "eWeapons";
            LoadEWeaponsFromDirectory(fsGameEWeaponsDir, true);
            Com_Printf("Loaded %d eWeapon definitions (fs_game: '%s' has priority)\n",
                g_eWeaponDefs.size(),
                fs_game->string);
        }
        else {
            Com_Printf("Loaded %d eWeapon definitions from base directory\n",
                g_eWeaponDefs.size());
        }
    }

    void PatchSprintScale(HMODULE handle) {
        if (!sp_mp(1))
            return;
        auto pattern = hook::pattern(handle, "? ? ? ? ? ? EB ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? 8B 46");
        if (!pattern.empty() && player_sprintSpeedScale && player_sprintSpeedScale->base) {


            Memory::VP::Nop(pattern.get_first(), 6);
            CreateMidHook(pattern.get_first(), [](SafetyHookContext& ctx) {

                float current_speedscale = player_sprintSpeedScale->base->value;
                auto eWeapon = GetCurrentEWeapon();
                if (eWeapon) {
                    current_speedscale *= eWeapon->sprintSpeedScale;
                }
                FPU::FMUL(current_speedscale);
                });

        }

        //pattern = hook::pattern(handle,"83 F8 ? 75 ? ? ? ? ? ? ? ? ? ? ? E9 ? ? ? ? 83 F8");
        //if (!pattern.empty()) {
        //    CreateMidHook(pattern.get_first(), [](SafetyHookContext& ctx) {

        //        printf("speed %f\n", *(float*)(ctx.esp + 0x8));

        //        });
        //}

    }


    class component final : public component_interface
    {
    public:
        void post_unpack() override
        {
            if (sp_mp(1)) {
                cg_weaponBobAmplitudeSprinting_horz = Cevar_Get("cg_BobweaponAmplitudeSprinting_horz", 0.02f, 0, 0.f, 1.f);
                cg_weaponBobAmplitudeSprinting_vert = Cevar_Get("cg_BobweaponAmplitudeSprinting_vert", 0.014f, 0, 0.f, 1.f);

                cg_bobAmplitudeSprinting_horz = Cevar_Get("cg_bobAmplitudeSprinting_horz", 0.02f, 0, 0.f, 1.f);
                cg_bobAmplitudeSprinting_vert = Cevar_Get("cg_bobAmplitudeSprinting_vert", 0.014f, 0, 0.f, 1.f);

                cg_weaponSprint_mod = Cevar_Get("cg_weaponSprint_mod", 1, CVAR_ARCHIVE);

                player_sprintSpeedScale = Cevar_Get("player_sprintSpeedScale", 1.6f, NULL, 0.f, FLT_MAX);

                game::Cmd_AddCommand("reload_eweapons", loadEWeapons);

            }

        }

        void post_cgame() override
        {
            if (!sp_mp(1))
                return;
            loadEWeapons();

            SprintT4_lol((HMODULE)cg_game_offset);

            PatchSprintScale((HMODULE)cg_game_offset);

            Memory::VP::InjectHook(cg(0x3002C995), CG_GetViewGetHorizontalBobFactor_sprint);
            Memory::VP::InjectHook(cg(0x3002C9EA), CG_GetViewGetHorizontalBobFactor_sprint);

            Memory::VP::InjectHook(cg(0x3002C9CE), CG_GetViewVerticalBobFactor_sprint);
            Memory::VP::InjectHook(cg(0x3002C961), CG_GetViewVerticalBobFactor_sprint);


        }

        void post_game_sp() override
        {
            PatchSprintScale((HMODULE)game_offset);
        }

    };



}
REGISTER_COMPONENT(weapon::component);