#pragma once

#include <memory>

#include "../canvas.h"

namespace tools {
	std::unique_ptr<ICanvasTool> paint();
}