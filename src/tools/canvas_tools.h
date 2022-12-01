#pragma once

#include <memory>

#include "../canvas.h"

namespace tools {
	std::unique_ptr<ICanvasTool> paint();
	std::unique_ptr<ICanvasTool> splatter();
	std::unique_ptr<ICanvasTool> smooth();
}