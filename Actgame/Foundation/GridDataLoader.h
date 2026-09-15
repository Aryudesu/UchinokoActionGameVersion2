#pragma once

#include "Result.h"

#include <string>
#include <vector>

namespace uchinoko {

using IntegerGrid = std::vector<std::vector<int>>;

class GridDataLoader {
public:
	static Result<IntegerGrid> Load(const std::string& FileName);
	static Result<IntegerGrid> Parse(const std::string& Text);
};

} // namespace uchinoko
