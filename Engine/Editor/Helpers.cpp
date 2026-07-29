#include "Editor/Helpers.hpp"

#include "Platform/Application.hpp"

ImVec2
blk::to_imvec2(const Rect<float>& rect)
{
	ImVec2 vec = {};
	vec.x = rect.x;
	vec.y = rect.y;

	return vec;
}

blk::Rect<float>
blk::to_rect2(const ImVec2& vec)
{
	return Rect{.x = vec.x, .y = vec.y};
}
