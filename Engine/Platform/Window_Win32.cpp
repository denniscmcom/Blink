// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Window.hpp"

#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Application_Internal.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Window_Internal.hpp"

#include <Windows.h>
#include <imgui.h>

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
blk::Window* window = nullptr;
}  // namespace

void
blk::create_window(const char* title)
{
	BLK_CHECK(window == nullptr);

	Application* application = get_application();
	BLK_CHECK(application);

	WNDCLASSEXA window_class = {};
	window_class.cbSize = sizeof(window_class);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = MainWndProc;
	window_class.hInstance = application->hinstance;
	window_class.lpszClassName = "BlinkWindowClass";

	if (!RegisterClassExA(&window_class))
	{
		BLK_FATAL("Failed to register window class\n");
	}

	// FIXME: I think this will cause problems in the future (maybe not reliable in different computers).
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		BLK_FATAL("Failed to set DPI awareness\n");
	}

	HWND hwnd = CreateWindowA(
		window_class.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		application->hinstance,
		NULL
	);

	if (!hwnd)
	{
		BLK_FATAL("Failed to create window\n");
	}

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	window = static_cast<Window*>(malloc(sizeof(Window)));
	BLK_CHECK(window);
	window->hwnd = hwnd;

	// FIXME: When moving view with mouse all becomes like jittery.
	RAWINPUTDEVICE raw_input_mouse_device = {};
	raw_input_mouse_device.usUsagePage = 0x01;
	raw_input_mouse_device.usUsage = 0x02;
	raw_input_mouse_device.hwndTarget = window->hwnd;

	if (!RegisterRawInputDevices(&raw_input_mouse_device, 1, sizeof(raw_input_mouse_device)))
	{
		BLK_FATAL("Failed to register raw input mouse device\n");
	}
}

void
blk::destroy_window()
{
	if (window)
	{
		free(window);
	}
}

blk::Rect<unsigned>
blk::get_window_client_size()
{
	BLK_CHECK(window);

	RECT rect;

	if (!GetClientRect(window->hwnd, &rect))
	{
		BLK_FATAL("Failed to get client rect\n");
	}

	const auto width = static_cast<unsigned>(rect.right - rect.left);
	const auto height = static_cast<unsigned>(rect.bottom - rect.top);

	return Rect{.x = width, .y = height};
}

blk::Window*
blk::get_window()
{
	return window;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
	blk::Application* application = blk::get_application();

	if (ImGui_ImplWin32_WndProcHandler(hwnd, message, w_param, l_param))
	{
		return true;
	}

	blk::Event event{};

	switch (message)
	{
	case WM_ACTIVATEAPP:
		break;
	case WM_SIZE:
		break;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		application->should_quit = true;
		break;
	case WM_INPUT: {
		UINT raw_input_data_size;

		if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(l_param),
				RID_INPUT,
				nullptr,
				&raw_input_data_size,
				sizeof(RAWINPUTHEADER)
			) != 0)
		{
			BLK_FATAL("Failed to get raw input size\n");
		}

		constexpr size_t max_raw_input_size = 1024;
		alignas(RAWINPUT) BYTE raw_input_data[max_raw_input_size];
		BLK_CHECK(raw_input_data_size <= sizeof(raw_input_data));

		if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(l_param),
				RID_INPUT,
				raw_input_data,
				&raw_input_data_size,
				sizeof(RAWINPUTHEADER)
			) == -1)
		{
			BLK_FATAL("Failed to get raw input data\n");
		}

		if (auto raw_input = reinterpret_cast<const RAWINPUT*>(raw_input_data);
			raw_input->header.dwType == RIM_TYPEMOUSE)
		{
			const RAWMOUSE& mouse_input = raw_input->data.mouse;

			if ((mouse_input.usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE)
			{
				event.type = blk::Event_Type::MOUSE_MOVE;
				event.mouse_delta_x = mouse_input.lLastX;
				event.mouse_delta_y = mouse_input.lLastY;
				blk::write_event(event);
			}

			if (mouse_input.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
			{
				event.type = blk::Event_Type::KEY_DOWN;
				event.key = blk::Key::MOUSE_RIGHT;
				blk::write_event(event);
			}

			if (mouse_input.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
			{
				event.type = blk::Event_Type::KEY_UP;
				event.key = blk::Key::MOUSE_RIGHT;
				blk::write_event(event);
			}
		}
	}

		// WM_INPUT requires DefWindowProc for cleanup.
		return DefWindowProcA(hwnd, message, w_param, l_param);
	case WM_KEYDOWN:
		event.type = blk::Event_Type::KEY_DOWN;
		// Fallthrough.
	case WM_KEYUP:
		if (event.type != blk::Event_Type::KEY_DOWN)
		{
			event.type = blk::Event_Type::KEY_UP;
		}

		switch (w_param)
		{
		case VK_ESCAPE:
			event.key = blk::Key::KEYBOARD_ESC;
			blk::write_event(event);
			break;
		case VK_F1:
			event.key = blk::Key::KEYBOARD_F1;
			blk::write_event(event);
			break;
		case VK_F2:
			event.key = blk::Key::KEYBOARD_F2;
			blk::write_event(event);
			break;
		case 'Q':
			event.key = blk::Key::KEYBOARD_Q;
			blk::write_event(event);
			break;
		case 'W':
			event.key = blk::Key::KEYBOARD_W;
			blk::write_event(event);
			break;
		case 'E':
			event.key = blk::Key::KEYBOARD_E;
			blk::write_event(event);
			break;
		case 'A':
			event.key = blk::Key::KEYBOARD_A;
			blk::write_event(event);
			break;
		case 'S':
			event.key = blk::Key::KEYBOARD_S;
			blk::write_event(event);
			break;
		case 'D':
			event.key = blk::Key::KEYBOARD_D;
			blk::write_event(event);
			break;
		default:
			break;
		}
		break;
	default:
		return DefWindowProcA(hwnd, message, w_param, l_param);
	}

	return 0;
}
