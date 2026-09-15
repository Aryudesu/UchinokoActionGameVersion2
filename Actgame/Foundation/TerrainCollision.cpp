#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>

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
	case CollisionShape::Stair2x1UpRightLow:
		SurfaceY = Top + static_cast<float>(TileHeight) * (1.0f - Ratio * 0.5f);
		return true;
	case CollisionShape::Stair2x1UpRightHigh:
		SurfaceY = Top + static_cast<float>(TileHeight) * (0.5f - Ratio * 0.5f);
		return true;
	case CollisionShape::Stair2x1UpLeftHigh:
		SurfaceY = Top + static_cast<float>(TileHeight) * Ratio * 0.5f;
		return true;
	case CollisionShape::Stair2x1UpLeftLow:
		SurfaceY = Top + static_cast<float>(TileHeight) * (0.5f + Ratio * 0.5f);
		return true;
	case CollisionShape::Stair1x2UpRightBottom:
		if (Ratio > 0.5f) return false;
		SurfaceY = Top + static_cast<float>(TileHeight) * (1.0f - Ratio * 2.0f);
		return true;
	case CollisionShape::Stair1x2UpRightTop:
		if (Ratio < 0.5f) return false;
		SurfaceY = Top + static_cast<float>(TileHeight) * (2.0f - Ratio * 2.0f);
		return true;
	case CollisionShape::Stair1x2UpLeftTop:
		if (Ratio > 0.5f) return false;
		SurfaceY = Top + static_cast<float>(TileHeight) * Ratio * 2.0f;
		return true;
	case CollisionShape::Stair1x2UpLeftBottom:
		if (Ratio < 0.5f) return false;
		SurfaceY = Top + static_cast<float>(TileHeight) * (Ratio * 2.0f - 1.0f);
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
		if (!Found || SurfaceY < Hit.SurfaceY) {
			Found = true;
			Hit.Tile = Position;
			Hit.SurfaceY = SurfaceY;
			Hit.Shape = Definition->Collision;
		}
	}
	return Found;
}

} // namespace uchinoko
