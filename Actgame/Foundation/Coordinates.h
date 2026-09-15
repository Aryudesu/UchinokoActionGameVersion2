#pragma once

namespace uchinoko {

struct WorldPosition {
	float X = 0.0f;
	float Y = 0.0f;
};

struct TilePosition {
	int Column = 0;
	int Row = 0;
};

inline bool operator==(const TilePosition& Left, const TilePosition& Right) {
	return Left.Column == Right.Column && Left.Row == Right.Row;
}

inline bool operator!=(const TilePosition& Left, const TilePosition& Right) {
	return !(Left == Right);
}

} // namespace uchinoko
