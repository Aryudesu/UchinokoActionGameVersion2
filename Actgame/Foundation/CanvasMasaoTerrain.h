#pragma once

#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

class CanvasMasaoTerrain {
public:
	static constexpr int EmptyCode = 0;
	static constexpr int OneWayCode = 15;
	static constexpr int SlopeUpRightCode = 18;
	static constexpr int SlopeUpLeftCode = 19;
	static constexpr int SolidCode = 20;

	static int CodeAt(const TileMap& Map, const TileCatalog& Catalog, int X, int Y);
	static int CodeFor(CollisionShape Shape);
	static bool IsSlope(int Code);
	static bool IsSolid(int Code);
	static int RoundDown(double Value);
	static int GetSakamichiY(const TileMap& Map, const TileCatalog& Catalog, int X, int Y);

	static bool ResolveHorizontalSolid(const TileMap& Map, const TileCatalog& Catalog,
		int& X, int Y, bool MovingRight);
	static bool ResolveVerticalSolid(const TileMap& Map, const TileCatalog& Catalog,
		int X, int& Y, bool MovingDown);
	static bool FollowHorizontalSlope(const TileMap& Map, const TileCatalog& Catalog,
		int OldX, int NewX, int& Y, int VelocityX10, int& VelocityY10, bool& Grounded);
	static bool ResolveHorizontalSlopeSide(const TileMap& Map, const TileCatalog& Catalog,
		int OldX, int& NewX, int Y, bool MovingRight, bool Grounded = false);
	static bool ResolveRisingSlope(const TileMap& Map, const TileCatalog& Catalog,
		int X, int OldY, int& NewY);
	static bool ResolveFallingSlope(const TileMap& Map, const TileCatalog& Catalog,
		int X, int OldY, int& NewY);
	static bool ResolveFallingOneWay(const TileMap& Map, const TileCatalog& Catalog,
		int X, int OldY, int& NewY);
	static bool ResolveDirectionalVerticalSolid(const TileMap& Map, const TileCatalog& Catalog,
		int& X, int OldY, int& NewY, int Direction, bool MovingDown);
};

} // namespace uchinoko
