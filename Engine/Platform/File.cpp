// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/File.hpp"

#include "Engine/Platform/Assert.hpp"

#include <stdio.h>

namespace blk
{
struct File
{
	const char* path;
	FILE* pointer;
	File_Access_Mode mode;
};
}  // namespace blk

blk::File*
blk::open_file(const char* path, File_Access_Mode mode)
{
	FILE* pointer = nullptr;

	switch (mode)
	{
	case File_Access_Mode::READ:
		pointer = fopen(path, "rb");
		break;
	case File_Access_Mode::WRITE:
		pointer = fopen(path, "wb");
		break;
	}

	if (!pointer)
	{
		return nullptr;
	}

	auto file = new File();
	file->path = path;
	file->pointer = pointer;
	file->mode = mode;

	return file;
}

void
blk::close_file(File* file)
{
	if (!BLK_VERIFY(file))
	{
		return;
	}

	if (fclose(file->pointer) != 0)
	{
		BLK_ERROR("Failed to close file: %s\n", file->path);

		return;
	}

	delete file;
}

uint64_t
blk::get_file_size(File* file)
{
	if (!BLK_VERIFY(file))
	{
		return 0;
	}

	if (fseek(file->pointer, 0, SEEK_END) != 0)
	{
		BLK_ERROR("Failed to move file pointer to the end of the file\n");
	}

	const long file_size = ftell(file->pointer);

	if (fseek(file->pointer, 0, SEEK_SET) != 0)
	{
		BLK_ERROR("Failed to move file pointer to the start of the file\n");
	}

	return static_cast<uint64_t>(file_size);
}

void
blk::read_file(File* file, char* buffer, uint64_t size)
{
	if (!BLK_VERIFY(file) || !BLK_VERIFY(buffer))
	{
		return;
	}

	if (fread(buffer, 1, size, file->pointer) < size)
	{
		BLK_ERROR("Failed to read file\n");
	}
}

void
blk::write_file(File* file, char* buffer, uint64_t size)
{
	if (!BLK_VERIFY(file) || !BLK_VERIFY(buffer))
	{
		return;
	}

	if (const size_t written = fwrite(buffer, 1, size, file->pointer); written != size)
	{
		BLK_ERROR("Failed to write file\n");
	}
}
