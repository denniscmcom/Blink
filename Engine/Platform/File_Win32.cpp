// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/File.hpp"

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <Windows.h>

namespace blk
{
/// Win32 file.
struct File
{
	/// `Allocator` used to create `File`.
	Allocator* allocator;
	/// `File_Access_Mode` used when calling `open_file`.
	File_Access_Mode mode;
	/// Win32 file handle.
	HANDLE handle;
};
}  // namespace blk

blk::Result
blk::open_file(Allocator* allocator, const char* path, File_Access_Mode mode, File*& file)
{
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(path))
	{
		return Result::INVALID_ARGUMENTS;
	}

	wchar_t wpath[MAX_PATH];

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH) <= 0)
	{
		return Result::INVALID_ARGUMENTS;
	}

	DWORD access_mode = 0;
	DWORD create_disposition = 0;

	switch (mode)
	{
	case File_Access_Mode::READ:
		access_mode = GENERIC_READ;
		create_disposition = OPEN_EXISTING;
		break;
	case File_Access_Mode::WRITE:
		access_mode = GENERIC_WRITE;
		// `CREATE_ALWAYS` truncates an existing file to zero length. `OPEN_ALWAYS` does not, so writing a file smaller
		// than the one it replaces leaves the previous file's tail behind.
		create_disposition = CREATE_ALWAYS;
		break;
	}

	auto handle = INVALID_HANDLE_VALUE;

	if (handle = CreateFileW(wpath, access_mode, 0, nullptr, create_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
		handle == INVALID_HANDLE_VALUE)
	{
		return Result::OS_ERROR;
	}

	void* pointer = nullptr;

	if (const Result result = allocate(*allocator, pointer, sizeof(File), alignof(File)); result != Result::SUCCESS)
	{
		return result;
	}

	file = static_cast<File*>(pointer);
	file->allocator = allocator;
	file->handle = handle;
	file->mode = mode;

	return Result::SUCCESS;
}

blk::Result
blk::close_file(File* file)
{
	if (!BLK_VERIFY(file))
	{
		return Result::INVALID_ARGUMENTS;
	}

	BLK_CHECK(file->allocator);

	if (CloseHandle(file->handle) == 0)
	{
		return Result::OS_ERROR;
	}

	free(*file->allocator, file);

	return Result::SUCCESS;
}

blk::Result
blk::get_file_size(const File* file, uint64_t& size)
{
	if (!BLK_VERIFY(file))
	{
		return Result::INVALID_ARGUMENTS;
	}

	LARGE_INTEGER file_size = {};

	if (!GetFileSizeEx(file->handle, &file_size))
	{
		return Result::OS_ERROR;
	}

	size = static_cast<uint64_t>(file_size.QuadPart);

	return Result::SUCCESS;
}

blk::Result
blk::read_file(const File* file, char* buffer, uint64_t size)
{
	if (!BLK_VERIFY(file) || !BLK_VERIFY(buffer))
	{
		return Result::INVALID_ARGUMENTS;
	}

	DWORD bytes_read = 0;

	// FIXME: We are implicitly casting `uint64_t size` — aka `unsigned long long` — to `unsigned long size`.
	if (ReadFile(file->handle, buffer, size, &bytes_read, nullptr) == 0)
	{
		return Result::OS_ERROR;
	}

	if (static_cast<uint64_t>(bytes_read) != size)
	{
		return Result::INVALID_ARGUMENTS;
	}

	return Result::SUCCESS;
}

blk::Result
blk::write_file(const File* file, const char* buffer, uint64_t size)
{
	if (!BLK_VERIFY(file) || !BLK_VERIFY(buffer) || file->mode != File_Access_Mode::WRITE)
	{
		return Result::INVALID_ARGUMENTS;
	}

	DWORD bytes_written = 0;

	// FIXME: We are implicitly casting `uint64_t size` — aka `unsigned long long` — to `unsigned long size`.
	if (WriteFile(file->handle, buffer, size, &bytes_written, nullptr) == 0)
	{
		return Result::OS_ERROR;
	}

	if (static_cast<uint64_t>(bytes_written) != size)
	{
		return Result::INVALID_ARGUMENTS;
	}

	return Result::SUCCESS;
}
