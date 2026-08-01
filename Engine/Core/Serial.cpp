// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Serial.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"

#include <stdlib.h>
#include <string.h>

blk::Serial::Serial(size_t size)
{
	resize(size);
}

blk::Serial::Serial(const char* buffer, size_t size)
{
	BLK_CHECK(buffer);

	resize(size);
	memcpy(buffer_, buffer, size);
}

blk::Serial::~Serial()
{
	if (buffer_)
	{
		free(buffer_);
	}
}

char*
blk::Serial::buffer() const
{
	return buffer_;
}

size_t
blk::Serial::size() const
{
	return size_;
}

size_t
blk::Serial::position() const
{
	return position_;
}

void
blk::Serial::resize(size_t size)
{
	if (size <= size_)
	{
		return;
	}

	size_t new_size = size_ * 2;

	if (new_size < size)
	{
		new_size = size;
	}

	auto* new_buffer = static_cast<char*>(calloc(new_size, 1));

	if (!new_buffer)
	{
		BLK_FATAL("Failed to allocate serial buffer\n");
	}

	if (buffer_)
	{
		memcpy(new_buffer, buffer_, size_);
		free(buffer_);
	}

	buffer_ = new_buffer;
	size_ = new_size;
}
