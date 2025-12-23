#include "framework.h"
#include "shared.h"
#include "structs.h"
#include "safetyhook.hpp"
#include "MemoryMgr.h"
#include "rinput.h"
#include <hidusage.h>

#include "Hooking.Patterns.h"

uintptr_t MessageMouse_addr;

import game;

namespace rinput {
	cevar_s* raw_input;
	int rawinput_x_current = 0;
	int rawinput_y_current = 0;
	int rawinput_x_old = 0;
	int rawinput_y_old = 0;

	uint32_t* window_center_x = 0;
	uint32_t* window_center_y = 0;
	int __cdecl MessageMouse_454590(int a2, int a3, int a4, int a5, int a6) {

		__asm xor eax, eax
		return cdecl_call<int>(MessageMouse_addr, a2, a3, a4, a5, a6);
	}
	static uintptr_t in_mouseold;
	static int rawInput_move()
	{
		if (raw_input && raw_input->base->integer) {
			auto delta_x = rawinput_x_current - rawinput_x_old;
			auto delta_y = rawinput_y_current - rawinput_y_old;

			rawinput_x_old = rawinput_x_current;
			rawinput_y_old = rawinput_y_current;

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			return MessageMouse_454590(3, delta_x, delta_y, 0, 0);
		}
		else {
			rawinput_x_current = 0;
			rawinput_y_current = 0;
			rawinput_x_old = 0;
			rawinput_y_old = 0;
			return cdecl_call<int>(in_mouseold);

		}
	}



	static void WM_INPUT_process(LPARAM lParam)
	{

		////

		UINT dwSize = sizeof(RAWINPUT);
		static RAWINPUT raw;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER));
		if (raw_input && raw_input->base->integer) {
			rawinput_x_current += raw.data.mouse.lLastX;
			rawinput_y_current += raw.data.mouse.lLastY;
		}
	}

	static void rawInput_init(HWND hWnd)
	{
		RAWINPUTDEVICE rid[1]{};
		rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		rid[0].dwFlags = RIDEV_INPUTSINK;
		rid[0].hwndTarget = hWnd;
		if (!RegisterRawInputDevices(rid, ARRAYSIZE(rid), sizeof(rid[0])))
			throw std::runtime_error("RegisterRawInputDevices failed");
	}
	uintptr_t MainWndProc_addr;
	LRESULT CALLBACK stub_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{

		switch (uMsg)
		{
		case WM_INPUT:
			WM_INPUT_process(lParam);
			return true;

		case WM_CREATE:
			//SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			rawInput_init(hWnd);
			break;
		}

		return stdcall_call<LRESULT>(MainWndProc_addr,hWnd, uMsg, wParam, lParam);

	}
	void Init() {
		Memory::VP::ReadCall(exe(0x45294A,0x469C6A), MessageMouse_addr);
		Memory::VP::InterceptCall(exe(0x452B99,0x469EB9), in_mouseold,rawInput_move);

		printf("MainWndProc_addr %p stub_MainWndProc %p\n", &MainWndProc_addr, stub_MainWndProc);
		Memory::VP::Read(exe(0x454992 + 1, 0x46BF01 + 1), MainWndProc_addr);
		Memory::VP::Patch<void*>(exe(0x454992 + 1, 0x46BF01 + 1), stub_MainWndProc);

		window_center_x = (uint32_t*)0x8EF7D8;
		window_center_y = (uint32_t*)0x8EF668;

		auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 85 C0 74 ? C7 05 ? ? ? ? 00 00 00 00 E9 ? ? ? ? E8");

		if (!pattern.empty()) {
			static auto clear_rawinput = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {
				rawinput_x_current = 0;
				rawinput_y_current = 0;
				rawinput_x_old = 0;
				rawinput_y_old = 0;
				});
		}

	}
}