#pragma once

#ifdef _WIN32
#include <Windows.h>

#define BLK_ENTRY()                                                                                                    \
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, const int /*nShowCmd*/)
#else
#define BLK_ENTRY() ((void)0)
#endif

namespace blk
{
template <typename Type>
struct Rect
{
	Type x;
	Type y;
};

#ifdef _WIN32
void create_application(HINSTANCE hinstance);
#endif

void destroy_application();
bool is_application_running();
void show_cursor();
void hide_cursor();
void set_cursor_position(const Rect<int>& position);
Rect<int> get_cursor_position();
}  // namespace blk
