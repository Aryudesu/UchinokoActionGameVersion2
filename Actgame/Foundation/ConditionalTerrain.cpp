#include "ConditionalTerrain.h"

#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

int ConditionalTerrain::ReadValue(
	GameStateField Field, const GameStateSnapshot& State) {
	switch (Field) {
	case GameStateField::Coins:
		return State.Coins;
	case GameStateField::Health:
		return State.Health;
	case GameStateField::Lives:
		return State.Lives;
	case GameStateField::Score:
		return State.Score;
	case GameStateField::None:
	default:
		return 0;
	}
}

bool ConditionalTerrain::Compare(
	int Left, ComparisonOperator Operator, int Right) {
	switch (Operator) {
	case ComparisonOperator::Equal:
		return Left == Right;
	case ComparisonOperator::NotEqual:
		return Left != Right;
	case ComparisonOperator::LessThan:
		return Left < Right;
	case ComparisonOperator::LessEqual:
		return Left <= Right;
	case ComparisonOperator::GreaterEqual:
		return Left >= Right;
	case ComparisonOperator::GreaterThan:
		return Left > Right;
	}
	return false;
}

ConditionalTerrainUpdate ConditionalTerrain::Synchronize(
	TileMap& Map, const TileCatalog& Catalog,
	const GameStateSnapshot& State) {
	ConditionalTerrainUpdate Update;

	for (int Row = 0; Row < Map.Height(); ++Row) {
		for (int Column = 0; Column < Map.Width(); ++Column) {
			const TilePosition Position = {Column, Row};
			int* CurrentId = Map.TryGet(Position);
			if (CurrentId == nullptr) continue;

			const TileDefinition* Current = Catalog.Find(*CurrentId);
			if (Current == nullptr ||
				Current->ConditionField == GameStateField::None) continue;

			const int CurrentValue = ReadValue(Current->ConditionField, State);
			const bool ConditionMet =
				Compare(CurrentValue, Current->ConditionOperator,
					Current->ConditionThreshold);
			const int DesiredId = ConditionMet
				? Current->ConditionTrueTileId
				: Current->ConditionFalseTileId;
			if (DesiredId < 0 || DesiredId == *CurrentId) continue;

			const TileDefinition* Desired = Catalog.Find(DesiredId);
			if (Desired == nullptr) continue;

			const bool WasSolid =
				Current->Collision == CollisionShape::Solid;
			const bool IsSolid =
				Desired->Collision == CollisionShape::Solid;

			*CurrentId = DesiredId;
			Update.ChangedTiles.push_back(Position);
			if (!WasSolid && IsSolid) {
				Update.ActivatedSolidTiles.push_back(Position);
			}
		}
	}

	return Update;
}

} // namespace uchinoko
