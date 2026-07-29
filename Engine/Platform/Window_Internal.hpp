#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace blk
{
#ifdef _WIN32
struct Window
{
	HWND hwnd;
};
#endif

Window* get_window();
}  // namespace blk
