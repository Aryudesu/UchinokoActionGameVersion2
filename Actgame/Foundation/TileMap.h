#pragma once

#include "Coordinates.h"
#include "GridDataLoader.h"
#include "Result.h"

namespace uchinoko {

class TileMap {
public:
	TileMap() = default;
	static Result<TileMap> Create(IntegerGrid Tiles, int TileWidth = 32, int TileHeight = 32);

	int Width() const;
	int Height() const;
	int TileWidth() const { return TileWidth_; }
	int TileHeight() const { return TileHeight_; }

	bool Contains(TilePosition Position) const;
	bool TryWorldToTile(WorldPosition Position, TilePosition& Result) const;
	const int* TryGet(TilePosition Position) const;
	int* TryGet(TilePosition Position);

private:
	IntegerGrid Tiles_;
	int TileWidth_ = 32;
	int TileHeight_ = 32;
};

} // namespace uchinoko
