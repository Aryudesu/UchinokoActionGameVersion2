#pragma once

#include "Coordinates.h"
#include "Result.h"
#include "PipeTransport.h"
#include "TileDefinition.h"
#include "TileMap.h"

#include <string>
#include <vector>

namespace uchinoko {

struct TerrainStageData {
	TileMap Map;
	TileCatalog Catalog;
	std::vector<PipeLink> Pipes;
	WorldPosition PlayerSpawn;
};

class TerrainStageLoader {
public:
	// Manifest 内の terrain と tiles は ManifestFile からの相対パスで解決する。
	static Result<TerrainStageData> Load(const std::string& ManifestFile);
	static Result<TileCatalog> LoadCatalog(const std::string& FileName);
	static Result<std::vector<PipeLink>> LoadPipes(
		const std::string& FileName, int TileWidth = 32, int TileHeight = 32);
};

} // namespace uchinoko
