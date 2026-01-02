#pragma once

#pragma warning (disable : 4244)
#pragma warning (disable : 4477)

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include "buildnumber.h"

#include <helper.hpp>
#include <iostream>
#include "Hooking.Patterns.h"
#include "safetyhook.hpp"
#include "shared.h"

#include "MemoryMgr.h"

#include <deque>

#include <set>
#include <algorithm>

#include "structs.h"

#include <stacktrace>


#include "cevar.h"
#include "rinput.h"
#include "GMath.h"

#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"
#include <unordered_map>

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
