#pragma once

#include <cstdint>
#include "terrapainter/math.h"

struct RGBu8 : public math::MVector<uint8_t, 3> {
	RGBu8() : math::MVector<uint8_t, 3>() {}
	RGBu8(uint8_t r, uint8_t g, uint8_t b) : math::MVector<uint8_t, 3>(r, g, b) {}
};