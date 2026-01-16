	#pragma once
	#include <safetyhook.hpp>
	#include "MemoryMgr.h"

	extern std::vector<std::unique_ptr<SafetyHookInline>> g_inlineHooks;
	extern std::vector<std::unique_ptr<SafetyHookMid>> g_midHooks;
    // Define the templates HERE in the header
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