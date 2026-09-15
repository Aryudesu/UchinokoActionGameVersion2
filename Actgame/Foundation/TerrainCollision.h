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
	static bool TryGetSurfaceY(
		CollisionShape Shape,
		TilePosition Tile,
		float WorldX,
		int TileWidth,
		int TileHeight,
		float& SurfaceY);

	static bool FindGround(
		const TileMap& Map,
		const TileCatalog& Catalog,
		WorldPosition Foot,
		float MaxRise,
		float MaxDrop,
		GroundHit& Hit);
};

} // namespace uchinoko
