#include "StageDefinition.h"

namespace uchinoko {

Result<GameMode> ParseGameMode(int Value) {
	switch (Value) {
	case 0:
		return Result<GameMode>::Success(GameMode::Action);
	case 1:
		return Result<GameMode>::Success(GameMode::Sokoban);
	default:
		return Result<GameMode>::Failure("Unknown GameMode: " + std::to_string(Value));
	}
}

const char* ToString(GameMode Mode) {
	switch (Mode) {
	case GameMode::Action:
		return "Action";
	case GameMode::Sokoban:
		return "Sokoban";
	default:
		return "Unknown";
	}
}

} // namespace uchinoko
