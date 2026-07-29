#include "Platform/Log.hpp"

#include <stdio.h>

namespace
{
void log_console(const char* msg);
}  // namespace

int
main()
{
	blk::create_log_sink(log_console);
	BLK_DEBUG("Hello from compiler\n");
}

namespace
{
void
log_console(const char* msg)
{
	printf("%s\n", msg);
}
}  // namespace
