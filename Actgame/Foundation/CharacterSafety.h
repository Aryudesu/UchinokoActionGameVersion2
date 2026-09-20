#pragma once

#include "CharacterController.h"
#include "TileInteraction.h"

#include <vector>

namespace uchinoko {

class TileCatalog;
class TileMap;

struct CharacterSafetyResult {
	bool Repositioned = false;
	bool Crushed = false;
	std::vector<TileEffect> Effects;
};

class CharacterSafety {
public:
	// WorldStateの切替で新しくSolidになったタイルだけを対象にする。
	// 重なった場合は最短方向への押し出しを試し、逃げ場がなければ即死Effectを返す。
	static CharacterSafetyResult ResolveActivatedSolids(
		CharacterController& Controller,
		const TileMap& Map, const TileCatalog& Catalog,
		const std::vector<TilePosition>& ActivatedSolidTiles);

private:
	static bool OverlapsTile(
		const CharacterBody& Body, TilePosition Tile,
		int TileWidth, int TileHeight);
	static bool OverlapsSolid(
		const CharacterBody& Body,
		const TileMap& Map, const TileCatalog& Catalog);
};

} // namespace uchinoko
