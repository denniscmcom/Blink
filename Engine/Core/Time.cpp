#include "Core/Time.hpp"

#include <chrono>

namespace
{
std::chrono::time_point<std::chrono::steady_clock> timer;
}  // namespace

void
blk::start_global_timer()
{
	timer = std::chrono::steady_clock::now();
}

double
blk::get_global_timer_seconds()
{
	auto current = std::chrono::steady_clock::now();

	return std::chrono::duration<double>(current - timer).count();
}
