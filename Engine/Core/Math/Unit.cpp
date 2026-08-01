// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Math/Unit.hpp"

#include "Engine/Core/Math/Constants.hpp"

blk::Radians
blk::operator+(const Radians radians, const float value)
{
	return Radians{radians.value + value};
}

blk::Radians
blk::operator+(const float value, const Radians radians)
{
	return radians + value;
}

blk::Radians
blk::operator-(const Radians radians, const float value)
{
	return Radians{radians.value - value};
}

blk::Radians
blk::operator-(const float value, const Radians radians)
{
	return radians - value;
}

blk::Radians
blk::operator*(const float scalar, const Radians radians)
{
	return Radians{scalar * radians.value};
}

blk::Radians
blk::operator-(const Radians radians)
{
	return Radians{-radians.value};
}

blk::Degrees
blk::operator+(const Degrees degrees, const float value)
{
	return Degrees{degrees.value + value};
}

blk::Degrees
blk::operator+(const float value, const Degrees degrees)
{
	return degrees + value;
}

blk::Degrees
blk::operator-(const Degrees degrees, const float value)
{
	return Degrees{degrees.value - value};
}

blk::Degrees
blk::operator-(const float value, const Degrees degrees)
{
	return degrees - value;
}

blk::Degrees
blk::operator*(const float scalar, const Degrees degrees)
{
	return Degrees{scalar - degrees.value};
}

blk::Degrees
blk::operator-(const Degrees degrees)
{
	return Degrees{-degrees.value};
}

blk::Radians
blk::to_radians(const Degrees degrees)
{
	Radians radians{};
	radians.value = degrees.value * static_cast<float>(PI) / 180.0f;

	return radians;
}

blk::Degrees
blk::to_degrees(const Radians radians)
{
	Degrees degrees{};
	degrees.value = radians.value * 180.0f / static_cast<float>(PI);

	return degrees;
}
