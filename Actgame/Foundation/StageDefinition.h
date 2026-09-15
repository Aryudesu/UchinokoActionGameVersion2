#pragma once

#include "Result.h"

#include <string>

namespace uchinoko {

enum class GameMode {
	Action = 0,
	Sokoban = 1
};

Result<GameMode> ParseGameMode(int Value);
const char* ToString(GameMode Mode);

struct StageDefinition {
	int StageNumber = 0;
	GameMode Mode = GameMode::Action;
	std::string DefinitionFile;
};

} // namespace uchinoko
