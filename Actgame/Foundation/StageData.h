#pragma once

#include "Coordinates.h"
#include "Result.h"
#include "StageDefinition.h"
#include "TileDefinition.h"
#include "TileMap.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace uchinoko {

enum class StagePropertyType {
	Integer,
	Float,
	Boolean,
	String,
	Vector2
};

class StagePropertyValue {
public:
	StagePropertyValue() = default;

	static StagePropertyValue Integer(int Value);
	static StagePropertyValue Float(float Value);
	static StagePropertyValue Boolean(bool Value);
	static StagePropertyValue String(std::string Value);
	static StagePropertyValue Vector2(WorldPosition Value);

	StagePropertyType Type() const { return Type_; }

	bool TryGetInteger(int& Value) const;
	bool TryGetFloat(float& Value) const;
	bool TryGetBoolean(bool& Value) const;
	bool TryGetString(std::string& Value) const;
	bool TryGetVector2(WorldPosition& Value) const;

private:
	StagePropertyType Type_ = StagePropertyType::Integer;
	int IntegerValue_ = 0;
	float FloatValue_ = 0.0f;
	bool BooleanValue_ = false;
	std::string StringValue_;
	WorldPosition Vector2Value_;
};

using StagePropertyMap = std::unordered_map<std::string, StagePropertyValue>;

struct TileSetDefinition {
	std::string Id;
	std::string ImageFile;
	int TileWidth = 32;
	int TileHeight = 32;
	int Columns = 0;
	int Rows = 0;
	int EmptyTileId = 0;
	bool Transparent = true;
	std::vector<TileDefinition> TerrainTiles;

	int TileCount() const { return Columns * Rows; }
	const TileDefinition* FindTerrainTile(int Id) const;
	Result<TileCatalog> BuildTerrainCatalog() const;
};

struct LayerMetadata {
	std::string Id;
	std::string Name;
	int ZOrder = 0;
	bool Visible = true;
};

enum class TileLayerRole {
	Terrain,
	Visual
};

struct TileLayer {
	LayerMetadata Metadata;
	TileLayerRole Role = TileLayerRole::Visual;
	std::string TileSetId;
	TileMap Map;
};

struct ObjectSpawn {
	std::string Id;
	std::string TypeId;
	WorldPosition Position;
	StagePropertyMap Properties;
};

struct ObjectLayer {
	LayerMetadata Metadata;
	std::vector<ObjectSpawn> Objects;
};

enum class StageRegionShape {
	Point,
	Rectangle
};

struct StageRegionGeometry {
	StageRegionShape Shape = StageRegionShape::Point;
	WorldPosition Position;
	WorldPosition Size;

	static StageRegionGeometry Point(WorldPosition Position);
	static StageRegionGeometry Rectangle(
		WorldPosition Position,
		float Width,
		float Height);
};

struct StageRegion {
	std::string Id;
	std::string TypeId;
	StageRegionGeometry Geometry;
	StagePropertyMap Properties;
};

struct RegionLayer {
	LayerMetadata Metadata;
	std::vector<StageRegion> Regions;
};

enum class StageDirection {
	None,
	Up,
	Down,
	Left,
	Right
};

struct StageTransition {
	std::string Id;
	std::string TypeId;
	StageRegionGeometry Entry;
	std::string TargetStageId;
	std::string TargetAreaId;
	WorldPosition ExitPosition;
	StageDirection EnterDirection = StageDirection::None;
	StageDirection ExitDirection = StageDirection::None;
	StagePropertyMap Properties;
};

struct StageAreaSettings {
	int TimeLimitSeconds = 0;
	std::string BgmId;
	std::string BackgroundId;
	StagePropertyMap Properties;
};

struct StageArea {
	std::string Id;
	int Width = 0;
	int Height = 0;
	int TileWidth = 32;
	int TileHeight = 32;
	StageAreaSettings Settings;
	std::vector<TileLayer> TileLayers;
	std::vector<ObjectLayer> ObjectLayers;
	std::vector<RegionLayer> RegionLayers;
	std::vector<StageTransition> Transitions;

	const TileLayer* FindTileLayer(const std::string& LayerId) const;
	TileLayer* FindTileLayer(const std::string& LayerId);
	const TileLayer* TerrainLayer() const;
	TileLayer* TerrainLayer();
	const ObjectLayer* FindObjectLayer(const std::string& LayerId) const;
	ObjectLayer* FindObjectLayer(const std::string& LayerId);
	const RegionLayer* FindRegionLayer(const std::string& LayerId) const;
	RegionLayer* FindRegionLayer(const std::string& LayerId);
};

struct StageData {
	std::string Id;
	GameMode Mode = GameMode::Action;
	std::string StartAreaId;
	StagePropertyMap Properties;
	std::vector<TileSetDefinition> TileSets;
	std::vector<StageArea> Areas;

	const TileSetDefinition* FindTileSet(const std::string& TileSetId) const;
	TileSetDefinition* FindTileSet(const std::string& TileSetId);
	const StageArea* FindArea(const std::string& AreaId) const;
	StageArea* FindArea(const std::string& AreaId);
};

Result<bool> ValidateStageData(const StageData& Data);

} // namespace uchinoko
