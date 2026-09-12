// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stddef.h>
#include <string.h>

namespace blk
{
/// Non-owning serializer/deserializer.
struct Serial
{
	/// Data buffer.
	char* buffer = nullptr;
	/// Size in bytes of `buffer`.
	size_t size = 0;
	/// Next byte position to read from or write to.
	size_t position = 0;
};

/// Initializes `Serial`.
Serial init_serial(char* buffer, size_t size);
/// Reads `sizeof(Type)` from `serial.buffer` and writes the result to `data`.
/// @warning `Type` should be serializable.
template <typename Type>
Result read(Serial& serial, Type& data);
/// Writes `data` to `serial.buffer`.
///
/// `data` is taken by reference so that an array keeps its type. Taking it by value would let `Type` decay to a
/// pointer, and the write would store the pointer instead of what it points at – which is what `read` would then fail
/// to match, since it takes a reference already.
/// @warning `Type` should be serializable.
template <typename Type>
Result write(Serial& serial, const Type& data);
/// Writes `size` bytes from `pointer` to `serial.buffer`.
template <typename Type>
Result write(Serial& serial, Type* pointer, size_t size);
}  // namespace blk

namespace blk
{
template <typename Type>
Result
read(Serial& serial, Type& data)
{
	if (serial.position + sizeof(Type) > serial.size)
	{
		return Result::OUT_OF_BOUNDS;
	}

	memcpy(&data, serial.buffer + serial.position, sizeof(Type));
	serial.position += sizeof(Type);

	return Result::SUCCESS;
}

template <typename Type>
Result
write(Serial& serial, const Type& data)
{
	if (serial.position + sizeof(Type) > serial.size)
	{
		return Result::OUT_OF_BOUNDS;
	}

	memcpy(serial.buffer + serial.position, &data, sizeof(Type));
	serial.position += sizeof(Type);

	return Result::SUCCESS;
}

template <typename Type>
Result
write(Serial& serial, Type* pointer, size_t size)
{
	if (!BLK_VERIFY(pointer))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (serial.position + size > serial.size)
	{
		return Result::OUT_OF_BOUNDS;
	}

	memcpy(serial.buffer + serial.position, pointer, size);
	serial.position += size;

	return Result::SUCCESS;
}
}  // namespace blk
