#pragma once

#include "Coordinates.h"

#include <vector>

namespace uchinoko {

class TileCatalog;
class TileMap;

enum class GameStateField {
	None,
	Coins,
	Health,
	Lives,
	Score
};

enum class ComparisonOperator {
	Equal,
	NotEqual,
	LessThan,
	LessEqual,
	GreaterEqual,
	GreaterThan
};

struct GameStateSnapshot {
	int Coins = 0;
	int Health = 0;
	int Lives = 0;
	int Score = 0;
};

struct ConditionalTerrainUpdate {
	std::vector<TilePosition> ChangedTiles;
	std::vector<TilePosition> ActivatedSolidTiles;
};

class ConditionalTerrain {
public:
	static ConditionalTerrainUpdate Synchronize(
		TileMap& Map, const TileCatalog& Catalog,
		const GameStateSnapshot& State);

	static int ReadValue(
		GameStateField Field, const GameStateSnapshot& State);
	static bool Compare(
		int Left, ComparisonOperator Operator, int Right);
};

} // namespace uchinoko
