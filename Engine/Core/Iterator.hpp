#pragma once

#include <stdint.h>

namespace blk
{
template <typename Type>
struct Pool_Slot
{
	Type element;
	bool is_used = false;
	uint32_t version = 0;
};

template <typename Slot>
class Pool_Iterator
{
  public:
	Pool_Iterator(Slot* slot, Slot* end);

	auto& operator*() const;
	auto* operator->() const;
	Pool_Iterator& operator++();

	bool operator!=(const Pool_Iterator& other) const;
	bool operator==(const Pool_Iterator& other) const;

  private:
	void skip_unused();

	Slot* slot_;
	Slot* end_;
};

template <typename Slot>
Pool_Iterator<Slot>::Pool_Iterator(Slot* slot, Slot* end) : slot_(slot), end_(end)
{
	// We skip until the first used slot.
	skip_unused();
}

template <typename Slot>
auto&
Pool_Iterator<Slot>::operator*() const
{
	return slot_->element;
}

template <typename Slot>
auto*
Pool_Iterator<Slot>::operator->() const
{
	return &slot_->element;
}

template <typename Slot>
Pool_Iterator<Slot>&
Pool_Iterator<Slot>::operator++()
{
	++slot_;
	skip_unused();

	return *this;
}

template <typename Slot>
bool
Pool_Iterator<Slot>::operator!=(const Pool_Iterator& other) const
{
	return slot_ != other.slot_;
}

template <typename Slot>
bool
Pool_Iterator<Slot>::operator==(const Pool_Iterator& other) const
{
	return slot_ == other.slot_;
}

template <typename Slot>
void
Pool_Iterator<Slot>::skip_unused()
{
	while (slot_ != end_ && !slot_->is_used)
	{
		++slot_;
	}
}
}  // namespace blk
