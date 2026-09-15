#include "LayeredMap.h"

#include <utility>

namespace uchinoko {

Result<LayeredMap> LayeredMap::Create(TileMap Terrain, TileMap Visual, TileMap Object, TileMap Event) {
	const int Width = Terrain.Width();
	const int Height = Terrain.Height();
	const int TileWidth = Terrain.TileWidth();
	const int TileHeight = Terrain.TileHeight();
	const TileMap* Layers[] = {&Visual, &Object, &Event};
	for (const TileMap* Layer : Layers) {
		if (Layer->Width() != Width || Layer->Height() != Height ||
			Layer->TileWidth() != TileWidth || Layer->TileHeight() != TileHeight) {
			return Result<LayeredMap>::Failure("All map layers must have the same dimensions and tile size");
		}
	}

	LayeredMap Map;
	Map.Terrain_ = std::move(Terrain);
	Map.Visual_ = std::move(Visual);
	Map.Object_ = std::move(Object);
	Map.Event_ = std::move(Event);
	return Result<LayeredMap>::Success(std::move(Map));
}

const TileMap& LayeredMap::Layer(MapLayerKind Kind) const {
	switch (Kind) {
	case MapLayerKind::Terrain: return Terrain_;
	case MapLayerKind::Visual: return Visual_;
	case MapLayerKind::Object: return Object_;
	case MapLayerKind::Event: return Event_;
	}
	return Terrain_;
}

TileMap& LayeredMap::Layer(MapLayerKind Kind) {
	return const_cast<TileMap&>(static_cast<const LayeredMap&>(*this).Layer(Kind));
}

} // namespace uchinoko
