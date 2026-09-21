#include "NativeStageDataLoader.h"

#include "GridDataLoader.h"

#include <nlohmann/json.hpp>

#include <climits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace uchinoko {
namespace {

using Json = nlohmann::json;

std::string StripUtf8Bom(std::string Text) {
	if (Text.size() >= 3 &&
		static_cast<unsigned char>(Text[0]) == 0xEF &&
		static_cast<unsigned char>(Text[1]) == 0xBB &&
		static_cast<unsigned char>(Text[2]) == 0xBF) {
		Text.erase(0, 3);
	}
	return Text;
}

std::string DirectoryOf(const std::string& FileName) {
	const std::size_t Position = FileName.find_last_of("/\\");
	if (Position == std::string::npos) return "";
	return FileName.substr(0, Position);
}

bool IsAbsolutePath(const std::string& Path) {
	if (Path.empty()) return false;
	if (Path[0] == '/' || Path[0] == '\\') return true;
	return Path.size() >= 2 && Path[1] == ':';
}

std::string JoinPath(const std::string& Base, const std::string& Relative) {
	if (Base.empty() || IsAbsolutePath(Relative)) return Relative;
	if (Base.back() == '/' || Base.back() == '\\') return Base + Relative;
	return Base + "/" + Relative;
}

const Json& RequireField(
	const Json& Object,
	const char* Name,
	const std::string& Context) {
	if (!Object.is_object()) {
		throw std::runtime_error(Context + " must be an object");
	}
	const auto Iterator = Object.find(Name);
	if (Iterator == Object.end()) {
		throw std::runtime_error(
			Context + " is missing required field '" + Name + "'");
	}
	return *Iterator;
}

std::string RequireString(
	const Json& Object,
	const char* Name,
	const std::string& Context) {
	const Json& Value = RequireField(Object, Name, Context);
	if (!Value.is_string()) {
		throw std::runtime_error(
			Context + "." + Name + " must be a string");
	}
	return Value.get<std::string>();
}

std::string OptionalString(
	const Json& Object,
	const char* Name,
	const std::string& DefaultValue,
	const std::string& Context) {
	const auto Iterator = Object.find(Name);
	if (Iterator == Object.end()) return DefaultValue;
	if (!Iterator->is_string()) {
		throw std::runtime_error(
			Context + "." + Name + " must be a string");
	}
	return Iterator->get<std::string>();
}

int ReadIntegerValue(const Json& Value, const std::string& Context) {
	if (Value.is_number_unsigned()) {
		const unsigned long long Parsed = Value.get<unsigned long long>();
		if (Parsed > static_cast<unsigned long long>(INT_MAX)) {
			throw std::runtime_error(Context + " is outside int range");
		}
		return static_cast<int>(Parsed);
	}
	if (!Value.is_number_integer()) {
		throw std::runtime_error(Context + " must be an integer");
	}
	const long long Parsed = Value.get<long long>();
	if (Parsed < INT_MIN || Parsed > INT_MAX) {
		throw std::runtime_error(Context + " is outside int range");
	}
	return static_cast<int>(Parsed);
}

int RequireInteger(
	const Json& Object,
	const char* Name,
	const std::string& Context) {
	return ReadIntegerValue(
		RequireField(Object, Name, Context),
		Context + "." + Name);
}

int OptionalInteger(
	const Json& Object,
	const char* Name,
	int DefaultValue,
	const std::string& Context) {
	const auto Iterator = Object.find(Name);
	if (Iterator == Object.end()) return DefaultValue;
	return ReadIntegerValue(*Iterator, Context + "." + Name);
}

bool OptionalBoolean(
	const Json& Object,
	const char* Name,
	bool DefaultValue,
	const std::string& Context) {
	const auto Iterator = Object.find(Name);
	if (Iterator == Object.end()) return DefaultValue;
	if (!Iterator->is_boolean()) {
		throw std::runtime_error(
			Context + "." + Name + " must be a boolean");
	}
	return Iterator->get<bool>();
}

float ReadFloatValue(const Json& Value, const std::string& Context) {
	if (!Value.is_number()) {
		throw std::runtime_error(Context + " must be a number");
	}
	return static_cast<float>(Value.get<double>());
}

WorldPosition ReadVector2(const Json& Value, const std::string& Context) {
	if (!Value.is_array() || Value.size() != 2) {
		throw std::runtime_error(
			Context + " must be a two-number array");
	}
	return {
		ReadFloatValue(Value[0], Context + "[0]"),
		ReadFloatValue(Value[1], Context + "[1]")
	};
}

void ReadIntegerPair(
	const Json& Value,
	const std::string& Context,
	int& First,
	int& Second) {
	if (!Value.is_array() || Value.size() != 2) {
		throw std::runtime_error(
			Context + " must be a two-integer array");
	}
	First = ReadIntegerValue(Value[0], Context + "[0]");
	Second = ReadIntegerValue(Value[1], Context + "[1]");
}

StagePropertyValue ReadPropertyValue(
	const Json& Value,
	const std::string& Context) {
	if (Value.is_boolean()) {
		return StagePropertyValue::Boolean(Value.get<bool>());
	}
	if (Value.is_number_unsigned() || Value.is_number_integer()) {
		return StagePropertyValue::Integer(
			ReadIntegerValue(Value, Context));
	}
	if (Value.is_number_float()) {
		return StagePropertyValue::Float(
			static_cast<float>(Value.get<double>()));
	}
	if (Value.is_string()) {
		return StagePropertyValue::String(Value.get<std::string>());
	}
	if (Value.is_array() && Value.size() == 2 &&
		Value[0].is_number() && Value[1].is_number()) {
		return StagePropertyValue::Vector2(
			ReadVector2(Value, Context));
	}

	throw std::runtime_error(
		Context +
		" must be int, float, bool, string, or a two-number Vector2");
}

StagePropertyMap ReadProperties(
	const Json& Object,
	const std::string& Context) {
	StagePropertyMap Properties;
	const auto Iterator = Object.find("properties");
	if (Iterator == Object.end()) return Properties;
	if (!Iterator->is_object()) {
		throw std::runtime_error(
			Context + ".properties must be an object");
	}

	for (auto Property = Iterator->begin();
		Property != Iterator->end(); ++Property) {
		Properties.emplace(
			Property.key(),
			ReadPropertyValue(
				Property.value(),
				Context + ".properties." + Property.key()));
	}
	return Properties;
}

LayerMetadata ReadLayerMetadata(
	const Json& Object,
	const std::string& Context) {
	LayerMetadata Metadata;
	Metadata.Id = RequireString(Object, "id", Context);
	Metadata.Name = OptionalString(
		Object, "name", Metadata.Id, Context);
	Metadata.ZOrder = OptionalInteger(
		Object, "zOrder", 0, Context);
	Metadata.Visible = OptionalBoolean(
		Object, "visible", true, Context);
	return Metadata;
}

GameMode ReadGameMode(const Json& Root) {
	const std::string Mode =
		OptionalString(Root, "mode", "Action", "stage");
	if (Mode == "Action") return GameMode::Action;
	if (Mode == "Sokoban") return GameMode::Sokoban;
	throw std::runtime_error(
		"stage.mode must be 'Action' or 'Sokoban'");
}

TileLayerRole ReadTileLayerRole(
	const Json& Object,
	const std::string& Context) {
	const std::string Role =
		RequireString(Object, "role", Context);
	if (Role == "terrain") return TileLayerRole::Terrain;
	if (Role == "visual") return TileLayerRole::Visual;
	throw std::runtime_error(
		Context + ".role must be 'terrain' or 'visual'");
}

StageDirection ReadDirection(
	const Json& Object,
	const char* Name,
	const std::string& Context) {
	const std::string Value =
		OptionalString(Object, Name, "none", Context);
	if (Value == "none") return StageDirection::None;
	if (Value == "up") return StageDirection::Up;
	if (Value == "down") return StageDirection::Down;
	if (Value == "left") return StageDirection::Left;
	if (Value == "right") return StageDirection::Right;
	throw std::runtime_error(
		Context + "." + Name +
		" must be none/up/down/left/right");
}

StageRegionGeometry ReadGeometry(
	const Json& Object,
	const std::string& Context) {
	const std::string Shape =
		RequireString(Object, "shape", Context);
	const WorldPosition Position =
		ReadVector2(
			RequireField(Object, "position", Context),
			Context + ".position");

	if (Shape == "point") {
		return StageRegionGeometry::Point(Position);
	}
	if (Shape == "rectangle") {
		const WorldPosition Size =
			ReadVector2(
				RequireField(Object, "size", Context),
				Context + ".size");
		return StageRegionGeometry::Rectangle(
			Position, Size.X, Size.Y);
	}

	throw std::runtime_error(
		Context + ".shape must be 'point' or 'rectangle'");
}

TileLayer ReadTileLayer(
	const Json& Object,
	const std::string& BaseDirectory,
	int TileWidth,
	int TileHeight,
	const std::string& Context) {
	TileLayer Layer;
	Layer.Metadata = ReadLayerMetadata(Object, Context);
	Layer.Role = ReadTileLayerRole(Object, Context);

	const std::string Source =
		RequireString(Object, "source", Context);
	const std::string FileName =
		JoinPath(BaseDirectory, Source);

	Result<IntegerGrid> Grid = GridDataLoader::Load(FileName);
	if (Grid.IsFailure()) {
		throw std::runtime_error(Grid.Error());
	}

	Result<TileMap> Map =
		TileMap::Create(
			std::move(Grid.Value()),
			TileWidth,
			TileHeight);
	if (Map.IsFailure()) {
		throw std::runtime_error(
			FileName + ": " + Map.Error());
	}
	Layer.Map = std::move(Map.Value());
	return Layer;
}

ObjectSpawn ReadObjectSpawn(
	const Json& Object,
	const std::string& Context) {
	ObjectSpawn Result;
	Result.Id = RequireString(Object, "id", Context);
	Result.TypeId = RequireString(Object, "type", Context);
	Result.Position = ReadVector2(
		RequireField(Object, "position", Context),
		Context + ".position");
	Result.Properties = ReadProperties(Object, Context);
	return Result;
}

ObjectLayer ReadObjectLayer(
	const Json& Object,
	const std::string& Context) {
	ObjectLayer Layer;
	Layer.Metadata = ReadLayerMetadata(Object, Context);

	const auto Iterator = Object.find("objects");
	if (Iterator == Object.end()) return Layer;
	if (!Iterator->is_array()) {
		throw std::runtime_error(
			Context + ".objects must be an array");
	}

	std::size_t Index = 0;
	for (const Json& Entry : *Iterator) {
		Layer.Objects.push_back(
			ReadObjectSpawn(
				Entry,
				Context + ".objects[" +
				std::to_string(Index) + "]"));
		++Index;
	}
	return Layer;
}

StageRegion ReadRegion(
	const Json& Object,
	const std::string& Context) {
	StageRegion Region;
	Region.Id = RequireString(Object, "id", Context);
	Region.TypeId = RequireString(Object, "type", Context);
	Region.Geometry = ReadGeometry(
		RequireField(Object, "geometry", Context),
		Context + ".geometry");
	Region.Properties = ReadProperties(Object, Context);
	return Region;
}

RegionLayer ReadRegionLayer(
	const Json& Object,
	const std::string& Context) {
	RegionLayer Layer;
	Layer.Metadata = ReadLayerMetadata(Object, Context);

	const auto Iterator = Object.find("regions");
	if (Iterator == Object.end()) return Layer;
	if (!Iterator->is_array()) {
		throw std::runtime_error(
			Context + ".regions must be an array");
	}

	std::size_t Index = 0;
	for (const Json& Entry : *Iterator) {
		Layer.Regions.push_back(
			ReadRegion(
				Entry,
				Context + ".regions[" +
				std::to_string(Index) + "]"));
		++Index;
	}
	return Layer;
}

StageTransition ReadTransition(
	const Json& Object,
	const std::string& Context) {
	StageTransition Transition;
	Transition.Id = RequireString(Object, "id", Context);
	Transition.TypeId = RequireString(Object, "type", Context);
	Transition.Entry = ReadGeometry(
		RequireField(Object, "entry", Context),
		Context + ".entry");
	Transition.TargetStageId = OptionalString(
		Object, "targetStage", "", Context);
	Transition.TargetAreaId = RequireString(
		Object, "targetArea", Context);
	Transition.ExitPosition = ReadVector2(
		RequireField(Object, "exitPosition", Context),
		Context + ".exitPosition");
	Transition.EnterDirection = ReadDirection(
		Object, "enterDirection", Context);
	Transition.ExitDirection = ReadDirection(
		Object, "exitDirection", Context);
	Transition.Properties = ReadProperties(Object, Context);
	return Transition;
}

StageAreaSettings ReadAreaSettings(
	const Json& Object,
	const std::string& Context) {
	StageAreaSettings Settings;
	Settings.TimeLimitSeconds = OptionalInteger(
		Object, "timeLimitSeconds", 0, Context);
	Settings.BgmId = OptionalString(
		Object, "bgm", "", Context);
	Settings.BackgroundId = OptionalString(
		Object, "background", "", Context);
	Settings.Properties = ReadProperties(Object, Context);
	return Settings;
}

StageArea ReadArea(
	const Json& Object,
	const std::string& BaseDirectory,
	const std::string& Context) {
	StageArea Area;
	Area.Id = RequireString(Object, "id", Context);
	ReadIntegerPair(
		RequireField(Object, "size", Context),
		Context + ".size",
		Area.Width,
		Area.Height);

	const auto TileSize = Object.find("tileSize");
	if (TileSize != Object.end()) {
		ReadIntegerPair(
			*TileSize,
			Context + ".tileSize",
			Area.TileWidth,
			Area.TileHeight);
	}

	const auto Settings = Object.find("settings");
	if (Settings != Object.end()) {
		if (!Settings->is_object()) {
			throw std::runtime_error(
				Context + ".settings must be an object");
		}
		Area.Settings = ReadAreaSettings(
			*Settings, Context + ".settings");
	}

	const auto TileLayers = Object.find("tileLayers");
	if (TileLayers != Object.end()) {
		if (!TileLayers->is_array()) {
			throw std::runtime_error(
				Context + ".tileLayers must be an array");
		}
		std::size_t Index = 0;
		for (const Json& Layer : *TileLayers) {
			Area.TileLayers.push_back(
				ReadTileLayer(
					Layer,
					BaseDirectory,
					Area.TileWidth,
					Area.TileHeight,
					Context + ".tileLayers[" +
					std::to_string(Index) + "]"));
			++Index;
		}
	}

	const auto ObjectLayers = Object.find("objectLayers");
	if (ObjectLayers != Object.end()) {
		if (!ObjectLayers->is_array()) {
			throw std::runtime_error(
				Context + ".objectLayers must be an array");
		}
		std::size_t Index = 0;
		for (const Json& Layer : *ObjectLayers) {
			Area.ObjectLayers.push_back(
				ReadObjectLayer(
					Layer,
					Context + ".objectLayers[" +
					std::to_string(Index) + "]"));
			++Index;
		}
	}

	const auto RegionLayers = Object.find("regionLayers");
	if (RegionLayers != Object.end()) {
		if (!RegionLayers->is_array()) {
			throw std::runtime_error(
				Context + ".regionLayers must be an array");
		}
		std::size_t Index = 0;
		for (const Json& Layer : *RegionLayers) {
			Area.RegionLayers.push_back(
				ReadRegionLayer(
					Layer,
					Context + ".regionLayers[" +
					std::to_string(Index) + "]"));
			++Index;
		}
	}

	const auto Transitions = Object.find("transitions");
	if (Transitions != Object.end()) {
		if (!Transitions->is_array()) {
			throw std::runtime_error(
				Context + ".transitions must be an array");
		}
		std::size_t Index = 0;
		for (const Json& Transition : *Transitions) {
			Area.Transitions.push_back(
				ReadTransition(
					Transition,
					Context + ".transitions[" +
					std::to_string(Index) + "]"));
			++Index;
		}
	}

	return Area;
}

StageData ReadStage(
	const Json& Root,
	const std::string& BaseDirectory) {
	if (!Root.is_object()) {
		throw std::runtime_error("stage root must be an object");
	}

	const int FormatVersion =
		RequireInteger(Root, "formatVersion", "stage");
	if (FormatVersion != 1) {
		throw std::runtime_error(
			"Unsupported stage formatVersion: " +
			std::to_string(FormatVersion));
	}

	StageData Data;
	Data.Id = RequireString(Root, "id", "stage");
	Data.Mode = ReadGameMode(Root);
	Data.StartAreaId =
		RequireString(Root, "startArea", "stage");
	Data.Properties = ReadProperties(Root, "stage");

	const Json& Areas = RequireField(Root, "areas", "stage");
	if (!Areas.is_array()) {
		throw std::runtime_error("stage.areas must be an array");
	}

	std::size_t Index = 0;
	for (const Json& Area : Areas) {
		Data.Areas.push_back(
			ReadArea(
				Area,
				BaseDirectory,
				"stage.areas[" +
				std::to_string(Index) + "]"));
		++Index;
	}
	return Data;
}

} // namespace

Result<StageData> NativeStageDataLoader::Load(
	const std::string& FileName) {
	std::ifstream File(FileName, std::ios::binary);
	if (!File) {
		return Result<StageData>::Failure(
			"Could not open stage file: " + FileName);
	}

	std::ostringstream Buffer;
	Buffer << File.rdbuf();

	Result<StageData> Parsed =
		Parse(
			StripUtf8Bom(Buffer.str()),
			DirectoryOf(FileName));
	if (Parsed.IsFailure()) {
		return Result<StageData>::Failure(
			FileName + ": " + Parsed.Error());
	}
	return Parsed;
}

Result<StageData> NativeStageDataLoader::Parse(
	const std::string& JsonText,
	const std::string& BaseDirectory) {
	try {
		const Json Root = Json::parse(StripUtf8Bom(JsonText));
		StageData Data = ReadStage(Root, BaseDirectory);

		Result<bool> Validation = ValidateStageData(Data);
		if (Validation.IsFailure()) {
			return Result<StageData>::Failure(
				"Stage validation failed: " +
				Validation.Error());
		}

		return Result<StageData>::Success(std::move(Data));
	} catch (const std::exception& Error) {
		return Result<StageData>::Failure(Error.what());
	}
}

} // namespace uchinoko
