// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stdint.h>

namespace blk
{
/// Maximum length in characters for a path.
constexpr size_t MAX_PATH_SIZE = 1'024;

enum class Result;
struct Allocator;

/// Opaque handle to a file.
struct File;

/// Mode to open a file.
enum class File_Access_Mode
{
	READ,
	WRITE,
};

/// Opens a file.
///
/// If `mode` is `File_Access_Mode::READ`, `path` should point to an already existing file.
/// If `mode` is `File_Access_Mode::WRITE` and `path` does not exist, it will create a new file on disk.
///
/// `path` should be a valid absolute path.
/// @warning Trying to open a file that is already open will fail.
Result open_file(Allocator* allocator, const char* path, File_Access_Mode mode, File*& file);
/// Closes `file`.
Result close_file(File* file);
/// Gets the size in bytes of `file`.
Result get_file_size(const File* file, uint64_t& size);
/// Reads `size` bytes from `file` and writes the result to `buffer`.
///
/// `buffer` should be a valid pre-allocated buffer that fits `size` bytes. Use `get_file_size` to allocate it.
Result read_file(const File* file, char* buffer, uint64_t size);
/// Writes `size` bytes from `buffer` to `file`.
///
/// `file` should be opened with `mode` set to `File_Access_Mode::WRITE`.
Result write_file(const File* file, const char* buffer, uint64_t size);
}  // namespace blk
