#pragma once

#include "Coordinates.h"
#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

struct GroundHit {
	TilePosition Tile;
	float SurfaceY = 0.0f;
	CollisionShape Shape = CollisionShape::None;
};

class TerrainCollision {
public:
	enum class TileSide {
		Left,
		Right
	};

	static bool TryGetSurfaceY(
		CollisionShape Shape,
		TilePosition Tile,
		float WorldX,
		int TileWidth,
		int TileHeight,
		float& SurfaceY);

	// 点が床・坂・階段の実体部分に含まれるかを返す。すり抜け床は実体に含めない。
	static bool ContainsSolidPoint(
		CollisionShape Shape,
		TilePosition Tile,
		WorldPosition Point,
		int TileWidth,
		int TileHeight);

	// タイルの左右端に実体がある場合、その垂直区間を返す。
	static bool TryGetSideBlock(
		CollisionShape Shape,
		TilePosition Tile,
		TileSide Side,
		int TileWidth,
		int TileHeight,
		float& BlockTop,
		float& BlockBottom);

	static bool FindGround(
		const TileMap& Map,
		const TileCatalog& Catalog,
		WorldPosition Foot,
		float MaxRise,
		float MaxDrop,
		GroundHit& Hit);
};

} // namespace uchinoko
