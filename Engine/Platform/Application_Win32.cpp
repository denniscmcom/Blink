// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Application.hpp"

#include "Engine/Platform/Application_Internal.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"

#include <Windows.h>

namespace
{
blk::Application* application = nullptr;
}  // namespace

void
blk::create_application(HINSTANCE hinstance)
{
	BLK_CHECK(application == nullptr);
	application = new Application();
	BLK_CHECK(application);
	application->hinstance = hinstance;
}

void
blk::destroy_application()
{
	if (application)
	{
		delete application;
	}
}

bool
blk::is_application_running()
{
	BLK_CHECK(application);

	MSG message = {};

	while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	return !application->should_quit;
}

void
blk::show_cursor()
{
	BLK_CHECK(application);

	if (application->is_cursor_hidden)
	{
		const int counter = ShowCursor(TRUE);

		if (counter == -1)
		{
			BLK_WARNING("Mouse is not installed\n");
			return;
		}

		BLK_CHECK(counter == 0);
		application->is_cursor_hidden = false;
	}
}

void
blk::hide_cursor()
{
	BLK_CHECK(application);

	if (!application->is_cursor_hidden)
	{
		const int counter = ShowCursor(FALSE);
		BLK_CHECK(counter == -1);
		application->is_cursor_hidden = true;
	}
}

void
blk::set_cursor_position(const Rect<int>& position)
{
	BLK_VERIFY(SetCursorPos(position.x, position.y));
}

blk::Rect<int>
blk::get_cursor_position()
{
	POINT point;

	if (!BLK_VERIFY(GetCursorPos(&point)))
	{
		return {};
	}

	return Rect{.x = static_cast<int>(point.x), .y = static_cast<int>(point.y)};
}

blk::Application*
blk::get_application()
{
	return application;
}
