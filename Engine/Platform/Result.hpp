// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Log.hpp"

// TODO (Consistency): We are not using these macros in many parts of the codebase.

/// If `expression` does not evaluates to `Result::SUCCESS` it returns `Result`.
///
/// It's used in functions that return `Result` and call other functions that also returns `Result` too. For example:
///
/// ```
/// struct Foo
/// {
///		Dyn_Array<int> numbers;
/// };
///
/// Result create_Foo(Foo& foo, Allocator* allocator)
/// {
///		if (const Result result = create_dyn_array(foo.numbers, allocator, 10); result != Result::SUCCESS)
///		{
///			return result;
///		}
///
///		return Result::SUCCESS;
/// }
///
/// ```
///
/// Instead, we could write:
///
/// ```
/// Result create_Foo(Foo& foo, Allocator* allocator)
/// {
///		BLK_SUCCESS_OR_RETURN(create_dyn_array(foo.numbers, allocator, 10));
///
///		return Result::SUCCESS;
/// }
/// ```
#define BLK_SUCCESS_OR_RETURN(expression)                                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		if (const blk::Result result = (expression); result != blk::Result::SUCCESS)                                   \
		{                                                                                                              \
			return result;                                                                                             \
		}                                                                                                              \
	} while (0)

/// If `expression` is not `Result::SUCCESS` it logs an error and returns `Result`.
#define BLK_SUCCESS_OR_ERROR_RETURN(expression, ...)                                                                   \
	do                                                                                                                 \
	{                                                                                                                  \
		if (const blk::Result result = (expression); result != blk::Result::SUCCESS)                                   \
		{                                                                                                              \
			BLK_ERROR(__VA_ARGS__);                                                                                    \
			return result;                                                                                             \
		}                                                                                                              \
	} while (0)

/// If `expression` is not `Result::SUCCESS` it executes the body.
///
/// It is used to avoid repetition when calling functions that return `Result` and only is important knowing if it
/// succeeded or not.
#define BLK_IF_NOT_SUCCESS(expression) if ((expression) != blk::Result::SUCCESS)

namespace blk
{
/// Return type for functions that can fail. It follows operating system conventions where `0` means success, positive
/// values are warnings or incomplete states, and negative values represent runtime errors.
enum class Result
{
	SUCCESS = 0,
	/// An allocation is required but there is not enough memory.
	OUT_OF_MEMORY = -1,
	/// Some or all arguments are invalid or corrupted.
	INVALID_ARGUMENTS = -2,
	/// An out of bounds memory access was about to happen.
	OUT_OF_BOUNDS = -3,
	/// A call to an OS function failed.
	OS_ERROR = -4,
	/// File extension and file magic number are different.
	INVALID_FILE_FORMAT = -5,
	/// Generic GPU error.
	/// Something went wrong with the GPU: a call to the graphics API failed, a resource cannot be created, etc.
	DEVICE_ERROR = -6,
};
}  // namespace blk
