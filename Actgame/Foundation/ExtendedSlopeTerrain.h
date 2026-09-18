#pragma once

#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

class ExtendedSlopeTerrain {
public:
	struct Slope2x1 {
		int LeftColumn = 0;
		int Row = 0;
		bool UpRight = true;
	};

	static bool TryFind2x1(const TileMap& Map, const TileCatalog& Catalog,
		int WorldX, int WorldY, Slope2x1& Result);
	static float SurfaceY(const Slope2x1& Slope, float WorldX);
	static bool TryCharacterY(const TileMap& Map, const TileCatalog& Catalog,
		float WorldX, float ProbeY, float& CharacterY, Slope2x1* Found = nullptr);
	static bool FollowHorizontal(const TileMap& Map, const TileCatalog& Catalog,
		float OldX, float NewX, float OldY, float& NewY, bool WasGrounded);
	static bool ResolveHighSide(const TileMap& Map, const TileCatalog& Catalog,
		float OldX, float& NewX, float Y, bool MovingRight, bool Grounded);
	static bool ResolveFalling(const TileMap& Map, const TileCatalog& Catalog,
		float X, float OldY, float& NewY);
};

} // namespace uchinoko
