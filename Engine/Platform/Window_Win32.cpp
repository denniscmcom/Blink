// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Window.hpp"

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Application_Internal.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Types.hpp"
#include "Engine/Platform/Window_Internal.hpp"

#include <Windows.h>
#include <imgui.h>
#include <stdint.h>

// Win32 window callback.
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
// Win32 window callback needed by ImGui to handle UI events.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
/// Pointer to the main window of the engine.
blk::Window* window = nullptr;
}  // namespace

blk::Result
blk::create_window(Allocator* allocator, const char* title)
{
	BLK_CHECK(window == nullptr);

	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(title))
	{
		return Result::INVALID_ARGUMENTS;
	}

	const Application* application = get_application();
	BLK_CHECK(application);

	WNDCLASSEXW window_class = {};
	window_class.cbSize = sizeof(window_class);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = MainWndProc;
	window_class.hInstance = application->hinstance;
	window_class.lpszClassName = L"BlinkWindowClass";

	if (!RegisterClassExW(&window_class))
	{
		return Result::OS_ERROR;
	}

	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		return Result::OS_ERROR;
	}

	wchar_t wtitle[MAX_WINDOW_TITLE_SIZE];

	if (MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, static_cast<int>(MAX_WINDOW_TITLE_SIZE)) <= 0)
	{
		return Result::INVALID_ARGUMENTS;
	}

	HWND hwnd = nullptr;

	if (hwnd = CreateWindowW(
			window_class.lpszClassName,
			wtitle,
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
		!hwnd)
	{
		return Result::OS_ERROR;
	}

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	void* pointer = nullptr;

	if (const Result result = allocate(*allocator, pointer, sizeof(Window), alignof(Window)); result != Result::SUCCESS)
	{
		return result;
	}

	window = static_cast<Window*>(pointer);
	window->allocator = allocator;
	window->hwnd = hwnd;

	RAWINPUTDEVICE raw_input_mouse_device = {};
	raw_input_mouse_device.usUsagePage = 0x01;
	raw_input_mouse_device.usUsage = 0x02;
	raw_input_mouse_device.hwndTarget = window->hwnd;

	if (!RegisterRawInputDevices(&raw_input_mouse_device, 1, sizeof(raw_input_mouse_device)))
	{
		return Result::OS_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::destroy_window()
{
	if (!window)
	{
		return;
	}

	BLK_CHECK(window->allocator);

	// TODO (Bug): The window class registered by `create_window` is never unregistered, so calling `create_window`
	// again fails with `ERROR_CLASS_ALREADY_EXISTS`. Unregistering it needs the `HINSTANCE` from `Application`.
	DestroyWindow(window->hwnd);
	free(*window->allocator, window);

	window = nullptr;
}

blk::Result
blk::get_window_client_rect(Rect<unsigned>& rect)
{
	if (!BLK_VERIFY(window))
	{
		rect = {};
		return Result::INVALID_ARGUMENTS;
	}

	RECT hwnd_rect = {};

	if (!GetClientRect(window->hwnd, &hwnd_rect))
	{
		return Result::OS_ERROR;
	}

	const auto width = static_cast<unsigned>(hwnd_rect.right - hwnd_rect.left);
	const auto height = static_cast<unsigned>(hwnd_rect.bottom - hwnd_rect.top);

	rect = Rect{
		.x = width,
		.y = height,
	};

	return Result::SUCCESS;
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

		if (BLK_VERIFY(application))
		{
			application->should_quit = true;
		}

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
		if (!BLK_VERIFY(raw_input_data_size <= sizeof(raw_input_data)))
		{
			return DefWindowProcW(hwnd, message, w_param, l_param);
		}

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

			if ((mouse_input.usFlags & MOUSE_MOVE_ABSOLUTE) != MOUSE_MOVE_ABSOLUTE)
			{
				event.type = blk::Event_Type::MOUSE_MOVE;
				event.mouse_delta_x = static_cast<int32_t>(mouse_input.lLastX);
				event.mouse_delta_y = static_cast<int32_t>(mouse_input.lLastY);
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
		return DefWindowProcW(hwnd, message, w_param, l_param);
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
		case VK_F3:
			event.key = blk::Key::KEYBOARD_F3;
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
		return DefWindowProcW(hwnd, message, w_param, l_param);
	}

	return 0;
}
