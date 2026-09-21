#pragma once

#include "Result.h"
#include "StageData.h"

#include <string>

namespace uchinoko {

class NativeStageDataLoader {
public:
	static Result<StageData> Load(const std::string& FileName);
	static Result<StageData> Parse(
		const std::string& JsonText,
		const std::string& BaseDirectory = "");
};

} // namespace uchinoko
