#include "helper.hpp"
#include <safetyhook.hpp>
std::vector<std::unique_ptr<SafetyHookInline>> g_inlineHooks;
std::vector<std::unique_ptr<SafetyHookMid>> g_midHooks;
