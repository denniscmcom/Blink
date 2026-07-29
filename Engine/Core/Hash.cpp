#include "Core/Hash.hpp"

#include <string.h>

namespace
{
constexpr uint64_t FNV_OFFSET_BASIS = 0xCBF29CE484222325;
constexpr uint64_t FNV_PRIME = 0x100000001B3;
}  // namespace

uint64_t
blk::hash_fnv1a(const char* buffer, const size_t size)
{
	uint64_t hash = FNV_OFFSET_BASIS;

	for (size_t i = 0; i < size; i++)
	{
		hash ^= static_cast<uint64_t>(buffer[i]);
		hash *= FNV_PRIME;
	}

	return hash;
}

uint64_t
blk::hash_fnv1a(const char* string)
{
	return hash_fnv1a(string, strlen(string));
}
