// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stdint.h>

namespace blk
{
struct File;

enum class File_Access_Mode
{
	READ,
	WRITE,
};

File* open_file(const char* path, File_Access_Mode mode);
void close_file(const File* file);
uint64_t get_file_size(File* file);
void read_file(File* file, char* buffer, uint64_t size);
void write_file(File* file, char* buffer, uint64_t size);
}  // namespace blk
