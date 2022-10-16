#pragma once

#include <cstdint>
#include "terrapainter/math.h"

struct RGBu8 : public math::MVector<uint8_t, 3> {
	constexpr RGBu8() : math::MVector<uint8_t, 3>() {}
	constexpr RGBu8(uint8_t r, uint8_t g, uint8_t b) : math::MVector<uint8_t, 3>(r, g, b) {}
	template<std::convertible_to<uint8_t> T>
	explicit constexpr RGBu8(const math::MVector<T, 3>& other) : math::MVector<uint8_t, 3>(other) {}
};