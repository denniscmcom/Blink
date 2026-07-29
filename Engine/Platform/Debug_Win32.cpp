#include "Platform/Debug.hpp"

#include <Windows.h>

void
blk::log_debugger(const char* msg)
{
	OutputDebugStringA(msg);
}
