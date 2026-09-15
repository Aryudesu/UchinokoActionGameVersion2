#include "TileMap.h"

#include <cmath>
#include <utility>

namespace uchinoko {

Result<TileMap> TileMap::Create(IntegerGrid Tiles, int TileWidth, int TileHeight) {
	if (Tiles.empty() || Tiles.front().empty()) {
		return Result<TileMap>::Failure("Tile map must not be empty");
	}
	if (TileWidth <= 0 || TileHeight <= 0) {
		return Result<TileMap>::Failure("Tile size must be positive");
	}
	const std::size_t Width = Tiles.front().size();
	for (std::size_t Row = 0; Row < Tiles.size(); ++Row) {
		if (Tiles[Row].size() != Width) {
			return Result<TileMap>::Failure(
				"Tile map row " + std::to_string(Row + 1) + " has a different width");
		}
	}

	TileMap Map;
	Map.Tiles_ = std::move(Tiles);
	Map.TileWidth_ = TileWidth;
	Map.TileHeight_ = TileHeight;
	return Result<TileMap>::Success(std::move(Map));
}

int TileMap::Width() const {
	return Tiles_.empty() ? 0 : static_cast<int>(Tiles_.front().size());
}

int TileMap::Height() const {
	return static_cast<int>(Tiles_.size());
}

bool TileMap::Contains(TilePosition Position) const {
	return Position.Row >= 0 && Position.Row < Height() &&
		Position.Column >= 0 && Position.Column < Width();
}

bool TileMap::TryWorldToTile(WorldPosition Position, TilePosition& Result) const {
	if (Position.X < 0.0f || Position.Y < 0.0f) return false;
	TilePosition Converted;
	Converted.Column = static_cast<int>(std::floor(Position.X / TileWidth_));
	Converted.Row = static_cast<int>(std::floor(Position.Y / TileHeight_));
	if (!Contains(Converted)) return false;
	Result = Converted;
	return true;
}

const int* TileMap::TryGet(TilePosition Position) const {
	if (!Contains(Position)) return nullptr;
	return &Tiles_[Position.Row][Position.Column];
}

int* TileMap::TryGet(TilePosition Position) {
	if (!Contains(Position)) return nullptr;
	return &Tiles_[Position.Row][Position.Column];
}

} // namespace uchinoko
