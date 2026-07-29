#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace blk
{
#ifdef _WIN32
struct Application
{
	HINSTANCE hinstance;
	bool should_quit;
	bool is_cursor_hidden;
};
#endif

Application* get_application();
}  // namespace blk
