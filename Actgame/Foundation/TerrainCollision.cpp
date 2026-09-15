#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace uchinoko {

bool TerrainCollision::TryGetSurfaceY(
	CollisionShape Shape,
	TilePosition Tile,
	float WorldX,
	int TileWidth,
	int TileHeight,
	float& SurfaceY) {
	if (Shape == CollisionShape::None || TileWidth <= 0 || TileHeight <= 0) return false;

	const float Left = static_cast<float>(Tile.Column * TileWidth);
	const float Top = static_cast<float>(Tile.Row * TileHeight);
	const float LocalX = std::max(0.0f, std::min(static_cast<float>(TileWidth), WorldX - Left));
	const float Ratio = LocalX / static_cast<float>(TileWidth);

	switch (Shape) {
	case CollisionShape::Solid:
	case CollisionShape::OneWay:
		SurfaceY = Top;
		return true;
	case CollisionShape::SlopeUpRight:
		SurfaceY = Top + static_cast<float>(TileHeight) * (1.0f - Ratio);
		return true;
	case CollisionShape::SlopeUpLeft:
		SurfaceY = Top + static_cast<float>(TileHeight) * Ratio;
		return true;
	case CollisionShape::None:
		return false;
	}
	return false;
}

bool TerrainCollision::FindGround(
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition Foot,
	float MaxRise,
	float MaxDrop,
	GroundHit& Hit) {
	if (MaxRise < 0.0f || MaxDrop < 0.0f || Foot.X < 0.0f) return false;

	const int Column = static_cast<int>(std::floor(Foot.X / Map.TileWidth()));
	if (Column < 0 || Column >= Map.Width()) return false;
	const int FirstRow = std::max(0, static_cast<int>(std::floor((Foot.Y - MaxRise) / Map.TileHeight())) - 1);
	const int LastRow = std::min(Map.Height() - 1,
		static_cast<int>(std::floor((Foot.Y + MaxDrop) / Map.TileHeight())) + 1);

	bool Found = false;
	float BestDistance = std::numeric_limits<float>::max();
	for (int Row = FirstRow; Row <= LastRow; ++Row) {
		const TilePosition Position = {Column, Row};
		const int* Id = Map.TryGet(Position);
		const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr) continue;

		float SurfaceY = 0.0f;
		if (!TryGetSurfaceY(Definition->Collision, Position, Foot.X,
			Map.TileWidth(), Map.TileHeight(), SurfaceY)) continue;
		const float Distance = SurfaceY - Foot.Y;
		if (Distance < -MaxRise || Distance > MaxDrop) continue;
		if (!Found || std::fabs(Distance) < std::fabs(BestDistance)) {
			Found = true;
			BestDistance = Distance;
			Hit.Tile = Position;
			Hit.SurfaceY = SurfaceY;
			Hit.Shape = Definition->Collision;
		}
	}
	return Found;
}

} // namespace uchinoko
