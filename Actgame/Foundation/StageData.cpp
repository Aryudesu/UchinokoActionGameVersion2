#include "StageData.h"

#include <unordered_set>
#include <utility>

namespace uchinoko {

StagePropertyValue StagePropertyValue::Integer(int Value) {
	StagePropertyValue Result;
	Result.Type_ = StagePropertyType::Integer;
	Result.IntegerValue_ = Value;
	return Result;
}

StagePropertyValue StagePropertyValue::Float(float Value) {
	StagePropertyValue Result;
	Result.Type_ = StagePropertyType::Float;
	Result.FloatValue_ = Value;
	return Result;
}

StagePropertyValue StagePropertyValue::Boolean(bool Value) {
	StagePropertyValue Result;
	Result.Type_ = StagePropertyType::Boolean;
	Result.BooleanValue_ = Value;
	return Result;
}

StagePropertyValue StagePropertyValue::String(std::string Value) {
	StagePropertyValue Result;
	Result.Type_ = StagePropertyType::String;
	Result.StringValue_ = std::move(Value);
	return Result;
}

StagePropertyValue StagePropertyValue::Vector2(WorldPosition Value) {
	StagePropertyValue Result;
	Result.Type_ = StagePropertyType::Vector2;
	Result.Vector2Value_ = Value;
	return Result;
}

bool StagePropertyValue::TryGetInteger(int& Value) const {
	if (Type_ != StagePropertyType::Integer) return false;
	Value = IntegerValue_;
	return true;
}

bool StagePropertyValue::TryGetFloat(float& Value) const {
	if (Type_ == StagePropertyType::Float) {
		Value = FloatValue_;
		return true;
	}
	// JSON serializers may normalize a lexical value such as 2.0 to 2.
	// Integer -> float is a safe widening conversion for semantic numeric
	// properties such as speed and range.
	if (Type_ == StagePropertyType::Integer) {
		Value = static_cast<float>(IntegerValue_);
		return true;
	}
	return false;
}

bool StagePropertyValue::TryGetBoolean(bool& Value) const {
	if (Type_ != StagePropertyType::Boolean) return false;
	Value = BooleanValue_;
	return true;
}

bool StagePropertyValue::TryGetString(std::string& Value) const {
	if (Type_ != StagePropertyType::String) return false;
	Value = StringValue_;
	return true;
}

bool StagePropertyValue::TryGetVector2(WorldPosition& Value) const {
	if (Type_ != StagePropertyType::Vector2) return false;
	Value = Vector2Value_;
	return true;
}

const TileDefinition* TileSetDefinition::FindTerrainTile(int Id) const {
	for (const TileDefinition& Definition : TerrainTiles) {
		if (Definition.Id == Id) return &Definition;
	}
	return nullptr;
}

Result<TileCatalog> TileSetDefinition::BuildTerrainCatalog() const {
	TileCatalog Catalog;
	for (const TileDefinition& Definition : TerrainTiles) {
		Result<bool> Registered = Catalog.Register(Definition);
		if (Registered.IsFailure()) {
			return Result<TileCatalog>::Failure(Registered.Error());
		}
	}
	return Result<TileCatalog>::Success(std::move(Catalog));
}

StageRegionGeometry StageRegionGeometry::Point(WorldPosition Position) {
	StageRegionGeometry Result;
	Result.Shape = StageRegionShape::Point;
	Result.Position = Position;
	Result.Size = {0.0f, 0.0f};
	return Result;
}

StageRegionGeometry StageRegionGeometry::Rectangle(
	WorldPosition Position,
	float Width,
	float Height) {
	StageRegionGeometry Result;
	Result.Shape = StageRegionShape::Rectangle;
	Result.Position = Position;
	Result.Size = {Width, Height};
	return Result;
}

namespace {

bool RegisterUnique(
	std::unordered_set<std::string>& Seen,
	const std::string& Id) {
	return Seen.insert(Id).second;
}

Result<bool> ValidateLayerMetadata(
	const LayerMetadata& Metadata,
	std::unordered_set<std::string>& LayerIds) {
	if (Metadata.Id.empty()) {
		return Result<bool>::Failure("Layer id must not be empty");
	}
	if (Metadata.Name.empty()) {
		return Result<bool>::Failure(
			"Layer name must not be empty: " + Metadata.Id);
	}
	if (!RegisterUnique(LayerIds, Metadata.Id)) {
		return Result<bool>::Failure(
			"Duplicate layer id: " + Metadata.Id);
	}
	return Result<bool>::Success(true);
}

Result<bool> ValidateGeometry(
	const StageRegionGeometry& Geometry,
	const std::string& Context) {
	if (Geometry.Shape == StageRegionShape::Rectangle &&
		(Geometry.Size.X <= 0.0f || Geometry.Size.Y <= 0.0f)) {
		return Result<bool>::Failure(
			Context + " rectangle size must be positive");
	}
	return Result<bool>::Success(true);
}

Result<bool> ValidateEntityIdentity(
	const std::string& Id,
	const std::string& TypeId,
	std::unordered_set<std::string>& EntityIds,
	const std::string& Context) {
	if (Id.empty()) {
		return Result<bool>::Failure(Context + " id must not be empty");
	}
	if (TypeId.empty()) {
		return Result<bool>::Failure(
			Context + " type id must not be empty: " + Id);
	}
	if (!RegisterUnique(EntityIds, Id)) {
		return Result<bool>::Failure(
			"Duplicate entity id in area: " + Id);
	}
	return Result<bool>::Success(true);
}

Result<bool> ValidateAreaBasics(const StageArea& Area) {
	if (Area.Id.empty()) {
		return Result<bool>::Failure("Area id must not be empty");
	}
	if (Area.Width <= 0 || Area.Height <= 0) {
		return Result<bool>::Failure(
			"Area dimensions must be positive: " + Area.Id);
	}
	if (Area.TileWidth <= 0 || Area.TileHeight <= 0) {
		return Result<bool>::Failure(
			"Area tile size must be positive: " + Area.Id);
	}
	if (Area.Settings.TimeLimitSeconds < 0) {
		return Result<bool>::Failure(
			"Area time limit must not be negative: " + Area.Id);
	}
	return Result<bool>::Success(true);
}

Result<bool> ValidateAreaContent(const StageData& Data, const StageArea& Area) {
	std::unordered_set<std::string> LayerIds;
	std::unordered_set<std::string> EntityIds;
	int TerrainLayerCount = 0;

	for (const TileLayer& Layer : Area.TileLayers) {
		Result<bool> MetadataResult =
			ValidateLayerMetadata(Layer.Metadata, LayerIds);
		if (MetadataResult.IsFailure()) return MetadataResult;

		if (Layer.Map.Width() != Area.Width ||
			Layer.Map.Height() != Area.Height ||
			Layer.Map.TileWidth() != Area.TileWidth ||
			Layer.Map.TileHeight() != Area.TileHeight) {
			return Result<bool>::Failure(
				"Tile layer dimensions or tile size do not match area: " +
				Layer.Metadata.Id);
		}

		if (!Layer.TileSetId.empty()) {
			const TileSetDefinition* TileSet = Data.FindTileSet(Layer.TileSetId);
			if (TileSet == nullptr) {
				return Result<bool>::Failure(
					"Tile layer references unknown tile set: " +
					Layer.Metadata.Id + " -> " + Layer.TileSetId);
			}
			if (TileSet->TileWidth != Area.TileWidth ||
				TileSet->TileHeight != Area.TileHeight) {
				return Result<bool>::Failure(
					"Tile set tile size does not match area: " +
					Layer.TileSetId);
			}

			if (Layer.Role == TileLayerRole::Terrain &&
				!TileSet->TerrainTiles.empty()) {
				for (int Row = 0; Row < Layer.Map.Height(); ++Row) {
					for (int Column = 0; Column < Layer.Map.Width(); ++Column) {
						const int* Id = Layer.Map.TryGet({Column, Row});
						if (Id == nullptr || TileSet->FindTerrainTile(*Id) != nullptr) {
							continue;
						}
						return Result<bool>::Failure(
							"Terrain layer contains undefined tile id " +
							std::to_string(*Id) + ": " +
							Layer.Metadata.Id);
					}
				}
			}
		}

		if (Layer.Role == TileLayerRole::Terrain) ++TerrainLayerCount;
	}

	if (TerrainLayerCount != 1) {
		return Result<bool>::Failure(
			"Area must contain exactly one terrain tile layer: " + Area.Id);
	}

	for (const ObjectLayer& Layer : Area.ObjectLayers) {
		Result<bool> MetadataResult =
			ValidateLayerMetadata(Layer.Metadata, LayerIds);
		if (MetadataResult.IsFailure()) return MetadataResult;

		for (const ObjectSpawn& Object : Layer.Objects) {
			Result<bool> EntityResult = ValidateEntityIdentity(
				Object.Id, Object.TypeId, EntityIds, "Object");
			if (EntityResult.IsFailure()) return EntityResult;
		}
	}

	for (const RegionLayer& Layer : Area.RegionLayers) {
		Result<bool> MetadataResult =
			ValidateLayerMetadata(Layer.Metadata, LayerIds);
		if (MetadataResult.IsFailure()) return MetadataResult;

		for (const StageRegion& Region : Layer.Regions) {
			Result<bool> EntityResult = ValidateEntityIdentity(
				Region.Id, Region.TypeId, EntityIds, "Region");
			if (EntityResult.IsFailure()) return EntityResult;

			Result<bool> GeometryResult =
				ValidateGeometry(Region.Geometry, "Region " + Region.Id);
			if (GeometryResult.IsFailure()) return GeometryResult;
		}
	}

	for (const StageTransition& Transition : Area.Transitions) {
		Result<bool> EntityResult = ValidateEntityIdentity(
			Transition.Id,
			Transition.TypeId,
			EntityIds,
			"Transition");
		if (EntityResult.IsFailure()) return EntityResult;

		Result<bool> GeometryResult =
			ValidateGeometry(
				Transition.Entry,
				"Transition " + Transition.Id);
		if (GeometryResult.IsFailure()) return GeometryResult;

		if (Transition.TargetAreaId.empty()) {
			return Result<bool>::Failure(
				"Transition target area must not be empty: " +
				Transition.Id);
		}
	}

	return Result<bool>::Success(true);
}

} // namespace

const TileLayer* StageArea::FindTileLayer(const std::string& LayerId) const {
	for (const TileLayer& Layer : TileLayers) {
		if (Layer.Metadata.Id == LayerId) return &Layer;
	}
	return nullptr;
}

TileLayer* StageArea::FindTileLayer(const std::string& LayerId) {
	return const_cast<TileLayer*>(
		static_cast<const StageArea&>(*this).FindTileLayer(LayerId));
}

const TileLayer* StageArea::TerrainLayer() const {
	for (const TileLayer& Layer : TileLayers) {
		if (Layer.Role == TileLayerRole::Terrain) return &Layer;
	}
	return nullptr;
}

TileLayer* StageArea::TerrainLayer() {
	return const_cast<TileLayer*>(
		static_cast<const StageArea&>(*this).TerrainLayer());
}

const ObjectLayer* StageArea::FindObjectLayer(const std::string& LayerId) const {
	for (const ObjectLayer& Layer : ObjectLayers) {
		if (Layer.Metadata.Id == LayerId) return &Layer;
	}
	return nullptr;
}

ObjectLayer* StageArea::FindObjectLayer(const std::string& LayerId) {
	return const_cast<ObjectLayer*>(
		static_cast<const StageArea&>(*this).FindObjectLayer(LayerId));
}

const RegionLayer* StageArea::FindRegionLayer(const std::string& LayerId) const {
	for (const RegionLayer& Layer : RegionLayers) {
		if (Layer.Metadata.Id == LayerId) return &Layer;
	}
	return nullptr;
}

RegionLayer* StageArea::FindRegionLayer(const std::string& LayerId) {
	return const_cast<RegionLayer*>(
		static_cast<const StageArea&>(*this).FindRegionLayer(LayerId));
}

const TileSetDefinition* StageData::FindTileSet(const std::string& TileSetId) const {
	for (const TileSetDefinition& TileSet : TileSets) {
		if (TileSet.Id == TileSetId) return &TileSet;
	}
	return nullptr;
}

TileSetDefinition* StageData::FindTileSet(const std::string& TileSetId) {
	return const_cast<TileSetDefinition*>(
		static_cast<const StageData&>(*this).FindTileSet(TileSetId));
}

const StageArea* StageData::FindArea(const std::string& AreaId) const {
	for (const StageArea& Area : Areas) {
		if (Area.Id == AreaId) return &Area;
	}
	return nullptr;
}

StageArea* StageData::FindArea(const std::string& AreaId) {
	return const_cast<StageArea*>(
		static_cast<const StageData&>(*this).FindArea(AreaId));
}

Result<bool> ValidateStageData(const StageData& Data) {
	if (Data.Id.empty()) {
		return Result<bool>::Failure("Stage id must not be empty");
	}
	if (Data.Areas.empty()) {
		return Result<bool>::Failure("Stage must contain at least one area");
	}
	if (Data.StartAreaId.empty()) {
		return Result<bool>::Failure("Start area id must not be empty");
	}

	std::unordered_set<std::string> TileSetIds;
	for (const TileSetDefinition& TileSet : Data.TileSets) {
		if (TileSet.Id.empty()) {
			return Result<bool>::Failure("Tile set id must not be empty");
		}
		if (TileSet.ImageFile.empty()) {
			return Result<bool>::Failure(
				"Tile set image file must not be empty: " + TileSet.Id);
		}
		if (TileSet.TileWidth <= 0 || TileSet.TileHeight <= 0 ||
			TileSet.Columns <= 0 || TileSet.Rows <= 0) {
			return Result<bool>::Failure(
				"Tile set dimensions must be positive: " + TileSet.Id);
		}
		if (TileSet.EmptyTileId < 0 ||
			TileSet.EmptyTileId >= TileSet.TileCount()) {
			return Result<bool>::Failure(
				"Tile set empty tile id is outside image grid: " +
				TileSet.Id);
		}
		Result<TileCatalog> TerrainCatalog = TileSet.BuildTerrainCatalog();
		if (TerrainCatalog.IsFailure()) {
			return Result<bool>::Failure(
				"Invalid terrain tile definition in tile set " +
				TileSet.Id + ": " + TerrainCatalog.Error());
		}
		for (const TileDefinition& Definition : TileSet.TerrainTiles) {
			if (Definition.ImageIndex < 0 ||
				Definition.ImageIndex >= TileSet.TileCount()) {
				return Result<bool>::Failure(
					"Terrain tile image index is outside tile set grid: " +
					TileSet.Id + " tile " +
					std::to_string(Definition.Id));
			}
		}
		if (!RegisterUnique(TileSetIds, TileSet.Id)) {
			return Result<bool>::Failure(
				"Duplicate tile set id: " + TileSet.Id);
		}
	}

	std::unordered_set<std::string> AreaIds;
	for (const StageArea& Area : Data.Areas) {
		Result<bool> Basics = ValidateAreaBasics(Area);
		if (Basics.IsFailure()) return Basics;
		if (!RegisterUnique(AreaIds, Area.Id)) {
			return Result<bool>::Failure(
				"Duplicate area id: " + Area.Id);
		}

		Result<bool> Content = ValidateAreaContent(Data, Area);
		if (Content.IsFailure()) return Content;
	}

	if (AreaIds.find(Data.StartAreaId) == AreaIds.end()) {
		return Result<bool>::Failure(
			"Start area does not exist: " + Data.StartAreaId);
	}

	for (const StageArea& Area : Data.Areas) {
		for (const StageTransition& Transition : Area.Transitions) {
			if (!Transition.TargetStageId.empty()) continue;
			if (AreaIds.find(Transition.TargetAreaId) == AreaIds.end()) {
				return Result<bool>::Failure(
					"Transition target area does not exist: " +
					Transition.TargetAreaId);
			}
		}
	}

	return Result<bool>::Success(true);
}

} // namespace uchinoko
