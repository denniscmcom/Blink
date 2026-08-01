// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Iterator.hpp"
#include "Engine/Platform/Assert.hpp"

#include <functional>
#include <stdint.h>
#include <utility>
#include <vector>

namespace blk
{
template <typename>
struct Pool_Handle
{
	uint32_t id = UINT32_MAX;
	uint32_t version = UINT32_MAX;
};

template <typename Type>
bool operator==(const Pool_Handle<Type>& lhs, const Pool_Handle<Type>& rhs);

// TODO: Is possible to implement a default operator to do if(some_pool_handle) that returns true if it is not
//   POOL_HANDLE_NONE?
template <typename Type>
constexpr Pool_Handle<Type> POOL_HANDLE_NONE = {.id = UINT32_MAX, .version = UINT32_MAX};

template <typename Type>
struct Pool_Handle_Hash
{
	size_t operator()(const Pool_Handle<Type>& key) const;
};

template <typename Type>
class Pool
{
  public:
	Pool() = default;
	explicit Pool(size_t capacity);

	Pool(const Pool& other) = delete;
	Pool(Pool&& other) noexcept = delete;
	Pool& operator=(const Pool& other) = delete;
	Pool& operator=(Pool&& other) noexcept = delete;

	~Pool();

	Pool_Iterator<Pool_Slot<Type>> begin();
	Pool_Iterator<Pool_Slot<Type>> end();
	Pool_Iterator<const Pool_Slot<Type>> begin() const;
	Pool_Iterator<const Pool_Slot<Type>> end() const;

	Pool_Handle<Type> insert(Type element);
	void remove(const Pool_Handle<Type>& handle);
	void resize(size_t capacity);
	Type* get(const Pool_Handle<Type>& handle) const;
	size_t count() const;

  private:
	void fill_free_indices(size_t first_index);

	// TODO: Use custom buffer not std::vector.
	std::vector<Pool_Slot<Type>> slots_;
	std::vector<size_t> free_indices_;
	size_t count_ = 0;
};

template <typename Type>
bool
operator==(const Pool_Handle<Type>& lhs, const Pool_Handle<Type>& rhs)
{
	return lhs.id == rhs.id && lhs.version == rhs.version;
}

template <typename Type>
size_t
Pool_Handle_Hash<Type>::operator()(const Pool_Handle<Type>& key) const
{
	const uint64_t id = static_cast<uint64_t>(key.id) << 32;

	return std::hash<uint64_t>{}(id | key.version);
}

template <typename Type>
Pool<Type>::Pool(const size_t capacity)
{
	resize(capacity);
}

template <typename Type>
Pool<Type>::~Pool()
{
}

template <typename Type>
Pool_Iterator<Pool_Slot<Type>>
Pool<Type>::begin()
{
	return Pool_Iterator<Pool_Slot<Type>>(slots_.data(), slots_.data() + slots_.size());
}

template <typename Type>
Pool_Iterator<Pool_Slot<Type>>
Pool<Type>::end()
{
	return Pool_Iterator<Pool_Slot<Type>>(slots_.data() + slots_.size(), slots_.data() + slots_.size());
}

template <typename Type>
Pool_Iterator<const Pool_Slot<Type>>
Pool<Type>::begin() const
{
	return Pool_Iterator<const Pool_Slot<Type>>(slots_.data(), slots_.data() + slots_.size());
}

template <typename Type>
Pool_Iterator<const Pool_Slot<Type>>
Pool<Type>::end() const
{
	return Pool_Iterator<const Pool_Slot<Type>>(slots_.data() + slots_.size(), slots_.data() + slots_.size());
}

template <typename Type>
Pool_Handle<Type>
Pool<Type>::insert(Type element)
{
	if (free_indices_.empty())
	{
		resize(slots_.empty() ? 10 : slots_.size() * 2);
	}

	if (!BLK_VERIFY(!free_indices_.empty()))
	{
		return {};
	}

	const uint32_t free_index = static_cast<uint32_t>(free_indices_.back());
	free_indices_.pop_back();

	Pool_Slot<Type>& slot = slots_[free_index];
	slot.element = std::move(element);
	slot.is_used = true;

	count_ += 1;

	return Pool_Handle<Type>{.id = free_index, .version = slot.version};
}

template <typename Type>
void
Pool<Type>::remove(const Pool_Handle<Type>& handle)
{
	if (slots_.size() <= handle.id)
	{
		return;
	}

	if (Pool_Slot<Type>& slot = slots_[handle.id]; slot.is_used && handle.version == slot.version)
	{
		// Release the element's resources now instead of holding them until the slot is reused.
		slot.element = Type{};
		slot.is_used = false;
		slot.version += 1;

		count_ -= 1;

		free_indices_.push_back(handle.id);
	}
}

template <typename Type>
void
Pool<Type>::resize(const size_t capacity)
{
	if (capacity <= slots_.size() || !BLK_VERIFY(capacity > 0))
	{
		return;
	}

	const size_t old_capacity = slots_.size();

	// std::vector runs the proper constructors/destructors and moves existing elements safely.
	slots_.resize(capacity);

	fill_free_indices(old_capacity);
}

template <typename Type>
Type*
Pool<Type>::get(const Pool_Handle<Type>& handle) const
{
	if (slots_.size() <= handle.id)
	{
		return nullptr;
	}

	const Pool_Slot<Type>& slot = slots_[handle.id];

	if (!slot.is_used || slot.version != handle.version)
	{
		return nullptr;
	}

	return const_cast<Type*>(&slot.element);
}

template <typename Type>
size_t
Pool<Type>::count() const
{
	return count_;
}

template <typename Type>
void
Pool<Type>::fill_free_indices(const size_t first_index)
{
	for (size_t i = slots_.size(); i > first_index; --i)
	{
		free_indices_.push_back(i - 1);
	}
}
}  // namespace blk
