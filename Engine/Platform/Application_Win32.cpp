// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Application.hpp"

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Application_Internal.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Types.hpp"

#include <Windows.h>

namespace
{
blk::Application* application = nullptr;
}  // namespace

blk::Result
blk::create_application(Allocator* allocator, HINSTANCE hinstance)
{
	if (!BLK_VERIFY(!application) || !BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	void* pointer = nullptr;

	if (const Result result = allocate(*allocator, pointer, sizeof(Application), alignof(Application));
		result != Result::SUCCESS)
	{
		return result;
	}

	application = static_cast<Application*>(pointer);
	application->hinstance = hinstance;
	application->allocator = allocator;

	return Result::SUCCESS;
}

void
blk::destroy_application()
{
	if (!BLK_VERIFY(application))
	{
		return;
	}

	if (!BLK_VERIFY(application->allocator))
	{
		// `Application` may be corrupted.
		return;
	}

	free(*application->allocator, application);
	application = nullptr;
}

bool
blk::is_application_running()
{
	if (!BLK_VERIFY(application))
	{
		return false;
	}

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
	if (!BLK_VERIFY(application))
	{
		return;
	}

	if (application->is_cursor_hidden)
	{
		// TODO (Bug): The display counter starts at -1 when no mouse is installed, so both `BLK_CHECK` assertions are
		// off by one on those machines. Query `GetSystemMetrics(SM_MOUSEPRESENT)` at startup and skip cursor management
		// when there is no mouse.
		const int counter = ShowCursor(TRUE);
		BLK_CHECK(counter == 0);
		application->is_cursor_hidden = false;
	}
}

void
blk::hide_cursor()
{
	if (!BLK_VERIFY(application))
	{
		return;
	}

	if (!application->is_cursor_hidden)
	{
		const int counter = ShowCursor(FALSE);
		BLK_CHECK(counter == -1);
		application->is_cursor_hidden = true;
	}
}

blk::Result
blk::set_cursor_position(const Rect<int>& position)
{
	if (!SetCursorPos(position.x, position.y))
	{
		return Result::OS_ERROR;
	}

	return Result::SUCCESS;
}

blk::Result
blk::get_cursor_position(Rect<int>& position)
{
	position = {};
	POINT point;

	if (!GetCursorPos(&point))
	{
		return Result::OS_ERROR;
	}

	position.x = static_cast<int>(point.x);
	position.y = static_cast<int>(point.y);

	return Result::SUCCESS;
}

blk::Application*
blk::get_application()
{
	return application;
}
