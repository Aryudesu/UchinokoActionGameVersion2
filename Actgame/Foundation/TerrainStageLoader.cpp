#include "TerrainStageLoader.h"

#include "GridDataLoader.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace uchinoko {
namespace {

std::string Trim(const std::string& Value) {
	const std::string Whitespace = " \t\r\n";
	const std::size_t Begin = Value.find_first_not_of(Whitespace);
	if (Begin == std::string::npos) return "";
	const std::size_t End = Value.find_last_not_of(Whitespace);
	return Value.substr(Begin, End - Begin + 1);
}

void RemoveBom(std::string& Line) {
	if (Line.size() >= 3 &&
		static_cast<unsigned char>(Line[0]) == 0xEF &&
		static_cast<unsigned char>(Line[1]) == 0xBB &&
		static_cast<unsigned char>(Line[2]) == 0xBF) {
		Line.erase(0, 3);
	}
}

Result<int> ParseInteger(const std::string& Text, const std::string& Name) {
	const std::string Value = Trim(Text);
	errno = 0;
	char* End = nullptr;
	const long Parsed = std::strtol(Value.c_str(), &End, 10);
	if (Value.empty() || errno == ERANGE || Parsed < INT_MIN || Parsed > INT_MAX ||
		End == Value.c_str() || *End != '\0') {
		return Result<int>::Failure("Invalid integer for " + Name + ": " + Value);
	}
	return Result<int>::Success(static_cast<int>(Parsed));
}

Result<bool> ParseBoolean(const std::string& Text, const std::string& Name) {
	const std::string Value = Trim(Text);
	if (Value == "0" || Value == "false") return Result<bool>::Success(false);
	if (Value == "1" || Value == "true") return Result<bool>::Success(true);
	return Result<bool>::Failure("Invalid boolean for " + Name + ": " + Value);
}

Result<CollisionShape> ParseCollisionShape(const std::string& Text) {
	const std::string Name = Trim(Text);
	const std::map<std::string, CollisionShape> Shapes = {
		{"None", CollisionShape::None},
		{"Solid", CollisionShape::Solid},
		{"OneWay", CollisionShape::OneWay},
		{"HitFromBelowOnly", CollisionShape::HitFromBelowOnly},
		{"SlopeUpRight", CollisionShape::SlopeUpRight},
		{"SlopeUpLeft", CollisionShape::SlopeUpLeft},
		{"Stair2x1UpRightLow", CollisionShape::Stair2x1UpRightLow},
		{"Stair2x1UpRightHigh", CollisionShape::Stair2x1UpRightHigh},
		{"Stair2x1UpLeftHigh", CollisionShape::Stair2x1UpLeftHigh},
		{"Stair2x1UpLeftLow", CollisionShape::Stair2x1UpLeftLow},
		{"Stair1x2UpRightBottom", CollisionShape::Stair1x2UpRightBottom},
		{"Stair1x2UpRightTop", CollisionShape::Stair1x2UpRightTop},
		{"Stair1x2UpLeftTop", CollisionShape::Stair1x2UpLeftTop},
		{"Stair1x2UpLeftBottom", CollisionShape::Stair1x2UpLeftBottom}
	};
	const auto Found = Shapes.find(Name);
	if (Found == Shapes.end()) {
		return Result<CollisionShape>::Failure("Unknown collision shape: " + Name);
	}
	return Result<CollisionShape>::Success(Found->second);
}

Result<MovementRegion> ParseMovementRegion(const std::string& Text) {
	const std::string Name = Trim(Text);
	if (Name.empty() || Name == "-" || Name == "None") {
		return Result<MovementRegion>::Success(MovementRegion::None);
	}
	if (Name == "Ladder") {
		return Result<MovementRegion>::Success(MovementRegion::Ladder);
	}
	if (Name == "Water") {
		return Result<MovementRegion>::Success(MovementRegion::Water);
	}
	if (Name == "GravityUp") {
		return Result<MovementRegion>::Success(MovementRegion::GravityUp);
	}
	if (Name == "GravityDown") {
		return Result<MovementRegion>::Success(MovementRegion::GravityDown);
	}
	return Result<MovementRegion>::Failure("Unknown movement region: " + Name);
}

std::vector<std::string> Split(const std::string& Line, char Delimiter) {
	std::vector<std::string> Values;
	std::istringstream Stream(Line);
	std::string Value;
	while (std::getline(Stream, Value, Delimiter)) Values.push_back(Trim(Value));
	return Values;
}

struct ParsedConditionBinding {
	GameStateField Field = GameStateField::None;
	ComparisonOperator Operator = ComparisonOperator::Equal;
	int Threshold = 0;
	int TrueTileId = -1;
	int FalseTileId = -1;
};

Result<ParsedConditionBinding> ParseConditionBinding(const std::string& Text) {
	ParsedConditionBinding Binding;
	const std::string Value = Trim(Text);
	if (Value.empty() || Value == "-") {
		return Result<ParsedConditionBinding>::Success(Binding);
	}

	const std::vector<std::string> Parts = Split(Value, '|');
	if (Parts.size() != 3) {
		return Result<ParsedConditionBinding>::Failure(
			"Invalid condition binding: " + Value);
	}

	const std::string Expression = Parts[0];
	struct OperatorToken {
		const char* Text;
		ComparisonOperator Operator;
	};
	const OperatorToken Operators[] = {
		{"==", ComparisonOperator::Equal},
		{"!=", ComparisonOperator::NotEqual},
		{"<=", ComparisonOperator::LessEqual},
		{">=", ComparisonOperator::GreaterEqual},
		{"<", ComparisonOperator::LessThan},
		{">", ComparisonOperator::GreaterThan}
	};

	std::size_t OperatorPosition = std::string::npos;
	std::string OperatorText;
	for (const OperatorToken& Token : Operators) {
		OperatorPosition = Expression.find(Token.Text);
		if (OperatorPosition != std::string::npos) {
			Binding.Operator = Token.Operator;
			OperatorText = Token.Text;
			break;
		}
	}
	if (OperatorPosition == std::string::npos) {
		return Result<ParsedConditionBinding>::Failure(
			"Condition comparison operator not found: " + Expression);
	}

	const std::string FieldText = Trim(Expression.substr(0, OperatorPosition));
	const std::string ThresholdText =
		Trim(Expression.substr(OperatorPosition + OperatorText.size()));

	if (FieldText == "Coins") Binding.Field = GameStateField::Coins;
	else if (FieldText == "Health") Binding.Field = GameStateField::Health;
	else if (FieldText == "Lives") Binding.Field = GameStateField::Lives;
	else if (FieldText == "Score") Binding.Field = GameStateField::Score;
	else {
		return Result<ParsedConditionBinding>::Failure(
			"Unknown game state field: " + FieldText);
	}

	Result<int> Threshold =
		ParseInteger(ThresholdText, "condition threshold");
	Result<int> TrueTile =
		ParseInteger(Parts[1], "condition true tile id");
	Result<int> FalseTile =
		ParseInteger(Parts[2], "condition false tile id");
	if (Threshold.IsFailure()) {
		return Result<ParsedConditionBinding>::Failure(Threshold.Error());
	}
	if (TrueTile.IsFailure()) {
		return Result<ParsedConditionBinding>::Failure(TrueTile.Error());
	}
	if (FalseTile.IsFailure()) {
		return Result<ParsedConditionBinding>::Failure(FalseTile.Error());
	}

	Binding.Threshold = Threshold.Value();
	Binding.TrueTileId = TrueTile.Value();
	Binding.FalseTileId = FalseTile.Value();
	return Result<ParsedConditionBinding>::Success(Binding);
}

Result<TileTrigger> ParseTileTrigger(const std::string& Text) {
	const std::string Name = Trim(Text);
	const std::map<std::string, TileTrigger> Triggers = {
		{"Touch", TileTrigger::Touch},
		{"HitFromBelow", TileTrigger::HitFromBelow},
		{"StandOn", TileTrigger::StandOn},
		{"PushFromLeft", TileTrigger::PushFromLeft},
		{"PushFromRight", TileTrigger::PushFromRight}
	};
	const auto Found = Triggers.find(Name);
	if (Found == Triggers.end()) {
		return Result<TileTrigger>::Failure("Unknown tile trigger: " + Name);
	}
	return Result<TileTrigger>::Success(Found->second);
}

Result<TileAction> ParseTileAction(const std::string& Text) {
	const std::string Name = Trim(Text);
	const std::map<std::string, TileAction> Actions = {
		{"None", TileAction::None},
		{"ReplaceTile", TileAction::ReplaceTile},
		{"BreakTile", TileAction::BreakTile},
		{"AddCoin", TileAction::AddCoin},
		{"AddHealth", TileAction::AddHealth},
		{"AddLife", TileAction::AddLife},
		{"AddScore", TileAction::AddScore},
		{"Damage", TileAction::Damage},
		{"InstantDeath", TileAction::InstantDeath},
		{"SpawnItem", TileAction::SpawnItem},
		{"IncrementCount", TileAction::IncrementCount},
		{"ToggleSwitch", TileAction::ToggleSwitch},
		{"Goal", TileAction::Goal}
	};
	const auto Found = Actions.find(Name);
	if (Found == Actions.end()) {
		return Result<TileAction>::Failure("Unknown tile action: " + Name);
	}
	return Result<TileAction>::Success(Found->second);
}

Result<std::vector<TileRule>> ParseTileRules(const std::string& Text) {
	std::vector<TileRule> Rules;
	const std::string Value = Trim(Text);
	if (Value.empty() || Value == "-") {
		return Result<std::vector<TileRule>>::Success(std::move(Rules));
	}

	const std::vector<std::string> RuleTexts = Split(Value, ';');
	for (std::size_t Index = 0; Index < RuleTexts.size(); ++Index) {
		const std::vector<std::string> Parts = Split(RuleTexts[Index], ':');
		if (Parts.size() < 2 || Parts.size() > 5) {
			return Result<std::vector<TileRule>>::Failure(
				"Invalid tile rule: " + RuleTexts[Index]);
		}

		Result<TileTrigger> Trigger = ParseTileTrigger(Parts[0]);
		Result<TileAction> Action = ParseTileAction(Parts[1]);
		if (Trigger.IsFailure()) {
			return Result<std::vector<TileRule>>::Failure(Trigger.Error());
		}
		if (Action.IsFailure()) {
			return Result<std::vector<TileRule>>::Failure(Action.Error());
		}

		TileRule Rule;
		Rule.Trigger = Trigger.Value();
		Rule.Action = Action.Value();
		if (Parts.size() >= 3 && !Parts[2].empty()) {
			Result<int> ParsedValue = ParseInteger(Parts[2], "tile rule value");
			if (ParsedValue.IsFailure()) {
				return Result<std::vector<TileRule>>::Failure(ParsedValue.Error());
			}
			Rule.Value = ParsedValue.Value();
		}
		if (Parts.size() >= 4) {
			if (Parts[3] == "once") Rule.Once = true;
			else if (Parts[3] == "repeat") Rule.Once = false;
			else {
				return Result<std::vector<TileRule>>::Failure(
					"Unknown tile rule lifetime: " + Parts[3]);
			}
		}
		if (Parts.size() >= 5 && !Parts[4].empty() && Parts[4] != "any") {
			const std::string Condition = Parts[4];
			std::string Number;
			if (Condition.compare(0, 6, "count<") == 0 && Condition.compare(0, 7, "count<=") != 0) {
				Rule.CountCondition = TileCountCondition::LessThan;
				Number = Condition.substr(6);
			} else if (Condition.compare(0, 7, "count<=") == 0) {
				Rule.CountCondition = TileCountCondition::LessEqual;
				Number = Condition.substr(7);
			} else if (Condition.compare(0, 7, "count==") == 0) {
				Rule.CountCondition = TileCountCondition::Equal;
				Number = Condition.substr(7);
			} else if (Condition.compare(0, 7, "count>=") == 0) {
				Rule.CountCondition = TileCountCondition::GreaterEqual;
				Number = Condition.substr(7);
			} else if (Condition.compare(0, 6, "count>") == 0) {
				Rule.CountCondition = TileCountCondition::GreaterThan;
				Number = Condition.substr(6);
			} else {
				return Result<std::vector<TileRule>>::Failure(
					"Unknown tile count condition: " + Condition);
			}
			Result<int> ParsedCount = ParseInteger(Number, "tile rule count condition");
			if (ParsedCount.IsFailure()) {
				return Result<std::vector<TileRule>>::Failure(ParsedCount.Error());
			}
			Rule.CountValue = ParsedCount.Value();
		}
		Rules.push_back(Rule);
	}
	return Result<std::vector<TileRule>>::Success(std::move(Rules));
}

std::string DirectoryOf(const std::string& Path) {
	const std::size_t Separator = Path.find_last_of("/\\");
	return Separator == std::string::npos ? "" : Path.substr(0, Separator);
}

bool IsAbsolutePath(const std::string& Path) {
	return (!Path.empty() && (Path.front() == '/' || Path.front() == '\\')) ||
		(Path.size() >= 2 && Path[1] == ':');
}

std::string ResolvePath(const std::string& BaseDirectory, const std::string& Path) {
	if (IsAbsolutePath(Path) || BaseDirectory.empty()) return Path;
	const char Separator = BaseDirectory.find('\\') == std::string::npos ? '/' : '\\';
	return BaseDirectory + Separator + Path;
}

Result<std::map<std::string, std::string>> LoadManifest(const std::string& FileName) {
	std::ifstream File(FileName, std::ios::binary);
	if (!File) return Result<std::map<std::string, std::string>>::Failure(
		"Could not open stage manifest: " + FileName);
	std::map<std::string, std::string> Values;
	std::string Line;
	int LineNumber = 0;
	while (std::getline(File, Line)) {
		++LineNumber;
		if (LineNumber == 1) RemoveBom(Line);
		Line = Trim(Line);
		if (Line.empty() || Line.front() == '#' || Line.front() == ';') continue;
		const std::size_t Equal = Line.find('=');
		if (Equal == std::string::npos) {
			return Result<std::map<std::string, std::string>>::Failure(
				FileName + ": expected key=value at line " + std::to_string(LineNumber));
		}
		const std::string Key = Trim(Line.substr(0, Equal));
		const std::string Value = Trim(Line.substr(Equal + 1));
		if (Key.empty() || Value.empty()) {
			return Result<std::map<std::string, std::string>>::Failure(
				FileName + ": empty key or value at line " + std::to_string(LineNumber));
		}
		if (Values.find(Key) != Values.end()) {
			return Result<std::map<std::string, std::string>>::Failure(
				FileName + ": duplicate key '" + Key + "'");
		}
		Values[Key] = Value;
	}
	return Result<std::map<std::string, std::string>>::Success(std::move(Values));
}

Result<std::string> Require(
	const std::map<std::string, std::string>& Values, const std::string& Key) {
	const auto Found = Values.find(Key);
	if (Found == Values.end()) return Result<std::string>::Failure("Missing setting: " + Key);
	return Result<std::string>::Success(Found->second);
}

} // namespace

Result<TileCatalog> TerrainStageLoader::LoadCatalog(const std::string& FileName) {
	std::ifstream File(FileName, std::ios::binary);
	if (!File) return Result<TileCatalog>::Failure("Could not open tile catalog: " + FileName);
	TileCatalog Catalog;
	std::string Line;
	int LineNumber = 0;
	while (std::getline(File, Line)) {
		++LineNumber;
		if (LineNumber == 1) RemoveBom(Line);
		Line = Trim(Line);
		if (Line.empty() || Line.front() == '#') continue;
		const std::vector<std::string> Cells = Split(Line, ',');
		if (Cells.size() != 5 && Cells.size() != 6 &&
			Cells.size() != 9 && Cells.size() != 10 &&
			Cells.size() != 11 && Cells.size() != 12) {
			return Result<TileCatalog>::Failure(FileName + ": expected 5, 6, 9, 10, 11 or 12 columns at line " +
				std::to_string(LineNumber));
		}
		Result<int> Id = ParseInteger(Cells[0], "tile id");
		Result<CollisionShape> Shape = ParseCollisionShape(Cells[1]);
		Result<int> Image = ParseInteger(Cells[2], "image index");
		Result<bool> Breakable = ParseBoolean(Cells[3], "breakable");
		Result<bool> Damaging = ParseBoolean(Cells[4], "damaging");
		Result<std::vector<TileRule>> Rules =
			Cells.size() >= 6
				? ParseTileRules(Cells[5])
				: Result<std::vector<TileRule>>::Success(std::vector<TileRule>());
		Result<int> SwitchChannel = Cells.size() >= 9
			? ParseInteger(Cells[6], "switch channel")
			: Result<int>::Success(-1);
		Result<int> SwitchOnTile = Cells.size() >= 9
			? ParseInteger(Cells[7], "switch on tile id")
			: Result<int>::Success(-1);
		Result<int> SwitchOffTile = Cells.size() >= 9
			? ParseInteger(Cells[8], "switch off tile id")
			: Result<int>::Success(-1);
		Result<int> AutoTogglePeriod = Cells.size() >= 10
			? ParseInteger(Cells[9], "auto toggle period")
			: Result<int>::Success(0);
		Result<ParsedConditionBinding> Condition =
			Cells.size() >= 11
				? ParseConditionBinding(Cells[10])
				: Result<ParsedConditionBinding>::Success(ParsedConditionBinding());
		Result<MovementRegion> Movement =
			Cells.size() == 12
				? ParseMovementRegion(Cells[11])
				: Result<MovementRegion>::Success(MovementRegion::None);
		if (Id.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Id.Error());
		if (Shape.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Shape.Error());
		if (Image.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Image.Error());
		if (Breakable.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Breakable.Error());
		if (Damaging.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Damaging.Error());
		if (Rules.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Rules.Error());
		if (SwitchChannel.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + SwitchChannel.Error());
		if (SwitchOnTile.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + SwitchOnTile.Error());
		if (SwitchOffTile.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + SwitchOffTile.Error());
		if (AutoTogglePeriod.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + AutoTogglePeriod.Error());
		if (AutoTogglePeriod.Value() < 0) return Result<TileCatalog>::Failure(FileName + ": auto toggle period must be >= 0");
		if (Condition.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Condition.Error());
		if (Movement.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Movement.Error());
		TileDefinition Definition;
		Definition.Id = Id.Value();
		Definition.Collision = Shape.Value();
		Definition.ImageIndex = Image.Value();
		Definition.Breakable = Breakable.Value();
		Definition.Damaging = Damaging.Value();
		Definition.Rules = std::move(Rules.Value());
		Definition.SwitchChannel = SwitchChannel.Value();
		Definition.SwitchOnTileId = SwitchOnTile.Value();
		Definition.SwitchOffTileId = SwitchOffTile.Value();
		Definition.AutoTogglePeriod = AutoTogglePeriod.Value();
		Definition.ConditionField = Condition.Value().Field;
		Definition.ConditionOperator = Condition.Value().Operator;
		Definition.ConditionThreshold = Condition.Value().Threshold;
		Definition.ConditionTrueTileId = Condition.Value().TrueTileId;
		Definition.ConditionFalseTileId = Condition.Value().FalseTileId;
		Definition.Movement = Movement.Value();
		Result<bool> Registered = Catalog.Register(Definition);
		if (Registered.IsFailure()) return Result<TileCatalog>::Failure(FileName + ": " + Registered.Error());
	}
	return Result<TileCatalog>::Success(std::move(Catalog));
}

Result<TerrainStageData> TerrainStageLoader::Load(const std::string& ManifestFile) {
	Result<std::map<std::string, std::string>> Manifest = LoadManifest(ManifestFile);
	if (Manifest.IsFailure()) return Result<TerrainStageData>::Failure(Manifest.Error());
	Result<std::string> TerrainName = Require(Manifest.Value(), "terrain");
	Result<std::string> CatalogName = Require(Manifest.Value(), "tiles");
	Result<std::string> SpawnXText = Require(Manifest.Value(), "spawn_x");
	Result<std::string> SpawnYText = Require(Manifest.Value(), "spawn_y");
	if (TerrainName.IsFailure()) return Result<TerrainStageData>::Failure(TerrainName.Error());
	if (CatalogName.IsFailure()) return Result<TerrainStageData>::Failure(CatalogName.Error());
	if (SpawnXText.IsFailure()) return Result<TerrainStageData>::Failure(SpawnXText.Error());
	if (SpawnYText.IsFailure()) return Result<TerrainStageData>::Failure(SpawnYText.Error());

	int TileWidth = 32;
	int TileHeight = 32;
	const auto WidthSetting = Manifest.Value().find("tile_width");
	const auto HeightSetting = Manifest.Value().find("tile_height");
	Result<int> SpawnX = ParseInteger(SpawnXText.Value(), "spawn_x");
	Result<int> SpawnY = ParseInteger(SpawnYText.Value(), "spawn_y");
	if (SpawnX.IsFailure()) return Result<TerrainStageData>::Failure(SpawnX.Error());
	if (SpawnY.IsFailure()) return Result<TerrainStageData>::Failure(SpawnY.Error());
	if (WidthSetting != Manifest.Value().end()) {
		Result<int> Parsed = ParseInteger(WidthSetting->second, "tile_width");
		if (Parsed.IsFailure()) return Result<TerrainStageData>::Failure(Parsed.Error());
		TileWidth = Parsed.Value();
	}
	if (HeightSetting != Manifest.Value().end()) {
		Result<int> Parsed = ParseInteger(HeightSetting->second, "tile_height");
		if (Parsed.IsFailure()) return Result<TerrainStageData>::Failure(Parsed.Error());
		TileHeight = Parsed.Value();
	}

	const std::string Directory = DirectoryOf(ManifestFile);
	Result<IntegerGrid> Grid = GridDataLoader::Load(ResolvePath(Directory, TerrainName.Value()));
	if (Grid.IsFailure()) return Result<TerrainStageData>::Failure(Grid.Error());
	Result<TileMap> Map = TileMap::Create(std::move(Grid.Value()), TileWidth, TileHeight);
	if (Map.IsFailure()) return Result<TerrainStageData>::Failure(Map.Error());
	Result<TileCatalog> Catalog = LoadCatalog(ResolvePath(Directory, CatalogName.Value()));
	if (Catalog.IsFailure()) return Result<TerrainStageData>::Failure(Catalog.Error());
	for (int Row = 0; Row < Map.Value().Height(); ++Row) {
		for (int Column = 0; Column < Map.Value().Width(); ++Column) {
			const int Id = *Map.Value().TryGet({Column, Row});
			if (Catalog.Value().Find(Id) == nullptr) {
				return Result<TerrainStageData>::Failure("Undefined tile ID " + std::to_string(Id) +
					" at row " + std::to_string(Row + 1) + ", column " + std::to_string(Column + 1));
			}
		}
	}
	TerrainStageData Stage;
	Stage.Map = std::move(Map.Value());
	Stage.Catalog = std::move(Catalog.Value());
	Stage.PlayerSpawn = {static_cast<float>(SpawnX.Value()), static_cast<float>(SpawnY.Value())};
	return Result<TerrainStageData>::Success(std::move(Stage));
}

} // namespace uchinoko
