#include "LegacyStageLoader.h"

#include <utility>

namespace uchinoko {
namespace {

bool IsCurrentV1TileCode(int Code) {
	// 現行Version1のMap::LoadMapは0..50をBlockFactoryへ渡すが、
	// BlockFactoryが意味を持つのは0..45まで。46..50はEmptyへ落ちるため、
	// 互換ローダーでは「未解決」として保持して情報を捨てない。
	return Code >= 0 && Code <= 45;
}

bool TryEnemyKind(int Code, int& EnemyKind) {
	if (Code <= -2 && Code >= -6) {
		EnemyKind = -Code - 1;
		return true;
	}
	return false;
}

Result<TileMap> MakeMap(IntegerGrid Grid, int TileWidth, int TileHeight) {
	return TileMap::Create(std::move(Grid), TileWidth, TileHeight);
}

} // namespace

Result<LegacyStageData> LegacyStageLoader::Load(
	const std::string& MapFile,
	const std::string& VisualFile,
	int TileWidth,
	int TileHeight) {
	Result<IntegerGrid> MapGrid = GridDataLoader::Load(MapFile);
	if (MapGrid.IsFailure()) {
		return Result<LegacyStageData>::Failure(MapGrid.Error());
	}

	Result<IntegerGrid> VisualGrid = GridDataLoader::Load(VisualFile);
	if (VisualGrid.IsFailure()) {
		return Result<LegacyStageData>::Failure(VisualGrid.Error());
	}

	return Build(
		std::move(MapGrid.Value()),
		std::move(VisualGrid.Value()),
		TileWidth,
		TileHeight);
}

Result<LegacyStageData> LegacyStageLoader::Build(
	IntegerGrid LegacyMap,
	IntegerGrid VisualMap,
	int TileWidth,
	int TileHeight) {
	if (TileWidth <= 0 || TileHeight <= 0) {
		return Result<LegacyStageData>::Failure("Tile size must be positive");
	}
	if (LegacyMap.empty() || LegacyMap.front().empty()) {
		return Result<LegacyStageData>::Failure("Legacy map must not be empty");
	}
	if (VisualMap.empty() || VisualMap.front().empty()) {
		return Result<LegacyStageData>::Failure("Legacy visual map must not be empty");
	}

	const std::size_t Height = LegacyMap.size();
	const std::size_t Width = LegacyMap.front().size();
	for (std::size_t Row = 0; Row < Height; ++Row) {
		if (LegacyMap[Row].size() != Width) {
			return Result<LegacyStageData>::Failure(
				"Legacy map rows must have the same width");
		}
	}
	if (VisualMap.size() != Height) {
		return Result<LegacyStageData>::Failure(
			"Legacy map and visual map must have the same dimensions");
	}
	for (std::size_t Row = 0; Row < Height; ++Row) {
		if (VisualMap[Row].size() != Width) {
			return Result<LegacyStageData>::Failure(
				"Legacy map and visual map must have the same dimensions");
		}
	}

	IntegerGrid Terrain(
		Height, std::vector<int>(Width, 0));
	IntegerGrid Object(
		Height, std::vector<int>(Width, 0));
	IntegerGrid Event(
		Height, std::vector<int>(Width, 0));

	LegacyStageData Stage;

	for (std::size_t Row = 0; Row < Height; ++Row) {
		for (std::size_t Column = 0; Column < Width; ++Column) {
			const int Code = LegacyMap[Row][Column];
			const TilePosition Position{
				static_cast<int>(Column),
				static_cast<int>(Row)
			};

			if (IsCurrentV1TileCode(Code)) {
				Terrain[Row][Column] = Code;
				continue;
			}

			if (Code == -1) {
				// V1も走査順にSetInitPosしていたため、複数ある場合は最後を採用する。
				Stage.HasPlayerSpawn = true;
				Stage.PlayerSpawn = {
					static_cast<float>(Column * static_cast<std::size_t>(TileWidth)),
					static_cast<float>(Row * static_cast<std::size_t>(TileHeight))
				};
				continue;
			}

			int EnemyKind = 0;
			if (TryEnemyKind(Code, EnemyKind)) {
				Object[Row][Column] = EnemyKind;

				LegacyEnemySpawn Spawn;
				Spawn.EnemyKind = EnemyKind;
				Spawn.LegacyCode = Code;
				Spawn.Position = Position;
				Spawn.WorldPosition = {
					static_cast<float>(Column * static_cast<std::size_t>(TileWidth)),
					static_cast<float>(Row * static_cast<std::size_t>(TileHeight))
				};
				Stage.Enemies.push_back(Spawn);
				continue;
			}

			// 301=Goal、321..325、-79等の旧エディタ/未移植コードを
			// 空白へ潰さずEventレイヤと一覧の両方へ残す。
			Event[Row][Column] = Code;
			LegacyMapMarker Marker;
			Marker.Code = Code;
			Marker.Position = Position;
			Stage.UnresolvedMarkers.push_back(Marker);
		}
	}

	Result<TileMap> TerrainMap =
		MakeMap(std::move(Terrain), TileWidth, TileHeight);
	if (TerrainMap.IsFailure()) {
		return Result<LegacyStageData>::Failure(TerrainMap.Error());
	}
	Result<TileMap> VisualLayer =
		MakeMap(std::move(VisualMap), TileWidth, TileHeight);
	if (VisualLayer.IsFailure()) {
		return Result<LegacyStageData>::Failure(VisualLayer.Error());
	}
	Result<TileMap> ObjectLayer =
		MakeMap(std::move(Object), TileWidth, TileHeight);
	if (ObjectLayer.IsFailure()) {
		return Result<LegacyStageData>::Failure(ObjectLayer.Error());
	}
	Result<TileMap> EventLayer =
		MakeMap(std::move(Event), TileWidth, TileHeight);
	if (EventLayer.IsFailure()) {
		return Result<LegacyStageData>::Failure(EventLayer.Error());
	}

	Result<LayeredMap> Layers = LayeredMap::Create(
		std::move(TerrainMap.Value()),
		std::move(VisualLayer.Value()),
		std::move(ObjectLayer.Value()),
		std::move(EventLayer.Value()));
	if (Layers.IsFailure()) {
		return Result<LegacyStageData>::Failure(Layers.Error());
	}

	Stage.Layers = std::move(Layers.Value());
	return Result<LegacyStageData>::Success(std::move(Stage));
}

} // namespace uchinoko
