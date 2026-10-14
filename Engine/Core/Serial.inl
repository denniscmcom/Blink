// Copyright DCM Systems. All Rights Reserved.

#pragma once

#ifdef BLK_NEW_API
#else
#include "Engine/Platform/Result.hpp"

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

#endif
