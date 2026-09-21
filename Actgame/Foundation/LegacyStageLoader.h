#pragma once

#include "Coordinates.h"
#include "GridDataLoader.h"
#include "LayeredMap.h"
#include "Result.h"

#include <string>
#include <vector>

namespace uchinoko {

struct LegacyEnemySpawn {
	int EnemyKind = 0;
	int LegacyCode = 0;
	TilePosition Position;
	WorldPosition World;
};

struct LegacyMapMarker {
	int Code = 0;
	TilePosition Position;
};

struct LegacyStageData {
	LayeredMap Layers;
	bool HasPlayerSpawn = false;
	WorldPosition PlayerSpawn;
	std::vector<LegacyEnemySpawn> Enemies;

	// C++版V1のBlockFactory/Player生成規則では直接解釈できない値。
	// HSP版では意味を持った100/110/301/321..325/-79等も、
	// C++版V1互換では実行時に空扱いする。
	// ただし移植調査用メタデータとして元コードは失わず保持する。
	std::vector<LegacyMapMarker> UnresolvedMarkers;
};

class LegacyStageLoader {
public:
	static Result<LegacyStageData> Load(
		const std::string& MapFile,
		const std::string& VisualFile,
		int TileWidth = 32,
		int TileHeight = 32);

	// ファイルI/Oを介さず、旧ARYグリッドをFoundationのLayeredMapへ分解する。
	// テストや将来の変換ツールからも再利用できる。
	static Result<LegacyStageData> Build(
		IntegerGrid LegacyMap,
		IntegerGrid VisualMap,
		int TileWidth = 32,
		int TileHeight = 32);
};

} // namespace uchinoko
