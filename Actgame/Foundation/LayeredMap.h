#pragma once

#include "Result.h"
#include "TileMap.h"

namespace uchinoko {

enum class MapLayerKind {
	Terrain,
	Visual,
	Object,
	Event
};

class LayeredMap {
public:
	LayeredMap() = default;
	static Result<LayeredMap> Create(TileMap Terrain, TileMap Visual, TileMap Object, TileMap Event);

	const TileMap& Layer(MapLayerKind Kind) const;
	TileMap& Layer(MapLayerKind Kind);
	int Width() const { return Terrain_.Width(); }
	int Height() const { return Terrain_.Height(); }

private:
	TileMap Terrain_;
	TileMap Visual_;
	TileMap Object_;
	TileMap Event_;
};

} // namespace uchinoko
