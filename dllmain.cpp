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


#include "structs.h"

#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

static std::vector<std::unique_ptr<SafetyHookInline>> g_inlineHooks;
static std::vector<std::unique_ptr<SafetyHookMid>> g_midHooks;

template<typename T, typename Fn>
SafetyHookInline* CreateInlineHook(T target, Fn destination, SafetyHookInline::Flags flags = SafetyHookInline::Default) {
    auto hook = std::make_unique<SafetyHookInline>(safetyhook::create_inline(target, destination, flags));
    auto* ptr = hook.get();
    g_inlineHooks.push_back(std::move(hook));
    return ptr;
}

template<typename T>
SafetyHookMid* CreateMidHook(T target, safetyhook::MidHookFn destination, safetyhook::MidHook::Flags flags = safetyhook::MidHook::Default) {
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

typedef cvar_t* (__cdecl* Cvar_GetT)(char* var_name, const char* var_value, int flags);
Cvar_GetT Cvar_Get = (Cvar_GetT)NULL;

cvar_t* cg_fovscale;
cvar_t* cg_fovfixaspectratio;
cvar_t* safeArea_horizontal;
void codDLLhooks(HMODULE handle);

typedef HMODULE(__cdecl* LoadsDLLsT)(const char* a1, FARPROC* a2, int a3);
LoadsDLLsT originalLoadDLL = nullptr;

struct COD_Classic_Version {
    DWORD WinMain_Check[2];
    const char* DLLName;
    DWORD LoadDLLAddr;
    DWORD DLL_CG_GetViewFov_offset;
    DWORD Cvar_Get_Addr;
    DWORD X_res_Addr;
};

COD_Classic_Version COD_UO_SP = {
{0x00455050,0x83EC8B55},
"uo_cgamex86.dll",
0x454440,
0x2CC20,
0x004337F0,
0x047BE104,
};

COD_Classic_Version COD_SP = {
{0x00452D60,0x83EC8B55},
"cgamex86.dll",
0x452190,
0x25200,
0x004319F0,
0x015347C4,
};

COD_Classic_Version *LoadedGame = NULL;


SafetyHookInline* CG_GetViewFov_og_S{};
constexpr auto STANDARD_ASPECT = 1.33333333333f;
void OpenConsole()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    std::cout << "Console initialized.\n";
    printf("hi");
}
float GetAspectRatio() {
    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    printf("aspect ratio %f\n", x / y);

    return x / y;
}

double GetAspectRatio_standardfix() {
    return (GetAspectRatio() / STANDARD_ASPECT);
}

double CG_GetViewFov_hook() {
    double fov = CG_GetViewFov_og_S->call<double>();

    if (cg_fovscale && cg_fovscale->value) {
        if (cg_fovfixaspectratio && cg_fovfixaspectratio->integer) {
            // Convert horizontal FOV to vertical, then back to horizontal with new aspect ratio
            double halfFovRad = (fov / 2.0) * (M_PI / 180.0); // Convert to radians
            double tanHalfFov = tan(halfFovRad);

            // Convert to vertical FOV (aspect-independent)
            double tanHalfVFov = tanHalfFov / STANDARD_ASPECT;

            // Convert back to horizontal FOV with current aspect ratio
            double newTanHalfFov = tanHalfVFov * GetAspectRatio();

            // Convert back to degrees
            fov = 2.0 * atan(newTanHalfFov) * (180.0 / M_PI);
        }

        fov = fov * cg_fovscale->value;
    }

    return fov;
}

void CheckModule()
{
    HMODULE hMod = GetModuleHandleA(LoadedGame->DLLName);
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
    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    // orthoWidth = (x² / y²) × (3/4) × y = (x² × 3) / (4y)
    float orthoWidth = (x * x * 3.0f) / (4.0f * y);

    width += (orthoWidth - x) / 2.0f;

    return width;
}

// For game functions - adjusts game's internal 480-based coordinate system
double process_width(double width = 0) {
    float x = (float)*(int*)LoadedGame->X_res_Addr;
    float y = (float)*(int*)(LoadedGame->X_res_Addr + 0x4);

    // Same formula but scaled to 480-height coordinate space
    float aspect = x / y;
    width += 480.0f * aspect - 640.0f;

    return width;
}

int process_width(int width) {
    return (int)process_width((int)0);
}

SafetyHookInline LoadLibraryD;

HMODULE __stdcall LoadLibraryHook(const char* filename) {

    auto hModule = LoadLibraryD.unsafe_stdcall<HMODULE>(filename);

    if (strstr(filename, "cgame") != NULL) {
        codDLLhooks(hModule);
    }

    return hModule;

}


SafetyHookInline FreeLibraryD;

BOOL __stdcall FreeLibraryHook(HMODULE hLibModule) {
    // Get the module's address range
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
    if (!safeArea_horizontal)
        return 1.f;
    return std::clamp(safeArea_horizontal->value, 0.1f, 1.f);
}

void Con_DrawConsole() {
    cdecl_call<int>(0x4D7D70);
    cdecl_call<void>(0x40A450);

}

void InitHook() {
    if(!CheckGame())
    return;
    SetUpFunctions();

    Memory::VP::InjectHook(0x00411757, Con_DrawConsole);

    LoadLibraryD = safetyhook::create_inline(LoadLibraryA, LoadLibraryHook);

    FreeLibraryD = safetyhook::create_inline(FreeLibrary, FreeLibraryHook);

    if (Cvar_Get != NULL) 
    {
        cg_fovscale = Cvar_Get((char*)"cg_fovscale", "1.0", CVAR_ARCHIVE);
        cg_fovfixaspectratio = Cvar_Get((char*)"cg_fovfixaspectratio", "1.0", CVAR_ARCHIVE);
        safeArea_horizontal = Cvar_Get((char*)"safeArea_horizontal", "1.0", CVAR_ARCHIVE);

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

    MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
}

SafetyHookInline glOrtho_og{};



struct OrthoParams {
    double left;
    double right;
    double bottom;
    double top;
    double zNear;
    double zFar;
};

SafetyHookInline qglViewport_og{};

void __stdcall qglViewport_detour(int x,
    int y,
    unsigned int width,
    unsigned int height) {

    if (_ReturnAddress() == (void*)0x4D7D96) {
        x = 240;
        width = 1440;
    }
    qglViewport_og.unsafe_stdcall(x, y, width, height);
};


SafetyHookInline qglScissor_og{};

void __stdcall qglScisso_detour(int x,
    int y,
    unsigned int width,
    unsigned int height) {

    if (_ReturnAddress() == (void*)0x004D7DAD) {
        x = 240;
        width = 1440;
    }
    qglScissor_og.unsafe_stdcall(x, y, width, height);
};

int __stdcall glOrtho_detour(double left, double right, double bottom, double top, double zNear, double zFar) {
    static OrthoParams params = { 0, 0, 0, 0, 0, 0 };

    //printf("glOrtho called from: %p\n", _ReturnAddress());
    //printf("  left   = %.2f (addr: %p)\n", params.left, &params.left);
    //printf("  right  = %.2f (addr: %p)\n", params.right, &params.right);
    //printf("  bottom = %.2f (addr: %p)\n", params.bottom, &params.bottom);
    //printf("  top    = %.2f (addr: %p)\n", params.top, &params.top);
    //printf("  zNear  = %.2f (addr: %p)\n", params.zNear, &params.zNear);
    //printf("  zFar   = %.2f (addr: %p)\n", params.zFar, &params.zFar);

    if (_ReturnAddress() == (void*)0x004D7E02) {
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

        left -= process_widths() +  floatsy[0];
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

SafetyHookMid* DrawHudElemMaterial_mid{};

uintptr_t DrawStretch_og{};

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
    // The ortho fix makes things 4:3, but the crosshair was scaled for widescreen
    // So we need to scale it UP to maintain proper size
    float aspect = GetAspectRatio();
    float aspectRatio = aspect / (4.0f / 3.0f);

    width *= aspectRatio;
    height *= aspectRatio;

    cdecl_call<void>(crosshair_render_func, x, y, width, height, unk1, u1, u2, v1, rotation, shaderHandle);
}

SafetyHookInline* Item_Paint{};

#define ALIGN_LEFT_BOTTOM   "[LB]"
#define ALIGN_LEFT_TOP      "[LT]"
#define ALIGN_RIGHT_BOTTOM  "[RB]"
#define ALIGN_RIGHT_TOP     "[RT]"
#define ALIGN_CENTER        "[CENTER]"
#define ALIGN_LEFT          "[LEFT]"
#define ALIGN_RIGHT         "[RIGHT]"
#define ALIGN_TOP           "[TOP]"
#define ALIGN_BOTTOM        "[BOTTOM]"

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
// HOOK FUNCTION
// ============================================================================
void __fastcall Item_Paint_Hook(itemDef_t* item) {
    AlignmentState state = { 0 };

    // Process alignment
    ProcessItemAlignment(item, nullptr, 0, &state);

    // Call original render
    Item_Paint->unsafe_thiscall(item);

    // Restore original state
    RestoreItemState(item, &state);
}

void codDLLhooks(HMODULE handle) {
   // printf("run");
    uintptr_t OFFSET = (uintptr_t)handle;
    //printf("HANDLE : 0x%p ADDR : 0x%p \n", handle, OFFSET + 0x2CC20);
    //CG_GetViewFov_og_S.reset();

    static auto blur_test = safetyhook::create_mid(0x4D9926, [](SafetyHookContext& ctx) {

        static int fuck[6]{};
        printf("fuck %p\n", fuck);
        *(int*)(ctx.esp) += fuck[0];

        *(int*)(ctx.esp + 0x4) += fuck[1];
        *(int*)(ctx.esp + 0x4 + 0x4) += fuck[2];

        *(int*)(ctx.esp + 0x4 + 0x4 + 0x4) += fuck[3];
        });

    Memory::VP::InterceptCall(OFFSET + 0x122BB, DrawStretch_og, DrawStretch_300135B0);
    Memory::VP::InterceptCall(OFFSET + 0x1221A, DrawStretch_og, DrawStretch_300135B0);

    Memory::VP::InterceptCall(OFFSET + 0x11F68, crosshair_render_func, crosshair_render_hook);

    Memory::VP::InterceptCall(OFFSET + 0x11222, trap_R_DrawStretchPic, R_DrawStretchPic_leftsniper);

    Memory::VP::InterceptCall(OFFSET + 0x11270, trap_R_DrawStretchPic, R_DrawStretchPic_rightsniper);

    Memory::VP::Nop(OFFSET + 0x11205, 2);

    Memory::VP::Nop(OFFSET + 0x11247, 2);

    CG_DrawPicAddr = OFFSET + 0x13B70;

    CG_GetViewFov_og_S = CreateInlineHook(OFFSET + LoadedGame->DLL_CG_GetViewFov_offset, &CG_GetViewFov_hook);
        //if (MH_CreateHook((void**)OFFSET + 0x2CC20, &CG_GetViewFov_hook, (void**)&CG_GetViewFov_og) != MH_OK) {
        //    MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
        //    return;
        //}

    Item_Paint = CreateInlineHook(OFFSET + 0x43EE0, Item_Paint_Hook);

    static auto DrawObjectives = CreateMidHook(OFFSET + 0x12900, [](SafetyHookContext& ctx) {
        float halfWidth = process_width() * 0.5f;
        float safeX = get_safeArea_horizontal();

        ctx.ecx -= halfWidth * get_safeArea_horizontal();

        });

    if (*(uintptr_t*)0x047BCF98 && !glOrtho_og.target_address()) {
        glOrtho_og = safetyhook::create_inline(*(uintptr_t*)0x047BCF98, &glOrtho_detour);
    }

    if (*(uintptr_t*)0x47BD978 && !qglViewport_og.target_address()) {
        //qglViewport_og = safetyhook::create_inline(*(uintptr_t*)0x047BD978, &qglViewport_detour);
    }

    if (*(uintptr_t*)0x47BD718 && !qglScissor_og.target_address()) {
        //qglScissor_og = safetyhook::create_inline(*(uintptr_t*)0x47BD718, &qglScisso_detour);
    }

    //DrawHudElemMaterial_mid.reset();
    DrawHudElemMaterial_mid = CreateMidHook(OFFSET + 0x1F8D4, [](SafetyHookContext& ctx) {

        const char* hud_elem_shader_name = (const char*)(ctx.esp + 0x1C);

        if (strcmp(hud_elem_shader_name, "black") == 0) {


            float* x = (float*)(ctx.esi);

            float& y = *(float*)(ctx.esp + 0x10);

            float& width = *(float*)(ctx.esp + 0x18);
            float& height = *(float*)(ctx.esp + 0x14);

            *x += -process_width();

            //printf("width %f, height %f before\n", width, height);

            //height = width * GetAspectRatio_standardfix();

            static float fuckthis[2]{};

            width += fuckthis[0];
            height += fuckthis[1];

            //height = 360;

            width *= (GetAspectRatio_standardfix() * 1.25f);
            /*printf("width %f, height %f after %p\n", width, height,fuckthis);*/
        }


        });
    
    
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitHook();
        OpenConsole();
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

