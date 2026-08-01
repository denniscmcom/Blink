// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"

#include <string.h>
#include <type_traits>

namespace blk
{
template <typename Type>
concept Serializable = std::is_trivially_copyable_v<Type> && !std::is_pointer_v<Type>;

class Serial
{
  public:
	explicit Serial(size_t size);
	Serial(const char* buffer, size_t size);

	Serial(const Serial& other) = delete;
	Serial(Serial&& other) noexcept = delete;
	Serial& operator=(const Serial& other) = delete;
	Serial& operator=(Serial&& other) noexcept = delete;

	~Serial();

	template <typename Type>
		requires Serializable<Type>
	Type read();

	template <typename Type>
		requires Serializable<Type>
	void write(Type data);

	template <typename Type>
	void write(Type* pointer, size_t size);

	char* buffer() const;
	size_t size() const;
	size_t position() const;

  private:
	/// Grows the buffer so it holds at least `size` bytes. Does nothing if it already fits.
	void resize(size_t size);

	char* buffer_ = nullptr;
	size_t size_ = 0;
	size_t position_ = 0;
};

template <typename Type>
	requires Serializable<Type>
Type
Serial::read()
{
	if (position_ + sizeof(Type) > size_)
	{
		BLK_FATAL("Read out of bounds\n");
	}

	Type data;
	memcpy(&data, buffer_ + position_, sizeof(Type));
	position_ += sizeof(Type);

	return data;
}

template <typename Type>
	requires Serializable<Type>
void
Serial::write(Type data)
{
	resize(position_ + sizeof(Type));

	memcpy(buffer_ + position_, &data, sizeof(Type));
	position_ += sizeof(Type);
}

template <typename Type>
void
Serial::write(Type* pointer, size_t size)
{
	BLK_CHECK(pointer);

	resize(position_ + size);

	memcpy(buffer_ + position_, pointer, size);
	position_ += size;
}
}  // namespace blk
