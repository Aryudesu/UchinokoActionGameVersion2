#pragma once

#include "Result.h"

#include <vector>

namespace uchinoko {

enum class CollisionShape {
	None,
	Solid,
	OneWay,
	SlopeUpRight,
	SlopeUpLeft,
	Stair2x1UpRightLow,
	Stair2x1UpRightHigh,
	Stair2x1UpLeftHigh,
	Stair2x1UpLeftLow,
	Stair1x2UpRightBottom,
	Stair1x2UpRightTop,
	Stair1x2UpLeftTop,
	Stair1x2UpLeftBottom
};

struct TileDefinition {
	int Id = 0;
	int ImageIndex = 0;
	CollisionShape Collision = CollisionShape::None;
	bool Breakable = false;
	bool Damaging = false;
};

class TileCatalog {
public:
	Result<bool> Register(TileDefinition Definition);
	const TileDefinition* Find(int Id) const;

private:
	std::vector<TileDefinition> Definitions_;
};

} // namespace uchinoko
