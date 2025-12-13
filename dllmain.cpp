// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
//#include "MinHook.h"
#include <windows.h>
#include <iostream>
#include "Hooking.Patterns.h"
#include "safetyhook.hpp"
//#pragma comment(lib, "libMinHook.x86.lib")
#include "shared.h"
typedef cvar_t* (__cdecl* Cvar_GetT)(char* var_name, const char* var_value, int flags);
Cvar_GetT Cvar_Get = (Cvar_GetT)NULL;

cvar_t* cg_fovscale;
cvar_t* cg_fovfixaspectratio;
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


SafetyHookInline CG_GetViewFov_og_S{};
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
    float y = (float)*(int*)LoadedGame->X_res_Addr + 0x4;
    return x / y;
}
double CG_GetViewFov_hook() {
    double fov = CG_GetViewFov_og_S.call<double>();
    if (cg_fovscale && cg_fovscale->value) {
        if (cg_fovfixaspectratio && !cg_fovfixaspectratio->integer) {
            fov = fov * (GetAspectRatio() / STANDARD_ASPECT);
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

void InitHook() {
    if(!CheckGame())
    return;
    SetUpFunctions();
    if (Cvar_Get != NULL) 
    {
        cg_fovscale = Cvar_Get((char*)"cg_fovscale", "1.0", CVAR_ARCHIVE);
        cg_fovfixaspectratio = Cvar_Get((char*)"cg_fovfixaspectratio", "1", CVAR_ARCHIVE);
    }
    //if (MH_Initialize() != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO INITIALIZE", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}
    //if (MH_CreateHook((void**)CheckGame()->LoadDLLAddr, &hookCOD_dllLoad, (void**)&originalLoadDLL) != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}

    originalLoadDLLd = safetyhook::create_inline(CheckGame()->LoadDLLAddr, hookCOD_dllLoad);


    //if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
    //    //MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
    //    return;
    //}

    MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
}

void codDLLhooks(HMODULE handle) {
   // printf("run");
    uintptr_t OFFSET = (uintptr_t)handle;
    //printf("HANDLE : 0x%p ADDR : 0x%p \n", handle, OFFSET + 0x2CC20);
    CG_GetViewFov_og_S.reset();
    CG_GetViewFov_og_S = safetyhook::create_inline(OFFSET + LoadedGame->DLL_CG_GetViewFov_offset, &CG_GetViewFov_hook);
        //if (MH_CreateHook((void**)OFFSET + 0x2CC20, &CG_GetViewFov_hook, (void**)&CG_GetViewFov_og) != MH_OK) {
        //    MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
        //    return;
        //}
    
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
        //OpenConsole();
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

