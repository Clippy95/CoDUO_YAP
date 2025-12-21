// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
//#include "MinHook.h"
#include <windows.h>
#include <iostream>
#include "Hooking.Patterns.h"
#include "safetyhook.hpp"
//#pragma comment(lib, "libMinHook.x86.lib")
#include "shared.h"

#include "MemoryMgr.h"

#include <deque>

#include <set>
#include <algorithm>

#include "structs.h"

#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

#include "Hooking.Patterns.h"

#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define MAX_CVARS_OLD 1024

#define MAX_CVARS_NEW 2048

static std::vector<std::unique_ptr<SafetyHookInline>> g_inlineHooks;
static std::vector<std::unique_ptr<SafetyHookMid>> g_midHooks;



template<typename T, typename Fn>
SafetyHookInline* CreateInlineHook(T target, Fn destination, SafetyHookInline::Flags flags = SafetyHookInline::Default) {
    if (!target)
        return NULL;
    auto hook = std::make_unique<SafetyHookInline>(safetyhook::create_inline(target, destination, flags));
    auto* ptr = hook.get();
    g_inlineHooks.push_back(std::move(hook));
    return ptr;
}

template<typename T>
SafetyHookMid* CreateMidHook(T target, safetyhook::MidHookFn destination, safetyhook::MidHook::Flags flags = safetyhook::MidHook::Default) {
    if (!target)
        return NULL;
    auto hook = std::make_unique<SafetyHookMid>(safetyhook::create_mid(target, destination, flags));
    auto* ptr = hook.get();
    g_midHooks.push_back(std::move(hook));
    return ptr;
}


// cdecl
template<typename Ret, typename... Args>
inline Ret cdecl_call(uintptr_t addr, Args... args) {
    return reinterpret_cast<Ret(__cdecl*)(Args...)>(addr)(args...);
}

// stdcall
template<typename Ret, typename... Args>
inline Ret stdcall_call(uintptr_t addr, Args... args) {
    return reinterpret_cast<Ret(__stdcall*)(Args...)>(addr)(args...);
}

// fastcall
template<typename Ret, typename... Args>
inline Ret fastcall_call(uintptr_t addr, Args... args) {
    return reinterpret_cast<Ret(__fastcall*)(Args...)>(addr)(args...);
}

// thiscall
template<typename Ret, typename... Args>
inline Ret thiscall_call(uintptr_t addr, Args... args) {
    return reinterpret_cast<Ret(__thiscall*)(Args...)>(addr)(args...);
}

typedef cvar_t* (__cdecl* Cvar_GetT)(const char* var_name, const char* var_value, int flags);
Cvar_GetT Cvar_Get = (Cvar_GetT)NULL;

// SP Only

cvar_t* g_save_allowbadchecksum;

// cvars
cvar_t* cg_fovMin;
cvar_t* cg_fovscale;
cvar_t* cg_fovfixaspectratio;
cvar_t* cg_fixaspect;
cvar_t* safeArea_horizontal;
cvar_t* safeArea_vertical;
cvar_t* r_noborder;
cvar_t* r_mode_auto;
cvar_t* player_sprintmult;

cvar_t* hook_shortversion;

cvar_t* hook_version;

cvar_t* r_ext_texture_filter_anisotropic;


void codDLLhooks(HMODULE handle);

void ui_hooks(HMODULE handle);

unsigned int x_modded = 1920;

typedef HMODULE(__cdecl* LoadsDLLsT)(const char* a1, FARPROC* a2, int a3);
LoadsDLLsT originalLoadDLL = nullptr;

enum COD_GAME {
    UNSUPPORTED = -1,
    UO_SP,
    UO_MP,
    COD_MAX_GAMES,
};

struct COD_Classic_Version {
    DWORD WinMain_Check[2];
    const char* cgamename;
    const char* uixname;
    DWORD LoadDLLAddr;
    DWORD DLL_CG_GetViewFov_offset;
    DWORD Cvar_Get_Addr;
    DWORD X_res_Addr;
    DWORD GL_Ortho_ptr;
    DWORD gl_ortho_ret;
    DWORD Item_Paint;
    DWORD CG_DrawFlashImage_Draw;
    COD_GAME game;
};

COD_Classic_Version COD_UO_SP = {
{0x00455050,0x83EC8B55},
"uo_cgamex86.dll",
"uo_uix86.dll",
0x454440,
0x2CC20,
0x004337F0,
0x047BE104,
0x47BCF98,
0x004D7E02,
0x43EE0,
0x122BB,
UO_SP,
};

COD_Classic_Version COD_SP = {
{0x0046C5C0,0x83EC8B55},
"uo_cgame_mp_x86.dll",
"uo_mp_uix86.dll",
0x452190,
0x3FFC0,
0x43D9E0,
0x0489A0A4,
0x4898F38,
0x004BF2B2,
0x58250,
0x1A96B,
UO_MP,
};

COD_Classic_Version *LoadedGame = NULL;

uintptr_t cg_game_offset = 0;
uintptr_t ui_offset = 0;
uintptr_t game_offset = 0;

#define CGAME_OFF(x) (cg_game_offset + (x - 0x30000000))
#define GAME_OFF(x) (game_offset + (x - 0x20000000))

#define UI_OFF(x) (ui_offset + (x - 0x40000000))

SAFETYHOOK_NOINLINE uintptr_t cg(uintptr_t CODUOSP, uintptr_t CODUOMP = 0) {

    if (cg_game_offset == NULL)
        return NULL;

    uintptr_t result = 0;

    if (LoadedGame == &COD_SP) {
        result = CODUOMP;
    }
    else result = CODUOSP;

    if (result >= 0x30000000) {
        result = CGAME_OFF(result);
    }
    return result;

}

SAFETYHOOK_NOINLINE uintptr_t ui(uintptr_t CODUOSP, uintptr_t CODUOMP = 0) {

    if (ui_offset == NULL)
        return NULL;

    uintptr_t result = 0;

    if (LoadedGame == &COD_SP) {
        result = CODUOMP;
    }
    else result = CODUOSP;

    if (result >= 0x40000000) {
        result = UI_OFF(result);
    }
    return result;

}

SAFETYHOOK_NOINLINE uintptr_t g(uintptr_t CODUOSP, uintptr_t CODUOMP = 0) {

    if (ui_offset == NULL)
        return NULL;

    uintptr_t result = 0;

    if (LoadedGame == &COD_SP) {
        result = CODUOMP;
    }
    else result = CODUOSP;

    if (result >= 0x20000000) {
        result = GAME_OFF(result);
    }
    return result;

}



SAFETYHOOK_NOINLINE uintptr_t exe(uintptr_t CODUOSP, uintptr_t CODUOMP = 0) {

    if (!LoadedGame)
        return NULL;

    if (LoadedGame->game == UO_SP)
        return CODUOSP;

    if (LoadedGame->game == UO_MP)
        return CODUOMP;

    return NULL;

}


uintptr_t sp_mp(uintptr_t CODUOSP, uintptr_t CODUOMP = 0) {

    if (!LoadedGame)
        return NULL;

    if (LoadedGame->game == UO_SP)
        return CODUOSP;

    if (LoadedGame->game == UO_MP)
        return CODUOMP;

    return NULL;

}

cvar_s* __cdecl Cvar_Find(const char* a1) {
    return cdecl_call<cvar_s*>(0x00433700,a1);
}

void trap_R_SetColor(float* rgba) {
    auto ptr = cg(0x3002A750);
    if (ptr)
        cdecl_call<void>(ptr, rgba);
}

SafetyHookInline* CG_GetViewFov_og_S{};
constexpr auto STANDARD_ASPECT = 1.33333333333f;
void OpenConsole()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    std::cout << "Console initialized.\n";
    printf("Oh,Hi Mark\n");
}
float GetAspectRatio() {
    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    //printf("aspect ratio %f\n", x / y);

    return x / y;
}

double GetAspectRatio_standardfix() {
    return (GetAspectRatio() / STANDARD_ASPECT);
}

double CG_GetViewFov_hook() {
    double fov = CG_GetViewFov_og_S->call<double>();

    if (cg_fovMin && (cg_fovMin->value - fov) > 0.f) {
        fov = cg_fovMin->value;
    }

    if (cg_fovscale && cg_fovscale->value) {
        double halfFovRad = (fov / 2.0) * (M_PI / 180.0); // Convert to radians
        double tanHalfFov = tan(halfFovRad);


        if (cg_fovfixaspectratio && cg_fovfixaspectratio->integer) {
            // Convert horizontal FOV to vertical, then back to horizontal with new aspect ratio
            // Convert to vertical FOV (aspect-independent)
            double tanHalfVFov = tanHalfFov / STANDARD_ASPECT;
            // Apply fovscale to the tangent
            tanHalfVFov *= cg_fovscale->value;
            // Convert back to horizontal FOV with current aspect ratio
            double newTanHalfFov = tanHalfVFov * GetAspectRatio();
            // Convert back to degrees
            fov = 2.0 * atan(newTanHalfFov) * (180.0 / M_PI);
        }
        else {
            // Apply fovscale to the tangent directly
            tanHalfFov *= cg_fovscale->value;
            // Convert back to degrees
            fov = 2.0 * atan(tanHalfFov) * (180.0 / M_PI);
        }
    }
    return fov;
}

int __stdcall StringContains(const char* SubStr, const char* StringBuffer)
{
    unsigned int v2; // edi
    const char* Str_1; // esi

    v2 = strlen(SubStr);
    Str_1 = strstr(StringBuffer, SubStr);
    if (!Str_1)
        return 0;
    while (Str_1 != StringBuffer && !isspace(*(Str_1 - 1)) || !isspace(Str_1[v2]) && Str_1[v2])
    {
        Str_1 = strstr(Str_1 + 1, SubStr);
        if (!Str_1)
            return 0;
    }
    return 1;
}

void CheckModule()
{
    HMODULE hMod = GetModuleHandleA(LoadedGame->cgamename);
    if (hMod)
    {
        std::cout << "cgamex86.dll is attached at address: " << hMod << std::endl;
        codDLLhooks(hMod);
    }
    else
    {
        std::cout << "cgamex86.dll is NOT attached.\n";
    }
}


SafetyHookInline originalLoadDLLd;
HMODULE __cdecl hookCOD_dllLoad(const char* a1, FARPROC* a2, int a3) {
    HMODULE result = originalLoadDLLd.ccall<HMODULE>(a1, a2, a3);
    CheckModule();
   // printf("0x%X \n", (int)result);
    return result;
}

COD_Classic_Version* CheckGame() {
    // Get the WinMain address value for comparison
    DWORD* winMainAddr;
    DWORD winMainValue;

    // Check for COD_UO_SP
    winMainAddr = (DWORD*)COD_UO_SP.WinMain_Check[0];
    winMainValue = *winMainAddr;

    if (winMainValue == COD_UO_SP.WinMain_Check[1]) {
        LoadedGame = &COD_UO_SP;
        return &COD_UO_SP;
    }

    // Check for COD_SP
    winMainAddr = (DWORD*)COD_SP.WinMain_Check[0];
    winMainValue = *winMainAddr;

    if (winMainValue == COD_SP.WinMain_Check[1]) {
        LoadedGame = &COD_SP;
        return &COD_SP;
    }

    // If no match found, return NULL
    return NULL;
}

void SetUpFunctions() {
    Cvar_Get = (Cvar_GetT)LoadedGame->Cvar_Get_Addr;
}

int __stdcall glviewport_47BD978 (DWORD xo, DWORD yo , DWORD x, DWORD y) {

    return stdcall_call<int>(**(uintptr_t**)0x47BD978, 240, yo, x, y);

}


LPVOID GetModuleEndAddress(HMODULE hModule) {
    if (hModule == NULL) {
        return NULL;
    }


    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;


    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }


    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);


    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return NULL;
    }


    DWORD imageSize = pNtHeaders->OptionalHeader.SizeOfImage;


    return (LPVOID)((BYTE*)hModule + imageSize);
}

// For glOrtho - adjusts screen-space ortho projection
double process_widths(double width = 0) {
    if (cg_fixaspect && !cg_fixaspect->integer) {
        return 0.f;
    }
    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    // orthoWidth = (x² / y²) × (3/4) × y = (x² × 3) / (4y)
    float orthoWidth = (x * x * 3.0f) / (4.0f * y);

    width += (orthoWidth - x) / 2.0f;

    return width;
}

// For game functions - adjusts game's internal 480-based coordinate system
double process_width(double width = 0) {

    if (cg_fixaspect && !cg_fixaspect->integer) {
        return 0.f;
    }

    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    // Same formula but scaled to 480-height coordinate space
    float aspect = x / y;
    width += 480.0f * aspect - 640.0f;

    return width;
}

int process_width(int width) {
    return (int)process_width((double)0.f);
}

SafetyHookInline LoadLibraryD;
void game_hooks(HMODULE handle);
HMODULE __stdcall LoadLibraryHook(const char* filename) {

    auto hModule = LoadLibraryD.unsafe_stdcall<HMODULE>(filename);

    if (strstr(filename, LoadedGame->cgamename) != NULL) {
        codDLLhooks(hModule);
    }
    else if (strstr(filename, LoadedGame->uixname) != NULL) {
        ui_hooks(hModule);
    }
    else if (!strcmp(filename,"uo_gamex86.dll")) {
        game_hooks(hModule);
    }

    return hModule;

}


SafetyHookInline FreeLibraryD;

BOOL __stdcall FreeLibraryHook(HMODULE hLibModule) {
    char LibraryName[256]{};
    GetModuleFileNameA(hLibModule, LibraryName, sizeof(LibraryName));

    if (LoadedGame && LoadedGame->cgamename && (strcmp(LibraryName, LoadedGame->cgamename) == 0)) {
        cg_game_offset = 0;
    }

    uintptr_t moduleStart = (uintptr_t)hLibModule;
    uintptr_t moduleEnd = (uintptr_t)GetModuleEndAddress(hLibModule);

    if (moduleEnd != 0) {
        // Check inline hooks
        for (auto& hookPtr : g_inlineHooks) {
            if (hookPtr) {
                uintptr_t targetAddr = (uintptr_t)hookPtr->target_address();
                if (targetAddr >= moduleStart && targetAddr < moduleEnd) {
                    hookPtr->reset();
                }
            }
        }

        // Check mid hooks
        for (auto& hookPtr : g_midHooks) {
            if (hookPtr) {
                uintptr_t targetAddr = (uintptr_t)hookPtr->target_address();
                if (targetAddr >= moduleStart && targetAddr < moduleEnd) {
                    hookPtr->reset();
                }
            }
        }

        // Clear out any reset hooks
        g_inlineHooks.erase(
            std::remove_if(g_inlineHooks.begin(), g_inlineHooks.end(),
                [](const std::unique_ptr<SafetyHookInline>& h) { return !h || !(*h); }),
            g_inlineHooks.end()
        );

        g_midHooks.erase(
            std::remove_if(g_midHooks.begin(), g_midHooks.end(),
                [](const std::unique_ptr<SafetyHookMid>& h) { return !h || !(*h); }),
            g_midHooks.end()
        );
    }

    auto hModule = FreeLibraryD.unsafe_stdcall<BOOL>(hLibModule);
    return hModule;
}

float get_safeArea_horizontal() {
    if (cg_fixaspect && !cg_fixaspect->integer) {
        return 1.f;
    }
    float printing = safeArea_horizontal != 0 ? safeArea_horizontal->value : 1.f;

    //printf("SafeArea cvar ptr %p %f\n", safeArea_horizontal, printing);

    if (!safeArea_horizontal)
        return 1.f;
    return std::clamp(safeArea_horizontal->value, 0.f, 1.f);
}



float get_safeArea_vertical_hack() {
    if (cg_fixaspect && !cg_fixaspect->integer) {
        return 0.f;
    }
    if (!safeArea_vertical)
        return 0.f;

    // Clamp first, then invert
    float clamped = std::clamp(safeArea_vertical->value, 0.f, 1.f);
    return 1.f - clamped;
}

float process_height_hack_safe() {

    return 240.f * get_safeArea_vertical_hack();

}

SafetyHookInline glOrtho_og{};
//
//
//
struct OrthoParams {
    double left;
    double right;
    double bottom;
    double top;
    double zNear;
    double zFar;
};
//
//SafetyHookInline qglViewport_og{};
//
//void __stdcall qglViewport_detour(int x,
//    int y,
//    unsigned int width,
//    unsigned int height) {
//
//    if (_ReturnAddress() == (void*)0x4D7D96) {
//        x = 240;
//        width = 1440;
//    }
//    qglViewport_og.unsafe_stdcall(x, y, width, height);
//};
//
//
//SafetyHookInline qglScissor_og{};
//
//void __stdcall qglScisso_detour(int x,
//    int y,
//    unsigned int width,
//    unsigned int height) {
//
//    if (_ReturnAddress() == (void*)0x004D7DAD) {
//        x = 240;
//        width = 1440;
//    }
//    qglScissor_og.unsafe_stdcall(x, y, width, height);
//};

int __stdcall glOrtho_detour(double left, double right, double bottom, double top, double zNear, double zFar) {
    static OrthoParams params = { 0, 0, 0, 0, 0, 0 };

    //printf("glOrtho called from: %p\n", _ReturnAddress());
    //printf("  left   = %.2f (addr: %p)\n", params.left, &params.left);
    //printf("  right  = %.2f (addr: %p)\n", params.right, &params.right);
    //printf("  bottom = %.2f (addr: %p)\n", params.bottom, &params.bottom);
    //printf("  top    = %.2f (addr: %p)\n", params.top, &params.top);
    //printf("  zNear  = %.2f (addr: %p)\n", params.zNear, &params.zNear);
    //printf("  zFar   = %.2f (addr: %p)\n", params.zFar, &params.zFar);

    if (_ReturnAddress() == (void*)LoadedGame->gl_ortho_ret) {
        //printf("glOrtho called from: %p\n", _ReturnAddress());
        //printf("  left   = %.2f (addr: %p)\n", left, &left);
        //printf("  right  = %.2f (addr: %p)\n", right, &right);
        //printf("  bottom = %.2f (addr: %p)\n", bottom, &bottom);
        //printf("  top    = %.2f (addr: %p)\n", top, &top);
        //printf("  zNear  = %.2f (addr: %p)\n", zNear, &zNear);
        //printf("  zFar   = %.2f (addr: %p)\n", zFar, &zFar);
        static float floatsy[2];
        //left -= process_width(0.f) + floatsy[0];
        //right += process_width(0.f) + floatsy[1];

        left -= process_widths() + floatsy[0];
        right += process_widths() + floatsy[1];

        /*printf("left %f right %f floatsy %p\n",left,right, floatsy);*/

        return glOrtho_og.unsafe_stdcall<int>(
            left + params.left,
            right + params.right,
            bottom + params.bottom,
            top + params.top,
            zNear + params.zNear,
            zFar + params.zFar
        );
    }

    return glOrtho_og.unsafe_stdcall<int>(left, right, bottom, top, zNear, zFar);
}

uintptr_t InsideWinMain;

uintptr_t cvar_init_og;

int Cvar_Init_hook() {
    r_ext_texture_filter_anisotropic = Cvar_Get("r_ext_texture_filter_anisotropic", "1", CVAR_ARCHIVE | CVAR_LATCH);
    auto result = cdecl_call<int>(cvar_init_og);

    int& size_cvars = *(int*)0x4805EC0;
    printf("cvar_init cvar hooks cvar_get ptr %p size %d\n", Cvar_Get, size_cvars);
    cg_fovMin = Cvar_Get((char*)"cg_fovMin", "1.0", CVAR_ARCHIVE);
    cg_fovscale = Cvar_Get((char*)"cg_fovscale", "1.0", CVAR_ARCHIVE);
    cg_fovfixaspectratio = Cvar_Get((char*)"cg_fixaspectFOV", "1", CVAR_ARCHIVE);
    cg_fixaspect = Cvar_Get((char*)"cg_fixaspect", "1", CVAR_ARCHIVE);
    safeArea_horizontal = Cvar_Get((char*)"safeArea_horizontal", "1.0", CVAR_ARCHIVE);
    safeArea_vertical = Cvar_Get((char*)"safeArea_vertical", "1.0", CVAR_ARCHIVE);
    printf("safearea ptr return %p size after %d\n", safeArea_horizontal, size_cvars);
    r_noborder = Cvar_Get((char*)"r_noborder", "0", CVAR_ARCHIVE);
    r_mode_auto = Cvar_Get((char*)"r_mode_auto", "0", CVAR_ARCHIVE);
    player_sprintmult = Cvar_Get("player_sprintmult", "0.66666669", CVAR_CHEAT);

    if (sp_mp(1)) {
        g_save_allowbadchecksum = Cvar_Get("g_save_allowbadchecksum", "0", 1);
    }

    return result;
}

void sub_431CA0(SafetyHookContext& ctx) {



}
void LoadMenuConfigs();



SafetyHookMid* DrawHudElemMaterial_mid{};

uintptr_t DrawStretch_og{};

int __cdecl DrawStretch_ui(float x, float y, float width, float height, float* a5) {

    int result = 0;
    x -= process_width();
    width += process_width() * 2.0f;
    auto ptr = ui(0x40007960);
    if (ptr) {
        result = cdecl_call<int>(ptr, x, y, width, height, a5);
    }

    return result;
}

int __cdecl DrawStretch_stretched(float x, float y, float width, float height, float* a5) {

    int result;
    x -= process_width();
    width += process_width() * 2.0f;

    if (DrawStretch_og) {
        result = cdecl_call<int>(DrawStretch_og, x, y, width, height, a5);
    }

        return result;
}

int __cdecl DrawStretch_300135B0(float x, float y, float width, float height, int a5) {

    x -= process_width();
    width += process_width() * 2.0f;

    //printf("this is called \n");

    return cdecl_call<int>(DrawStretch_og, x, y, width, height, a5);
}

uintptr_t trap_R_DrawStretchPic{};

float whatevers[4]{};


void __cdecl R_DrawStretchPic_leftsniper(float x, float y, float width, float height, float unk1, float unk2, float unk3, float unk4, int image_id) {


    x = -process_widths(0.f);
    width += process_widths(0.f);
    if(trap_R_DrawStretchPic)
    cdecl_call<void>(trap_R_DrawStretchPic, x, y, width, height, unk1, unk2, unk3, unk4, image_id);
}

void __cdecl R_DrawStretchPic_rightsniper(float x, float y, float width, float height, float unk1, float unk2, float unk3, float unk4, int image_id) {




    width += process_widths(0.f);

    cdecl_call<void>(trap_R_DrawStretchPic, x, y, width, height, unk1, unk2, unk3, unk4, image_id);
}

uintptr_t crosshair_render_func;

uintptr_t CG_DrawPicAddr;

int __cdecl CG_DrawPic(float a1, float a2, float a3, float a4, int a5) {
    if (CG_DrawPicAddr) {
        return cdecl_call<int>(CG_DrawPicAddr, a1, a2, a3, a4, a5);
    }
    return -1;
}
static float adjustments[10];

void _cdecl crosshair_render_hook(float x, float y, float width, float height, int unk1, float u1, float u2, float v1, float rotation, int shaderHandle) {
    int side;
    __asm mov side, esi

    float aspect = GetAspectRatio();
    float aspectRatio = aspect / (4.0f / 3.0f);

    static float horizontal_width_mult = 1.0f;
    static float horizontal_height_mult = 1.0f;
    static float vertical_width_mult = 1.0f;
    static float vertical_height_mult = 1.0f;
    static float x_offset_mult = 0.0f;  // Try 0 first (no centering)
    static float y_offset_mult = 0.0f;

    float orig_width = width;
    float orig_height = height;

    bool is_horizontal = (side == 0 || side == 2);

    if (is_horizontal) {
        width *= aspectRatio * horizontal_width_mult;
        height *= aspectRatio * horizontal_height_mult;
    }
    else {
        width *= aspectRatio * vertical_width_mult;
        height *= aspectRatio * vertical_height_mult;
    }

    x -= ((width - orig_width) / 2.0f) * x_offset_mult;
    y -= ((height - orig_height) / 2.0f) * y_offset_mult;

    //printf("side=%d, x_off_mult=%.3f (%p), y_off_mult=%.3f (%p)\n",
    //    side, x_offset_mult, &x_offset_mult, y_offset_mult, &y_offset_mult);

    cdecl_call<void>(crosshair_render_func, x, y, width, height, unk1, u1, u2, v1, rotation, shaderHandle);
}

SafetyHookInline* Item_Paint_cg{};

SafetyHookInline* Item_Paint_ui{};

#define ALIGN_LEFT_BOTTOM   "[LB]"
#define ALIGN_LEFT_TOP      "[LT]"
#define ALIGN_RIGHT_BOTTOM  "[RB]"
#define ALIGN_RIGHT_TOP     "[RT]"
#define ALIGN_CENTER        "[CENTER]"
#define ALIGN_LEFT          "[LEFT]"
#define ALIGN_RIGHT         "[RIGHT]"
#define ALIGN_TOP           "[TOP]"
#define ALIGN_BOTTOM        "[BOTTOM]"
#define ALIGN_STRETCH       "[STRETCH]"
#define ALIGN_BLACKSCREEN   "[BLACKSCREEN]"

// ============================================================================
// ALIGNMENT PROCESSING FUNCTION
// ============================================================================
struct AlignmentState {
    const char* originalText;
    rectDef_t originalRect;
    bool wasModified;
};

bool ProcessItemAlignment(itemDef_t* item, char* textBuffer, size_t bufferSize, AlignmentState* state) {
    if (!item->text) return false;

    const char* text = item->text;
    state->originalText = text;
    state->originalRect = item->window.rect;
    state->wasModified = false;

    float halfWidth = process_width() * 0.5f;
    float safeX = get_safeArea_horizontal();

    // Helper macro to check and process alignment
#define CHECK_ALIGNMENT(marker, xOffset) \
        if (strncmp(text, marker, strlen(marker)) == 0) { \
            const char* remainingText = text + strlen(marker); \
            if (remainingText[0] != '\0') { \
                strcpy_s(textBuffer, bufferSize, remainingText); \
                item->text = textBuffer; \
            } else { \
                item->text = nullptr; \
            } \
            item->window.rect.x += (xOffset); \
            state->wasModified = true; \
            return true; \
        }

    // Check each alignment marker
    CHECK_ALIGNMENT(ALIGN_LEFT_BOTTOM, (-halfWidth) * safeX)
        CHECK_ALIGNMENT(ALIGN_LEFT_TOP, (-halfWidth) * safeX)
        CHECK_ALIGNMENT(ALIGN_RIGHT_BOTTOM, (halfWidth)*safeX)
        CHECK_ALIGNMENT(ALIGN_RIGHT_TOP, (halfWidth)*safeX)
        CHECK_ALIGNMENT(ALIGN_LEFT, (-halfWidth) * safeX)
        CHECK_ALIGNMENT(ALIGN_RIGHT, (halfWidth)*safeX)
        CHECK_ALIGNMENT(ALIGN_CENTER, 0.0f)
        CHECK_ALIGNMENT(ALIGN_TOP, 0.0f)
        CHECK_ALIGNMENT(ALIGN_BOTTOM, 0.0f)

#undef CHECK_ALIGNMENT

        return state->wasModified;
}

void RestoreItemState(itemDef_t* item, const AlignmentState* state) {
    if (state->wasModified) {
        item->text = state->originalText;
        item->window.rect = state->originalRect;
    }
}


// ============================================================================
// JSON CONFIGURATION STRUCTURE
// ============================================================================
struct Alignment {
    uint32_t h_left : 1;
    uint32_t h_right : 1;
    uint32_t h_center : 1;
    uint32_t v_top : 1;
    uint32_t v_bottom : 1;
    uint32_t v_center : 1;
    uint32_t stretch : 1;   // For stretch behavior
    uint32_t black_screen : 1;
    uint32_t _unused : 24;  // Padding to 32 bits
};

struct MenuConfig {
    std::string menuName;
    Alignment alignment;
};

// Global storage for all menu configs
std::vector<MenuConfig> g_menuConfigs;

// ============================================================================
// JSON PARSING - SUPPORTS MULTIPLE MENUS PER FILE
// ============================================================================

Alignment ParseAlignment(const std::string& alignStr) {
    Alignment align = { 0 };

    if (alignStr == ALIGN_LEFT) {
        align.h_left = 1;
    }
    else if (alignStr == ALIGN_RIGHT) {
        align.h_right = 1;
    }
    else if (alignStr == ALIGN_CENTER) {
        align.h_center = 1;
        align.v_center = 1;
    }
    else if (alignStr == ALIGN_LEFT_BOTTOM) {
        align.h_left = 1;
        align.v_bottom = 1;
    }
    else if (alignStr == ALIGN_LEFT_TOP) {
        align.h_left = 1;
        align.v_top = 1;
    }
    else if (alignStr == ALIGN_RIGHT_BOTTOM) {
        align.h_right = 1;
        align.v_bottom = 1;
    }
    else if (alignStr == ALIGN_RIGHT_TOP) {
        align.h_right = 1;
        align.v_top = 1;
    }
    else if (alignStr == ALIGN_TOP) {
        align.v_top = 1;
    }
    else if (alignStr == ALIGN_BOTTOM) {
        align.v_bottom = 1;
    }
    else if (alignStr == ALIGN_STRETCH) {
        align.stretch = 1;
    }
    else if (alignStr == ALIGN_BLACKSCREEN) {
        align.black_screen = 1;
    }

    return align;
}

void LoadMenuConfigs() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);

    std::filesystem::path exePath(modulePath);
    std::filesystem::path menuwideDir = exePath.parent_path() / "menuwide";

    if (!std::filesystem::exists(menuwideDir)) {
        printf("menuwide directory not found: %s\n", menuwideDir.string().c_str());
        return;
    }

    printf("Loading menu configs from: %s\n", menuwideDir.string().c_str());

    for (const auto& entry : std::filesystem::directory_iterator(menuwideDir)) {
        if (entry.path().extension() != ".json") continue;

        if (entry.path().filename() == "_hudelem_shaders.json") {
            printf("Skipping _hudelem_shaders.json (handled separately)\n");
            continue;
        }

        try {
            std::ifstream file(entry.path());
            nlohmann::json j = nlohmann::json::parse(file);

            if (j.is_array()) {
                for (const auto& menuJson : j) {
                    MenuConfig config;
                    config.menuName = menuJson["menuName"].get<std::string>();
                    config.alignment = ParseAlignment(menuJson["alignment"].get<std::string>());

                    g_menuConfigs.push_back(config);
                    printf("Loaded config for menu '%s'\n", config.menuName.c_str());
                }
            }
            else {
                MenuConfig config;
                config.menuName = j["menuName"].get<std::string>();
                config.alignment = ParseAlignment(j["alignment"].get<std::string>());

                g_menuConfigs.push_back(config);
                printf("Loaded config for menu '%s'\n", config.menuName.c_str());
            }
        }
        catch (const std::exception& e) {
            printf("Failed to parse %s: %s\n",
                entry.path().string().c_str(), e.what());
        }
    }
}

// ============================================================================
// LOOKUP FUNCTIONS
// ============================================================================
const MenuConfig* FindMenuConfig(const char* menuName) {
    if (!menuName) return nullptr;

    for (const auto& config : g_menuConfigs) {
        if (config.menuName == menuName) {
            return &config;
        }
    }

    return nullptr;
}

// ============================================================================
// ALIGNMENT PROCESSING - JSON VERSION
// ============================================================================
bool ProcessItemAlignment_FromJSON(itemDef_t* item, const char* menuName,
    AlignmentState* state) {
    if (!menuName) return false;

    state->originalText = item->text;
    state->originalRect = item->window.rect;
    state->wasModified = false;

    const MenuConfig* config = FindMenuConfig(menuName);
    if (!config) return false;

    float halfWidth = process_width() * 0.5f;
    float safeX = get_safeArea_horizontal();

    float height_hack = process_height_hack_safe() * 0.5f;


    if (config->alignment.h_left) {
        item->window.rect.x += (-halfWidth) * safeX;
        state->wasModified = true;
    }
    else if (config->alignment.h_right) {
        item->window.rect.x += (halfWidth)*safeX;
        state->wasModified = true;
    }
    else if (config->alignment.h_center) {
        state->wasModified = false;
    }

    if (config->alignment.v_top) {
         item->window.rect.y += height_hack;
        state->wasModified = true;
    }
    else if (config->alignment.v_bottom) {
        item->window.rect.y -= height_hack;
        state->wasModified = true;
    }
    else if (config->alignment.v_center) {
        // No Y adjustment for center
        state->wasModified = false;
    }

    return state->wasModified;
}


void __fastcall Item_Paint_Hook(itemDef_t* item, SafetyHookInline* hook, bool ui = false) {
    char textBuffer[1024];
    AlignmentState state = { 0 };

    const char* menuName = nullptr;
    menuDef_t* menu = nullptr;
    // Get menu name
    auto parent_ptr = sp_mp((uintptr_t)item->parent, item->textSavegameInfo);
    if (parent_ptr) {
        menu = (menuDef_t*)parent_ptr;
        if (menu && menu->window.name) {
            menuName = menu->window.name;
            //printf("menuName %s\n", menuName);
        }
    }

    // Try JSON-based alignment first (applies to entire menu)
    bool processedFromJSON = ProcessItemAlignment_FromJSON(item, menuName, &state);

    // If not found in JSON, fall back to text marker processing (per-item)
    if (!processedFromJSON) {
        ProcessItemAlignment(item, textBuffer, sizeof(textBuffer), &state);
    }


    auto config = FindMenuConfig(menuName);


    if (item && item->window.name) {
        //printf("item %s\n", item->window.name);
    }

    if (config && menu && menu->items && menu->items[0] == item || item && item->window.name && (strcmp(item->window.name,"main_back_top") == 0) ) {
        if (config && config->alignment.black_screen || item && item->window.name && (strcmp(item->window.name, "main_back_top") == 0)) {
            static float black[4] = { 0.f,0.f,0.f,1.f };
            ui ? DrawStretch_ui(0.f, 0.f, 640.f, 480.f, black) : DrawStretch_stretched(0.f, 0.f, 640.f, 480.f, black);
        }
    }
    // Call original render
    hook->unsafe_thiscall(item);



    // Restore original state
    RestoreItemState(item, &state);
}

void __fastcall Item_Paint_cg_f(itemDef_t* item) {
    Item_Paint_Hook(item, Item_Paint_cg,false);
}

void __fastcall Item_Paint_ui_f(itemDef_t* item) {
    Item_Paint_Hook(item, Item_Paint_ui,true);
}

uintptr_t DrawSingleHudElem2dog = 0;

struct HudAlignmentState {
    alignx_e originalAlignX;
    aligny_e originalAlignY;
    float originalX;
    float originalY;
    bool wasModified;
};

void ProcessHudElemAlignment(hudelem_s* hud, HudAlignmentState* state) {
    if (!hud) return;

    // Save original state
    state->originalAlignX = hud->alignx;
    state->originalAlignY = hud->aligny;
    state->originalX = (float)hud->x;
    state->originalY = (float)hud->y;
    state->wasModified = false;

    float halfWidth = process_width() * 0.5f;
    float safeX = get_safeArea_horizontal();
    // float halfHeight = process_height() * 0.5f;
    float height_hack = process_height_hack_safe() * 0.5f;

    // Process horizontal alignment (alignx)
    switch (hud->alignx) {
    case ALIGNX_LEFT:
        hud->x += (int32_t)((-halfWidth) * safeX);
        state->wasModified = true;
        break;

    case ALIGNX_RIGHT:
        hud->x += (int32_t)((halfWidth)*safeX);
        state->wasModified = true;
        break;

    case ALIGNX_CENTER:
        // No X adjustment for center
        state->wasModified = false;
        break;
    }
    // required otherwise will break "black" shader
    if ((hud->width < 640 && hud->height < 480)) {
        switch (hud->aligny) {
        case ALIGNY_TOP:
            hud->y += height_hack;
            state->wasModified = true;
            break;

        case ALIGNY_BOTTOM:
            hud->y -= height_hack;
            state->wasModified = true;
            break;

        case ALIGNY_MIDDLE:

            state->wasModified = false;
            break;
        }
    }
}

void RestoreHudElemState(hudelem_s* hud, const HudAlignmentState* state) {
    if (state->wasModified) {
        hud->alignx = state->originalAlignX;
        hud->aligny = state->originalAlignY;
        hud->x = (int32_t)state->originalX;
        hud->y = (int32_t)state->originalY;
    }
}

// ============================================================================
// UPDATED HOOK FUNCTION
// ============================================================================

int __fastcall DrawSingleHudElem2dHook(hudelem_s* thisa) {
    HudAlignmentState state = { };

    if (thisa) {
        //printf("type? %d x %d y %d alignx %d aligny %d\n",
        //    thisa->type, thisa->x, thisa->y,
        //    thisa->alignx, thisa->aligny);

        // Process alignment adjustments
        ProcessHudElemAlignment(thisa, &state);
    }

    // Call original render
    auto result = thiscall_call<int>(DrawSingleHudElem2dog, thisa);

    // Restore original state
    if (thisa) {
        RestoreHudElemState(thisa, &state);
    }

    return result;
}

struct HudShaderConfig {
    std::string shaderName;
    Alignment alignment;
};

// Global storage for shader configs
std::vector<HudShaderConfig> g_hudShaderConfigs;

void LoadHudShaderConfigs() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);

    std::filesystem::path exePath(modulePath);
    std::filesystem::path configPath = exePath.parent_path() / "menuwide" / "_hudelem_shaders.json";

    if (!std::filesystem::exists(configPath)) {
        printf("_hudelem_shaders.json not found: %s\n", configPath.string().c_str());
        return;
    }

    printf("Loading HUD shader configs from: %s\n", configPath.string().c_str());

    try {
        std::ifstream file(configPath);
        nlohmann::json j = nlohmann::json::parse(file);

        if (j.is_array()) {
            for (const auto& shaderJson : j) {
                HudShaderConfig config;
                config.shaderName = shaderJson["shaderName"].get<std::string>();
                config.alignment = ParseAlignment(shaderJson["alignment"].get<std::string>());

                g_hudShaderConfigs.push_back(config);
                printf("Loaded shader config for '%s' (stretch: %d)\n",
                    config.shaderName.c_str(), config.alignment.stretch);
            }
        }
    }
    catch (const std::exception& e) {
        printf("Failed to parse _hudelem_shaders.json: %s\n", e.what());
    }
}


const HudShaderConfig* FindHudShaderConfig(const char* shaderName) {
    if (!shaderName) return nullptr;

    for (const auto& config : g_hudShaderConfigs) {
        if (config.shaderName == shaderName) {
            return &config;
        }
    }

    return nullptr;
}
SafetyHookMid* DrawObjectives;

SafetyHookMid* DrawMissionObjectives;

void StaticInstructionPatches(cvar_s* safeArea_horizontal_ptr = safeArea_horizontal, bool DLLLoad = true) {

    auto safe_x = get_safeArea_horizontal();

    auto width_x = process_width() * 0.5f;

    auto x = width_x * safe_x;
    auto menuConfig = FindMenuConfig("Compass");
    if (cg(0x30026DC6) && menuConfig && menuConfig->alignment.h_left) {
        Memory::VP::Patch<float>(cg(0x30026DC6) + 0x1, 21.f - x);
        Memory::VP::Patch<float>(cg(0x30026DDC) + 0x1, 20.f - x);
        Memory::VP::Patch<float>(cg(0x30026F8D) + 0x4, 20.f - x);
        Memory::VP::Patch<float>(cg(0x30026E17) + 0x4, 20.f - x);

        if (DLLLoad) {
            CreateMidHook(cg(0x30026F36), [](SafetyHookContext& ctx) {
                float& checkbox_x = *(float*)(ctx.esp);

                auto menuConfig = FindMenuConfig("Compass");
                if (menuConfig && menuConfig->alignment.h_left) {
                    // Get actual screen width from scale
                    float scale = *(float*)cg(0x3024B740);
                    float screen_width = scale * 640.0f;
                    float offset = (screen_width / 2.0f) - 640.0f;
                    offset *= get_safeArea_horizontal();


                    //printf("scale=%.2f, screen_width=%.2f, offset=%.2f, checkbox_x before=%.2f, after=%.2f\n",
                    //    scale, screen_width, offset, checkbox_x, checkbox_x - offset);

                    checkbox_x -= process_widths() * get_safeArea_horizontal();
                }
                });
        }

    }

}
const char* UNKNOWN = "BADSTR";
const char* __cdecl String_Alloc_ui(const char* string_to_alloc) {
    auto ptr = ui(0x4000D400);

    if (ptr) {
        return cdecl_call<const char*>(ptr, string_to_alloc);
    }
    return UNKNOWN;
}



typedef struct vidmode_s
{
    const char* description;
    int width, height;
    float pixelAspect;
} vidmode_t;

struct vidmode_menu
{
    const char* description;
    int r_mode_setting;
};

struct DisplayModeKey {
    int width;
    int height;
    int bpp;
    int freq;

    bool operator<(const DisplayModeKey& other) const {
        if (width != other.width) return width > other.width;
        if (height != other.height) return height > other.height;
        if (bpp != other.bpp) return bpp > other.bpp;
        return freq > other.freq;
    }
};

struct ResolutionKey {
    int width;
    int height;

    bool operator<(const ResolutionKey& other) const {
        if (width != other.width) return width > other.width; // Descending
        return height > other.height;
    }

    bool operator==(const ResolutionKey& other) const {
        return width == other.width && height == other.height;
    }
};

std::vector<vidmode_t> r_vidModes_dynamic;
std::vector<vidmode_menu> r_vidModes_menu_dynamic;
std::vector<std::string> mode_descriptions;
std::vector<std::string> menu_descriptions;

std::vector<char*> allocated_strings;

char* AllocateString(const char* str) {
    size_t len = strlen(str) + 1;
    char* newStr = new char[len];
    strcpy(newStr, str);
    allocated_strings.push_back(newStr);
    return newStr;
}

void InitializeDisplayModes() {
    DEVMODE devMode;
    int modeNum = 0;
    std::set<ResolutionKey> uniqueResolutions;

    memset(&devMode, 0, sizeof(DEVMODE));
    devMode.dmSize = sizeof(DEVMODE);

    // Enumerate all display modes
    while (EnumDisplaySettings(NULL, modeNum, &devMode)) {
        if (devMode.dmPelsWidth >= 640 &&
            devMode.dmPelsHeight >= 480 &&
            devMode.dmBitsPerPel >= 16) {

            ResolutionKey key = {
                (int)devMode.dmPelsWidth,
                (int)devMode.dmPelsHeight
            };
            uniqueResolutions.insert(key);
        }
        modeNum++;
    }

    // Convert to r_vidModes array (with "Mode X:" prefix)
    int modeIndex = 0;
    for (const auto& res : uniqueResolutions) {
        char desc[64];
        float aspect = (float)res.width / (float)res.height;
        bool isWide = (aspect > 1.5f);

        if (isWide) {
            sprintf(desc, "Mode %2d: %dx%d (wide)", modeIndex, res.width, res.height);
        }
        else {
            sprintf(desc, "Mode %2d: %dx%d", modeIndex, res.width, res.height);
        }

        vidmode_t vidMode = {
            AllocateString(desc),
            res.width,
            res.height,
            1.0f
        };
        r_vidModes_dynamic.push_back(vidMode);
        modeIndex++;
    }

    // Convert to menu array (just "WxH")

    int menuCount = min((int)uniqueResolutions.size(), MAX_MULTI_CVARS);

    auto it = uniqueResolutions.begin();
    for (int i = 0; i < menuCount; i++, ++it) {
        char menuDesc[32];
        sprintf(menuDesc, "%dx%d", it->width, it->height);

        vidmode_menu menuMode = {
            AllocateString(menuDesc),
            i
        };
        r_vidModes_menu_dynamic.push_back(menuMode);
    }

    printf("Initialized %d unique display modes (%d available for menu)\n",
        (int)r_vidModes_dynamic.size(), menuCount);

    // Debug print
    for (int i = 0; i < min(5, (int)r_vidModes_menu_dynamic.size()); i++) {
        printf("Menu[%d]: %s -> Mode %d\n", i,
            r_vidModes_menu_dynamic[i].description,
            r_vidModes_menu_dynamic[i].r_mode_setting);



        vidmode_t* r_vidModes = nullptr;
        vidmode_menu* r_vidModes_menu = nullptr;
        int r_vidModes_count = 0;
        int r_vidModes_menu_count = 0;
    }
}
vidmode_t* r_vidModes = nullptr;
vidmode_menu* r_vidModes_menu = nullptr;
int r_vidModes_count = 0;
int r_vidModes_menu_count = 0;

void InitializeDisplayModesForGame() {
    if (!sp_mp(1))
        return;
    InitializeDisplayModes();

    r_vidModes_count = r_vidModes_dynamic.size();
    r_vidModes = new vidmode_t[r_vidModes_count];
    memcpy(r_vidModes, r_vidModes_dynamic.data(), r_vidModes_count * sizeof(vidmode_t));

    r_vidModes_menu_count = r_vidModes_menu_dynamic.size();
    r_vidModes_menu = new vidmode_menu[r_vidModes_menu_count];
    memcpy(r_vidModes_menu, r_vidModes_menu_dynamic.data(), r_vidModes_menu_count * sizeof(vidmode_menu));

    auto r_mode_read = exe(0x425F9E + 2,0x4C177B + 2);
    if (r_mode_read) {
        Memory::VP::Patch<void*>(r_mode_read, r_vidModes);
        Memory::VP::Patch<void*>(exe(0x425FC4 + 2,0x4C17BD + 2), r_vidModes);
        if (sp_mp(1)) {
            Memory::VP::Patch<void*>(exe(0x4AA35B + 1), r_vidModes);
            Memory::VP::Patch<void*>(exe(0x4AA39D + 1), r_vidModes);
        }
        *(int*)exe(0x0057B578,0x5CE908) = r_vidModes_count;
    }
}

void ui_hooks(HMODULE handle) {
    if (sp_mp(1)) {
        auto cvar_version = Cvar_Find("shortversion");

        if (cvar_version && cvar_version->string) {
            char buffer[128]{};
            sprintf_s(buffer, sizeof(buffer), "%s CCH r%d %s", cvar_version->string, BUILD_NUMBER, COMMIT_HASH);
            hook_shortversion = Cvar_Get("hook_shortversion", buffer, CVAR_ROM);
        }

        uintptr_t OFFSET = (uintptr_t)handle;
        ui_offset = OFFSET;
        Item_Paint_ui = CreateInlineHook(ui(0x40015400), Item_Paint_ui_f);
        CreateMidHook(ui(0x40017330), [](SafetyHookContext& ctx) {
            itemDef_s* item = *(itemDef_s**)(ctx.esp + 0x42C);
            if (item) {
                if (item->cvar && !strcmp("ui_r_mode", item->cvar)) {
                    printf("cvar %s\n", item->cvar);
                    multiDef_t* multiPtr = (multiDef_t*)sp_mp((uintptr_t)item->typeData, item->cursorPos);
                    if (multiPtr) {
                        multiPtr->count = 0;
                        // Use the menu-specific array (already limited to MAX_MULTI_CVARS)
                        for (int i = 0; i < r_vidModes_menu_count; i++) {
                            multiPtr->cvarList[multiPtr->count] = String_Alloc_ui(r_vidModes_menu[i].description);
                            multiPtr->cvarValue[multiPtr->count] = r_vidModes_menu[i].r_mode_setting;
                            multiPtr->count++;

                            if (multiPtr->count >= MAX_MULTI_CVARS)
                                break;

                        }
                    }


                }

                if (hook_shortversion && item->cvar && !strcmp("shortversion", item->cvar)) {

                    item->cvar = String_Alloc_ui("hook_shortversion");

                }

            }
            });
    }
}

void game_hooks(HMODULE handle) {
    if(!sp_mp(1))
        return;
    uintptr_t OFFSET = (uintptr_t)handle;
    game_offset = OFFSET;
    if(g_save_allowbadchecksum && g_save_allowbadchecksum->integer)
    Memory::VP::Nop(g(0x200306C8), 5);

    if (player_sprintmult) {
        auto pattern = hook::pattern(handle, "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 7A ? 8B 41 ? 83 E0 ? C7 44 81");
        if (!pattern.empty())
            Memory::VP::Patch<float*>(pattern.get_first(2), &player_sprintmult->value);

    }

}

SafetyHookInline* SCR_DrawString_hook_possible1_detour;

int __cdecl SCR_DrawString_hook_possible1(float x, float y, float width, float height) {
    if (x >= 0 && x <= 120) {

        x -= (process_width() * get_safeArea_horizontal());

    }


    if (x >= 300 && x <= 640) {
        x += (process_width() * get_safeArea_horizontal());
    }

    return SCR_DrawString_hook_possible1_detour->unsafe_ccall<int>(x, y, width, height);
}


void codDLLhooks(HMODULE handle) {
    // printf("run");
    uintptr_t OFFSET = (uintptr_t)handle;
    cg_game_offset = OFFSET;
    StaticInstructionPatches();
    Memory::VP::InterceptCall(OFFSET + LoadedGame->CG_DrawFlashImage_Draw, DrawStretch_og, DrawStretch_300135B0);
    Memory::VP::InterceptCall(cg(0x3001221A, 0x3001A8CF), DrawStretch_og, DrawStretch_300135B0);

    unsigned int* res = (unsigned int*)LoadedGame->X_res_Addr;

    x_modded = res[0] + (res[0] - (res[1] * GetAspectRatio()));

    auto pat = hook::pattern(handle, "83 EC ? 8B 44 24 ? ? ? ? ? ? ? ? ? ? ? 8B 4C 24 ? 6A 00");

    if (!pat.empty()) {
        //SCR_DrawString_hook_possible1_detour = CreateInlineHook(pat.get_first(), SCR_DrawString_hook_possible1_detour);
    }

    if (sp_mp(0, 0)) {

    }

    Memory::VP::InterceptCall(cg(0x30011F68, 0x3001A49B), crosshair_render_func, crosshair_render_hook);

    Memory::VP::InterceptCall(cg(0x30011222, 0x30019752), trap_R_DrawStretchPic, R_DrawStretchPic_leftsniper);

    Memory::VP::InterceptCall(cg(0x30011270, 0x300197A0), trap_R_DrawStretchPic, R_DrawStretchPic_rightsniper);

    Memory::VP::Nop(cg(0x30011205, 0x30019735), 2);

    Memory::VP::Nop(cg(0x30011247, 0x30019777), 2);

    CG_DrawPicAddr = cg(0x30013B70, 0x3001CAA0);

    CG_GetViewFov_og_S = CreateInlineHook(OFFSET + LoadedGame->DLL_CG_GetViewFov_offset, &CG_GetViewFov_hook);
    //if (MH_CreateHook((void**)OFFSET + 0x2CC20, &CG_GetViewFov_hook, (void**)&CG_GetViewFov_og) != MH_OK) {
    //    MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}

// CG_DrawWeaponSelect
    CreateMidHook(cg(0x30033A41, 0x30046C5B), [](SafetyHookContext& ctx) {
        auto config = FindMenuConfig("weaponinfo");
        if (config && config->alignment.h_right) {
            *(float*)(ctx.esp + 0x10) += ((process_width() * 0.5f) * get_safeArea_horizontal());
        }

        });

    Item_Paint_cg = CreateInlineHook(OFFSET + LoadedGame->Item_Paint, Item_Paint_cg_f);

    pat = hook::pattern(handle, "50 51 6A ? FF 15 ? ? ? ? 83 C4 ? 83 C4 ? C3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 EC");
    if (!pat.empty()) {
        DrawObjectives = CreateMidHook(pat.get_first(), [](SafetyHookContext& ctx) {
            auto menuConfig = FindMenuConfig("Compass");
            if (menuConfig) {
                if (menuConfig->alignment.h_left) {
                    float halfWidth = process_width() * 0.5f;
                    float safeX = get_safeArea_horizontal();

                    ctx.ecx -= halfWidth * get_safeArea_horizontal();
                }

                if (menuConfig->alignment.v_bottom) {
                    ctx.eax -= (int)(process_height_hack_safe() * 0.5f);
                }

            }

            });
    }

    auto SingleHudElem_ptr = cg(0x3001FB14, 0x3002A4C4);
    if (SingleHudElem_ptr) {
        Memory::VP::InterceptCall(SingleHudElem_ptr, DrawSingleHudElem2dog, DrawSingleHudElem2dHook);
    }

    DrawHudElemMaterial_mid = CreateMidHook(cg(0x3001F8D4, 0x3002A26C), [](SafetyHookContext& ctx) {
        const char* hud_elem_shader_name = (const char*)(ctx.esp + 0x1C);
        hudelem_s* elem = (hudelem_s*)ctx.edi;
        // Check if shader has a config
        const HudShaderConfig* shaderConfig = FindHudShaderConfig(hud_elem_shader_name);




        float* x = (float*)(ctx.esi);
        float* y = (float*)(ctx.esp + 0x10);
        float* width = (float*)(ctx.esp + 0x18);
        float* height = (float*)(ctx.esp + 0x14);
        
        if (sp_mp(0, 1)) {
            y = (float*)(ctx.esp + 0x14);
            height = (float*)(ctx.esp + 0x10);
        }

        printf("x: %f y %f width %f height %f\n", *x, *y, *width, *height);
        auto isAlignX = [elem](const alignx_e alig_type) {
            if (!elem) {
                return false;
            }

            return elem->alignx == alig_type;

            };

        auto isAlignY = [elem](const aligny_e alig_type) {
            if (!elem) {
                return false;
            }

            return elem->aligny == alig_type;

            };


        // Hardcoded "black" OR config with stretch flag set

        bool is_black_screen = (strcmp(hud_elem_shader_name, "black") == 0);

        bool shouldStretch = (is_black_screen && (*width >= 640 && *height >= 480)) ||
            (shaderConfig && shaderConfig->alignment.stretch);
        if (shouldStretch) {

            *x += -process_width();

            static float fuckthis[2]{};
            *width += fuckthis[0];
            *height += fuckthis[1];

            *width *= (GetAspectRatio_standardfix() * 2.f);
        }
        // Apply alignment-based adjustments from config
        else if (shaderConfig) {


            float halfWidth = process_width() * 0.5f;
            float safeX = get_safeArea_horizontal();

            if (shaderConfig->alignment.h_left && !isAlignX(ALIGNX_LEFT)) {
                *x += (-halfWidth) * safeX;
            }
            else if (shaderConfig->alignment.h_right && !isAlignX(ALIGNX_RIGHT)) {
                *x += (halfWidth)*safeX;
            }
            float height_hack = process_height_hack_safe() * 0.5f;
            if (shaderConfig->alignment.v_bottom && !isAlignY(ALIGNY_BOTTOM) && !is_black_screen) {
                *y -= height_hack;
            }
            else if (shaderConfig->alignment.v_top && !isAlignY(ALIGNY_TOP) && !is_black_screen) {
                *y += height_hack;
            }

            // Add vertical when ready
            // if (shaderConfig->alignment.v_top) { ... }
            // if (shaderConfig->alignment.v_bottom) { ... }
        }

        });
    
    
}




SafetyHookInline Cvar_Set_og;
cvar_s* __cdecl Cvar_Set(const char* cvar_name, const char* value, BOOL force) {
    auto result = Cvar_Set_og.ccall<cvar_s*>(cvar_name, value, force);

    if (cvar_name && value) {
        //printf("cvar_name %s value %s cvar ptr %p\n", cvar_name, value, result);
    }

    if (result && result->name && (strcmp(result->name, "safeArea_horizontal") == 0)) {
        StaticInstructionPatches(NULL,false);
    }

    return result;



}

SafetyHookInline SCR_AdjustFrom640_OG;
bool ConsoleDrawing;
void Con_DrawConsole() {
    //cdecl_call<int>(0x4D7D70);
    ConsoleDrawing = true;
    cdecl_call<void>(0x40A450);
    ConsoleDrawing = false;
}



float* __cdecl SCR_AdjustFrom640(float* x, float* y, float* w, float* h) {

    auto result = SCR_AdjustFrom640_OG.ccall<float*>(x, y, w, h);

    if (ConsoleDrawing && x && h) {
        static float fuckwit[2];

        *x *= fuckwit[0];
        *w *= fuckwit[1];
        printf("fuckwit %p", fuckwit);
    }

}

uintptr_t qglTexParameteri_ptr;

bool GL_EXT_texture_filter_anisotropic_supported = false;

int __stdcall qglTexParameteri_aniso_hook1(int a1, int a2, int a3) {
    int filter_value = 1;
    if (r_ext_texture_filter_anisotropic) {
        filter_value = std::clamp(r_ext_texture_filter_anisotropic->integer, 1, 16);
    }
    if(GL_EXT_texture_filter_anisotropic_supported)
    stdcall_call<int>(*(int*)qglTexParameteri_ptr, a1, 0x84FE, filter_value);
    auto result = stdcall_call<int>(*(int*)qglTexParameteri_ptr, a1, a2, a3);
    return result;
    
}

int __stdcall qglTexParameteri_aniso_hook2(int a1, int a2, int a3) {
    if(GL_EXT_texture_filter_anisotropic_supported)
    stdcall_call<int>(*(int*)qglTexParameteri_ptr, a1, 0x84FE, 1);
    auto result = stdcall_call<int>(*(int*)qglTexParameteri_ptr, a1, a2, a3);
    return result;

}

SafetyHookInline com_initd;

int __cdecl com_init_hook() {
    auto result = com_initd.unsafe_ccall<int>();


    return result;

}

void InitHook() {
    CheckGame();
    if (!CheckGame()) {
        MessageBoxW(NULL, L"COD CLASSIC LOAD FAILED", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    InitializeDisplayModesForGame();

    //com_initd = safetyhook::create_inline(exe(0x00431CA0, 0x0043BC10), com_init_hook);

    auto pat = hook::pattern("FF 15 ? ? ? ? 8B 15 ? ? ? ? 52 E9");


    if (!pat.empty()) {

        uintptr_t call1 = (uintptr_t)pat.get_first();

        Memory::VP::Read<uintptr_t>(call1 + 2, qglTexParameteri_ptr);

        Memory::VP::Nop(call1, 6);

        pat = hook::pattern("FF 15 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 56");

        if (!pat.empty()) {
            Memory::VP::Nop(pat.get_first(), 6);

            Memory::VP::InjectHook(call1, qglTexParameteri_aniso_hook1, Memory::VP::HookType::Call);
            Memory::VP::InjectHook(pat.get_first(), qglTexParameteri_aniso_hook2, Memory::VP::HookType::Call);
        }
        pat = hook::pattern("FF 15 ? ? ? ? 83 C4 ? 5F 5E 5B 59 C3 68");
        if (!pat.empty()) {
            static auto GL_EXT_texture_filter_anisotropic_check = safetyhook::create_mid(pat.get_first(-40), [](SafetyHookContext& ctx) {
                GL_EXT_texture_filter_anisotropic_supported = ctx.eax != 0;
                });

                Memory::VP::Nop(pat.get_first(), 6);
        }
    }

    pat = hook::pattern("53 8B 5C 24 ? 56 8B 74 24 ? 57 53");
        
        if(!pat.empty())
        Cvar_Set_og = safetyhook::create_inline(pat.get_first(), Cvar_Set);
        pat = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 46 ? 5B 83 C4");
        if(!pat.empty())
        static auto blur_test = safetyhook::create_mid(pat.get_first(), [](SafetyHookContext& ctx) {

            static float fuck[4]{};
            float* args = (float*)(ctx.esp);


            args[0] -= process_widths();

            args[2] += process_widths() * 2.f;

            });
    
        pat = hook::pattern("FF 15 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 2B 4C 24");

        if (!pat.empty()) {
            // Borderless
            printf(" HOOKING window thing\n");
            static auto Borderless = safetyhook::create_mid(pat.get_first(), [](SafetyHookContext& ctx) {
                printf("window thing noborder cvar: %p\n", r_noborder);
                if (r_noborder && r_noborder->integer) {
                    auto borderless_style = WS_POPUP | WS_VISIBLE;
                    printf("first %p\n second %p\n", *(int*)(ctx.esp + 0x5C), *(int*)(ctx.esp + 0x4));
                    *(int*)(ctx.esp + 0x5C) = borderless_style;
                    *(int*)(ctx.esp + 0x4) = borderless_style;

                }

                });
        }

        pat = hook::pattern("8B 15 ? ? ? ? 8B 42 ? 5F");

        if (!pat.empty()) {
            static auto r_mode_auto_hook = safetyhook::create_mid(pat.get_first(), [](SafetyHookContext& ctx) {

                unsigned int* width = (unsigned int*)ctx.esi;
                unsigned int* height = (unsigned int*)ctx.edx;

                if (r_mode_auto && r_mode_auto->integer) {
                    *width = GetSystemMetrics(SM_CXSCREEN);
                    *height = GetSystemMetrics(SM_CYSCREEN);
                }

                });
        }

        pat = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? EB ? 57");

        if (!pat.empty()) {
            Memory::VP::Patch<uint32_t>(pat.get_first(4), WS_EX_LEFT);
        }

    char buffer[128]{};
    const char* buffertosee = "UNKNOWN";
    if (LoadedGame == &COD_UO_SP) {
        buffertosee = "COD_UO_SP";
    }
    else if (LoadedGame == &COD_SP) {
        buffertosee = "COD_UO_MP";
    }

    //sprintf_s(buffer, sizeof(buffer), "INIT START %s", buffertosee);
    //MessageBoxA(NULL, buffer, "Error", MB_OK | MB_ICONWARNING);

    SetUpFunctions();
    LoadMenuConfigs();
    LoadHudShaderConfigs();
    printf("should call the cg func\n");

    //pat = hook::pattern("83 C4 ? 8D 4C 24 ? 68 ? ? ? ? 51 E8 ? ? ? ? 8D 54 24");
    //if(!pat.empty())
    //    static auto AfterCvars = safetyhook::create_mid(pat.get_first(), sub_431CA0);

    Memory::VP::InterceptCall(exe(0x4ADADF, 0x4C4EFF), cvar_init_og, Cvar_Init_hook);

    

    //Memory::VP::InjectHook(0x00411757, Con_DrawConsole);

    //SCR_AdjustFrom640_OG = safetyhook::create_inline(0x411070, SCR_AdjustFrom640);

    LoadLibraryD = safetyhook::create_inline(LoadLibraryA, LoadLibraryHook);

    FreeLibraryD = safetyhook::create_inline(FreeLibrary, FreeLibraryHook);

    if (Cvar_Get != NULL)
    {


        //Memory::VP::InjectHook(0x4D7D90, glviewport_47BD978);

        //static auto test = CreateMidHook(0x004D7D90, [](SafetyHookContext& ctx) {

        //    int* args = (int*)ctx.esp;

        //    args[0] = process_width(0);

        //    float whatever = args[2];



        //    float mult = (STANDARD_ASPECT / GetAspectRatio());


        //    whatever *= mult;

        //    args[2] = (int)whatever;

        //    });


    }
    //if (MH_Initialize() != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO INITIALIZE", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}
    //if (MH_CreateHook((void**)CheckGame()->LoadDLLAddr, &hookCOD_dllLoad, (void**)&originalLoadDLL) != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}

    //originalLoadDLLd = safetyhook::CreateInlineHook(CheckGame()->LoadDLLAddr, hookCOD_dllLoad);


    //if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}

    //if (cg(1)) {
    //    static auto borderless_hook1 = safetyhook::create_mid(0x00502F87, [](SafetyHookContext& ctx) {
    //        auto& cdsFullScreen = ctx.esi;
    //        if (r_noborder && r_noborder->integer) {
    //            printf("fuckwit\n");
    //            cdsFullScreen = false;
    //            DWORD& dwStylea = *(DWORD*)(ctx.esp + 0x54);
    //            DWORD& dwExStylea = *(DWORD*)(ctx.esp + 0x50);
    //            dwExStylea = 0;
    //            dwStylea = WS_POPUP | WS_SYSMENU;
    //            ctx.eip = 0x00502FC0;
    //        }

    //        });
    //}

    std::thread([&]() {
        while (!glOrtho_og.target_address()) {
            if (*(uintptr_t*)LoadedGame->GL_Ortho_ptr) {
                glOrtho_og = safetyhook::create_inline(
                    *(uintptr_t*)LoadedGame->GL_Ortho_ptr,
                    &glOrtho_detour
                );
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        }).detach();


}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        OpenConsole();
        InitHook();
        //CheckModule();
        HMODULE moduleHandle;
        // idk why but this makes it not DETATCH prematurely
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)DllMain, &moduleHandle);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        //FreeConsole();
        //MH_Uninitialize();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

