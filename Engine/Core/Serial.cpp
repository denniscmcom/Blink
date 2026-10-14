// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#ifdef BLK_NEW_API
#else

#include "Engine/Core/Core.hpp"

#include "Engine/Platform/Assert.hpp"

blk::Serial
blk::init_serial(char* buffer, size_t size)
{
	if (!BLK_VERIFY(buffer))
	{
		return {};
	}

	return Serial{.buffer = buffer, .size = size};
}

#endif
