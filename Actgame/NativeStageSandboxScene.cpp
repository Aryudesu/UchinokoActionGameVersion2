#include "NativeStageSandboxScene.h"

#include "Conf.h"
#include "DxLib.h"
#include "Foundation/NativeStageDataLoader.h"
#include "Foundation/PlayerResourceRules.h"
#include "InputKey.h"
#include "SceneChanger.h"

#include <algorithm>
#include <utility>

namespace {
constexpr int StageOffsetX = 64;
constexpr int StageOffsetY = 120;
constexpr int PlayerZOrder = 15;

int ScreenX(float WorldX) {
	return StageOffsetX + static_cast<int>(WorldX);
}

int ScreenY(float WorldY) {
	return StageOffsetY + static_cast<int>(WorldY);
}

bool TryPipeDirection(
	uchinoko::StageDirection Direction,
	uchinoko::PipeDirection& Result) {
	switch (Direction) {
	case uchinoko::StageDirection::Down:
		Result = uchinoko::PipeDirection::Down;
		return true;
	case uchinoko::StageDirection::Up:
		Result = uchinoko::PipeDirection::Up;
		return true;
	case uchinoko::StageDirection::Right:
		Result = uchinoko::PipeDirection::Right;
		return true;
	case uchinoko::StageDirection::Left:
		Result = uchinoko::PipeDirection::Left;
		return true;
	case uchinoko::StageDirection::None:
		return false;
	}
	return false;
}

bool TryRegionGoalKind(
	const uchinoko::StageRegion& Region,
	uchinoko::GoalKind& Kind) {
	const auto Found = Region.Properties.find("goalKind");
	if (Found == Region.Properties.end()) return false;

	std::string Value;
	if (!Found->second.TryGetString(Value)) return false;
	if (Value == "normal") {
		Kind = uchinoko::GoalKind::Normal;
		return true;
	}
	if (Value == "secret") {
		Kind = uchinoko::GoalKind::Secret;
		return true;
	}
	return false;
}
}

NativeStageSandboxScene::NativeStageSandboxScene() {
	Reload();
}

NativeStageSandboxScene::~NativeStageSandboxScene() {
	DestroyTileSets();
}

void NativeStageSandboxScene::DestroyTileSets() {
	for (auto& Pair : TileSets_) {
		for (int Handle : Pair.second.Handles) {
			if (Handle >= 0) DeleteGraph(Handle);
		}
	}
	TileSets_.clear();
}

bool NativeStageSandboxScene::LoadTileSets() {
	SetTransColor(TRANSR, TRANSG, TRANSB);

	for (const uchinoko::TileSetDefinition& Definition : Stage_.TileSets) {
		LoadedTileSet Loaded;
		Loaded.EmptyTileId = Definition.EmptyTileId;
		Loaded.Transparent = Definition.Transparent;
		Loaded.Handles.assign(
			static_cast<std::size_t>(Definition.TileCount()), -1);

		const int Result = LoadDivGraph(
			Definition.ImageFile.c_str(),
			Definition.TileCount(),
			Definition.Columns,
			Definition.Rows,
			Definition.TileWidth,
			Definition.TileHeight,
			Loaded.Handles.data());
		if (Result == -1) {
			LoadError_ =
				"TileSet image load failed: " +
				Definition.Id + " -> " + Definition.ImageFile;
			DestroyTileSets();
			return false;
		}

		TileSets_.emplace(Definition.Id, std::move(Loaded));
	}
	return true;
}

bool NativeStageSandboxScene::ActivateArea(const std::string& AreaId) {
	Area_ = Stage_.FindArea(AreaId);
	TerrainLayer_ = Area_ == nullptr ? nullptr : Area_->TerrainLayer();
	if (Area_ == nullptr) {
		LoadError_ = "Area not found: " + AreaId;
		return false;
	}
	if (TerrainLayer_ == nullptr) {
		LoadError_ = "Terrain layer not found in area: " + AreaId;
		return false;
	}
	if (TerrainLayer_->TileSetId.empty()) {
		LoadError_ = "Terrain layer has no tileSet: " + AreaId;
		return false;
	}

	const uchinoko::TileSetDefinition* TileSet =
		Stage_.FindTileSet(TerrainLayer_->TileSetId);
	if (TileSet == nullptr) {
		LoadError_ =
			"Terrain TileSet not found: " + TerrainLayer_->TileSetId;
		return false;
	}
	if (TileSet->TerrainTiles.empty()) {
		LoadError_ =
			"Terrain TileSet has no terrainTiles: " + TileSet->Id;
		return false;
	}

	uchinoko::Result<uchinoko::TileCatalog> Catalog =
		TileSet->BuildTerrainCatalog();
	if (Catalog.IsFailure()) {
		LoadError_ =
			"Terrain catalog build failed: " + Catalog.Error();
		return false;
	}
	TerrainCatalog_ = std::move(Catalog.Value());
	TerrainRuntime_.Reset(TerrainLayer_->Map);
	return true;
}

bool NativeStageSandboxScene::InitializeNativePlayer() {
	PlayerReady_ = false;
	if (Area_ == nullptr) {
		LoadError_ = "Start area is not active";
		return false;
	}

	const uchinoko::ObjectSpawn* Spawn = nullptr;
	for (const uchinoko::ObjectLayer& Layer : Area_->ObjectLayers) {
		for (const uchinoko::ObjectSpawn& Object : Layer.Objects) {
			if (Object.TypeId != "PlayerSpawn") continue;
			if (Spawn != nullptr) {
				LoadError_ = "Multiple PlayerSpawn objects in start area";
				return false;
			}
			Spawn = &Object;
		}
	}
	if (Spawn == nullptr) {
		LoadError_ = "PlayerSpawn object not found in start area";
		return false;
	}

	uchinoko::CharacterBody Body;
	Body.Position = Spawn->Position;
	Body.Grounded = true;
	Player_ = uchinoko::CharacterController(Body);
	PlayerReady_ = true;
	return true;
}

void NativeStageSandboxScene::ApplyTerrainEffects() {
	if (TerrainLayer_ == nullptr) return;

	const std::vector<uchinoko::TileEffect> Effects =
		uchinoko::TileBehaviorSystem::ApplyAll(
			Player_.Interactions(),
			TerrainLayer_->Map,
			TerrainCatalog_,
			TerrainRuntime_);

	for (const uchinoko::TileEffect& Effect : Effects) {
		switch (Effect.Type) {
		case uchinoko::TileEffectType::AddCoin:
			uchinoko::PlayerResourceRules::AddCoin(
				Effect.Value, Coins_, Lives_);
			break;
		case uchinoko::TileEffectType::AddHealth:
			uchinoko::PlayerResourceRules::AddHealth(
				Effect.Value, Health_);
			break;
		case uchinoko::TileEffectType::AddLife:
			uchinoko::PlayerResourceRules::AddLife(
				Effect.Value, Lives_);
			break;
		case uchinoko::TileEffectType::AddScore:
			Score_ += Effect.Value;
			break;
		case uchinoko::TileEffectType::Damage:
			if (Effect.Actor == uchinoko::TileActor::Player) {
				Health_ -= Effect.Value;
				if (Health_ <= 0) {
					Health_ = 0;
					Dead_ = true;
				}
			}
			break;
		case uchinoko::TileEffectType::InstantDeath:
			if (Effect.Actor == uchinoko::TileActor::Player) {
				Dead_ = true;
			}
			break;
		default:
			// SpawnItem / switch / brick / goal adapters are connected
			// separately. TileBehaviorSystem itself is already shared.
			break;
		}
	}
}

void NativeStageSandboxScene::CheckGoalRegions() {
	if (Area_ == nullptr || Completion_.Cleared) return;

	// TerrainのTouch判定と同じ中央16x32をGoal判定にも使う。
	// 見た目32x32の端がGoalへ少し触れただけではクリアにしない。
	const uchinoko::CharacterTouchBounds Touch = Player_.TouchBounds();
	const uchinoko::WorldPosition GoalProbePosition = {
		Touch.Left,
		Touch.Top
	};
	const uchinoko::WorldPosition GoalProbeSize = {
		Touch.Right - Touch.Left,
		Touch.Bottom - Touch.Top
	};

	for (const uchinoko::RegionLayer& Layer : Area_->RegionLayers) {
		for (const uchinoko::StageRegion& Region : Layer.Regions) {
			if (Region.TypeId != "Goal") continue;
			if (!Region.Geometry.IntersectsRectangle(
				GoalProbePosition, GoalProbeSize)) {
				continue;
			}

			uchinoko::GoalKind Kind;
			if (!TryRegionGoalKind(Region, Kind)) {
				LoadError_ =
					"Goal region has invalid goalKind: " + Region.Id;
				return;
			}

			ClearState_.Record(Kind);
			Completion_.Complete(Kind);
			return;
		}
	}
}

bool NativeStageSandboxScene::TryBeginTransition(
	const uchinoko::CharacterInput& Input) {
	if (Area_ == nullptr || Pipe_.IsActive()) return false;

	for (const uchinoko::StageTransition& Transition : Area_->Transitions) {
		if (Transition.TypeId != "Pipe") continue;
		if (!Transition.TargetStageId.empty()) continue;
		if (Transition.Entry.Shape != uchinoko::StageRegionShape::Point) continue;

		uchinoko::PipeDirection EnterDirection;
		uchinoko::PipeDirection ExitDirection;
		if (!TryPipeDirection(
				Transition.EnterDirection, EnterDirection) ||
			!TryPipeDirection(
				Transition.ExitDirection, ExitDirection)) {
			continue;
		}

		uchinoko::PipeLink Link;
		Link.EntryPosition = Transition.Entry.Position;
		Link.EnterDirection = EnterDirection;
		Link.ExitPosition = Transition.ExitPosition;
		Link.ExitDirection = ExitDirection;

		const std::vector<uchinoko::PipeLink> Links = {Link};
		if (!Pipe_.TryBegin(Input, Player_, Links)) continue;

		ActiveTransitionId_ = Transition.Id;
		ActiveTransitionTargetAreaId_ = Transition.TargetAreaId;
		return true;
	}
	return false;
}

void NativeStageSandboxScene::UpdateTransition() {
	if (!Pipe_.IsActive()) return;

	const uchinoko::PipeTransportPhase Before = Pipe_.Phase();
	Pipe_.Update(Player_);

	// PipeTransportが完全暗転時にExitPosition内部へ移した直後、
	// 描画対象/当たり判定をTargetAreaへ切り替える。
	if (Before == uchinoko::PipeTransportPhase::FadeOut &&
		Pipe_.Phase() == uchinoko::PipeTransportPhase::FadeIn &&
		Area_ != nullptr &&
		ActiveTransitionTargetAreaId_ != Area_->Id) {
		if (!ActivateArea(ActiveTransitionTargetAreaId_)) {
			Pipe_.Reset();
			ActiveTransitionId_.clear();
			ActiveTransitionTargetAreaId_.clear();
			return;
		}
	}

	if (!Pipe_.IsActive()) {
		ActiveTransitionId_.clear();
		ActiveTransitionTargetAreaId_.clear();
	}
}

void NativeStageSandboxScene::Reload() {
	DestroyTileSets();
	Area_ = nullptr;
	TerrainLayer_ = nullptr;
	PlayerReady_ = false;
	Pipe_.Reset();
	Completion_.Reset();
	ClearState_ = uchinoko::StageClearState();
	ActiveTransitionId_.clear();
	ActiveTransitionTargetAreaId_.clear();
	Coins_ = 0;
	Score_ = 0;
	Health_ = 4;
	Lives_ = 3;
	Dead_ = false;
	LoadError_.clear();

	uchinoko::Result<uchinoko::StageData> Loaded =
		uchinoko::NativeStageDataLoader::Load(
			"dat/stage/native-test/stage.json");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}

	Stage_ = std::move(Loaded.Value());

	if (!LoadTileSets()) return;
	if (!ActivateArea(Stage_.StartAreaId)) return;
	InitializeNativePlayer();
}

void NativeStageSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	if (ReturnKey(KEY_INPUT_R) == 1) {
		Reload();
		return;
	}
	if (ReturnKey(KEY_INPUT_D) == 1) {
		ShowDebug_ = !ShowDebug_;
	}
	if (!LoadError_.empty() || !PlayerReady_ || TerrainLayer_ == nullptr ||
		Dead_) {
		return;
	}

	if (Pipe_.IsActive()) {
		UpdateTransition();
		return;
	}

	if (Completion_.Cleared) {
		Player_.StepWithoutInput(TerrainLayer_->Map, TerrainCatalog_);
		return;
	}

	uchinoko::CharacterInput Input;
	if (ReturnKey(KEY_INPUT_LEFT) != 0) Input.Horizontal -= 1.0f;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) Input.Horizontal += 1.0f;
	if (ReturnKey(KEY_INPUT_UP) != 0) Input.Vertical -= 1.0f;
	if (ReturnKey(KEY_INPUT_DOWN) != 0) Input.Vertical += 1.0f;
	Input.JumpPressed = ReturnKey(KEY_INPUT_Z) == 1;

	if (TryBeginTransition(Input)) return;

	Player_.Step(Input, TerrainLayer_->Map, TerrainCatalog_);
	ApplyTerrainEffects();
	if (Dead_) return;
	CheckGoalRegions();
}

void NativeStageSandboxScene::DrawTileLayer(
	const uchinoko::TileLayer& Layer) {
	const auto Found = TileSets_.find(Layer.TileSetId);
	if (Found == TileSets_.end()) return;
	const LoadedTileSet& Loaded = Found->second;
	const uchinoko::TileSetDefinition* DefinitionSet =
		Stage_.FindTileSet(Layer.TileSetId);
	if (DefinitionSet == nullptr) return;

	for (int Row = 0; Row < Layer.Map.Height(); ++Row) {
		for (int Column = 0; Column < Layer.Map.Width(); ++Column) {
			const int* TileId = Layer.Map.TryGet({Column, Row});
			if (TileId == nullptr) continue;

			int ImageIndex = *TileId;
			if (Layer.Role == uchinoko::TileLayerRole::Terrain) {
				const uchinoko::TileDefinition* Definition =
					DefinitionSet->FindTerrainTile(*TileId);
				if (Definition == nullptr) {
					const int Left =
						StageOffsetX + Column * Layer.Map.TileWidth();
					const int Top =
						StageOffsetY + Row * Layer.Map.TileHeight();
					DrawBox(
						Left, Top,
						Left + Layer.Map.TileWidth(),
						Top + Layer.Map.TileHeight(),
						GetColor(255, 60, 220), FALSE);
					DrawFormatString(
						Left + 3, Top + 8,
						GetColor(255, 255, 255),
						"T%d", *TileId);
					continue;
				}
				ImageIndex = Definition->ImageIndex;
			}

			if (ImageIndex == Loaded.EmptyTileId) continue;

			const int Left =
				StageOffsetX + Column * Layer.Map.TileWidth();
			const int Top =
				StageOffsetY + Row * Layer.Map.TileHeight();

			if (ImageIndex < 0 ||
				static_cast<std::size_t>(ImageIndex) >= Loaded.Handles.size()) {
				DrawBox(
					Left, Top,
					Left + Layer.Map.TileWidth(),
					Top + Layer.Map.TileHeight(),
					GetColor(255, 60, 220), FALSE);
				DrawFormatString(
					Left + 3, Top + 8,
					GetColor(255, 255, 255),
					"%d", ImageIndex);
				continue;
			}

			DrawGraph(
				Left,
				Top,
				Loaded.Handles[static_cast<std::size_t>(ImageIndex)],
				Loaded.Transparent ? TRUE : FALSE);

			if (ShowDebug_ &&
				Layer.Role == uchinoko::TileLayerRole::Terrain) {
				const uchinoko::TileDefinition* Definition =
					DefinitionSet->FindTerrainTile(*TileId);
				if (Definition != nullptr) {
					for (const uchinoko::TileRule& Rule : Definition->Rules) {
						const char* Label = nullptr;
						unsigned int Color = GetColor(255, 255, 255);
						switch (Rule.Action) {
						case uchinoko::TileAction::AddCoin:
							Label = "C";
							Color = GetColor(255, 225, 80);
							break;
						case uchinoko::TileAction::Damage:
							Label = "D";
							Color = GetColor(255, 100, 80);
							break;
						case uchinoko::TileAction::InstantDeath:
							Label = "K";
							Color = GetColor(255, 70, 100);
							break;
						case uchinoko::TileAction::SpawnItem:
							Label = "?";
							Color = GetColor(255, 210, 100);
							break;
						case uchinoko::TileAction::Goal:
							Label = "G";
							Color = GetColor(100, 255, 170);
							break;
						default:
							break;
						}
						if (Label != nullptr) {
							DrawString(Left + 10, Top + 7, Label, Color);
							break;
						}
					}
				}
			}
		}
	}

	if (ShowDebug_) {
		DrawFormatString(
			StageOffsetX + Area_->Width * Area_->TileWidth + 24,
			StageOffsetY + 18 * Layer.Metadata.ZOrder / 10 + 60,
			GetColor(210, 220, 255),
			"z=%d  %s",
			Layer.Metadata.ZOrder,
			Layer.Metadata.Name.c_str());
	}
}

void NativeStageSandboxScene::DrawPlayer() {
	if (!PlayerReady_) return;

	const uchinoko::CharacterBody& Body = Player_.Body();
	const int Left = ScreenX(Body.Position.X);
	const int Top = ScreenY(Body.Position.Y);
	const int Right = ScreenX(Body.Position.X + Body.Width);
	const int Bottom = ScreenY(Body.Position.Y + Body.Height);

	DrawBox(
		Left, Top, Right, Bottom,
		GetColor(245, 215, 70), TRUE);
	DrawBox(
		Left, Top, Right, Bottom,
		GetColor(255, 255, 150), FALSE);

	if (ShowDebug_) {
		const uchinoko::CharacterTouchBounds Touch = Player_.TouchBounds();
		DrawBox(
			ScreenX(Touch.Left),
			ScreenY(Touch.Top),
			ScreenX(Touch.Right),
			ScreenY(Touch.Bottom),
			GetColor(80, 230, 255), FALSE);
		DrawFormatString(
			Left, Top - 20,
			GetColor(255, 255, 255),
			"P(%.0f,%.0f)%s",
			Body.Position.X,
			Body.Position.Y,
			Body.Grounded ? " G" : "");
	}
}

void NativeStageSandboxScene::DrawObjectLayer(
	const uchinoko::ObjectLayer& Layer) {
	for (const uchinoko::ObjectSpawn& Object : Layer.Objects) {
		const int X = ScreenX(Object.Position.X);
		const int Y = ScreenY(Object.Position.Y);

		if (Object.TypeId == "PlayerSpawn") {
			if (ShowDebug_) {
				DrawCircle(X + 16, Y + 16, 5, GetColor(120, 255, 120), FALSE);
				DrawLine(X + 8, Y + 16, X + 24, Y + 16,
					GetColor(120, 255, 120), 1);
				DrawLine(X + 16, Y + 8, X + 16, Y + 24,
					GetColor(120, 255, 120), 1);
				DrawString(X, Y + 34, "PlayerSpawn", GetColor(150, 255, 150));
			}
			continue;
		}

		if (Object.TypeId == "WalkingEnemy") {
			DrawBox(
				X + 2, Y + 2, X + 30, Y + 30,
				GetColor(240, 90, 90), FALSE);
			DrawString(X + 10, Y + 8, "E", GetColor(255, 170, 170));
		} else if (Object.TypeId == "HorizontalLift") {
			DrawBox(
				X - 6, Y + 11, X + 38, Y + 21,
				GetColor(90, 180, 255), FALSE);
			DrawString(X + 12, Y - 6, "L", GetColor(160, 220, 255));
		} else {
			DrawCircle(X + 16, Y + 16, 12, GetColor(120, 230, 160), FALSE);
		}

		if (ShowDebug_) {
			DrawFormatString(
				X, Y + 34, GetColor(255, 255, 255),
				"%s", Object.Id.c_str());
		}
	}
}

void NativeStageSandboxScene::DrawGeometry(
	const uchinoko::StageRegionGeometry& Geometry,
	unsigned int Color,
	const char* Label) {
	const int Left = ScreenX(Geometry.Position.X);
	const int Top = ScreenY(Geometry.Position.Y);

	if (Geometry.Shape == uchinoko::StageRegionShape::Point) {
		DrawCircle(Left, Top, 7, Color, FALSE);
		DrawLine(Left - 10, Top, Left + 10, Top, Color, 2);
		DrawLine(Left, Top - 10, Left, Top + 10, Color, 2);
	} else {
		const int Right = ScreenX(
			Geometry.Position.X + Geometry.Size.X);
		const int Bottom = ScreenY(
			Geometry.Position.Y + Geometry.Size.Y);
		DrawBox(Left, Top, Right, Bottom, Color, FALSE);
	}

	if (ShowDebug_ && Label != nullptr) {
		DrawString(Left + 4, Top + 4, Label, Color);
	}
}

void NativeStageSandboxScene::DrawRegionLayer(
	const uchinoko::RegionLayer& Layer) {
	for (const uchinoko::StageRegion& Region : Layer.Regions) {
		unsigned int Color = GetColor(80, 240, 210);
		if (Region.TypeId == "Goal") {
			uchinoko::GoalKind Kind;
			if (TryRegionGoalKind(Region, Kind) &&
				Kind == uchinoko::GoalKind::Secret) {
				Color = GetColor(220, 120, 255);
			} else {
				Color = GetColor(100, 255, 150);
			}
		}
		DrawGeometry(
			Region.Geometry,
			Color,
			Region.TypeId.c_str());
	}
}

void NativeStageSandboxScene::DrawTransitions() {
	if (Area_ == nullptr) return;
	for (const uchinoko::StageTransition& Transition : Area_->Transitions) {
		const unsigned int Color = GetColor(255, 210, 70);
		DrawGeometry(
			Transition.Entry,
			Color,
			Transition.TypeId.c_str());

		if (ShowDebug_) {
			DrawFormatString(
				ScreenX(Transition.Entry.Position.X) + 4,
				ScreenY(Transition.Entry.Position.Y) + 20,
				Color,
				"-> %s%s%s",
				Transition.TargetStageId.empty()
					? "" : Transition.TargetStageId.c_str(),
				Transition.TargetStageId.empty() ? "" : "/",
				Transition.TargetAreaId.c_str());
		}

		if (Transition.TargetStageId.empty() &&
			Transition.TargetAreaId == Area_->Id) {
			const int EntryX = ScreenX(Transition.Entry.Position.X);
			const int EntryY = ScreenY(Transition.Entry.Position.Y);
			const int ExitX = ScreenX(Transition.ExitPosition.X);
			const int ExitY = ScreenY(Transition.ExitPosition.Y);
			DrawLine(EntryX, EntryY, ExitX, ExitY, Color, 1);
			DrawCircle(ExitX, ExitY, 5, Color, TRUE);
		}
	}
}

void NativeStageSandboxScene::draw() {
	if (!LoadError_.empty()) {
		DrawString(
			16, 16,
			"Native stage load error (R: retry, Esc: menu)",
			GetColor(255, 100, 100));
		DrawString(
			16, 44,
			LoadError_.c_str(),
			GetColor(255, 255, 255));
		return;
	}
	if (Area_ == nullptr) return;

	std::vector<DrawLayerEntry> DrawOrder;
	int Order = 0;
	for (std::size_t Index = 0; Index < Area_->TileLayers.size(); ++Index) {
		if (!Area_->TileLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->TileLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Tile,
			Index});
	}
	for (std::size_t Index = 0; Index < Area_->ObjectLayers.size(); ++Index) {
		if (!Area_->ObjectLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->ObjectLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Object,
			Index});
	}
	if (PlayerReady_) {
		DrawOrder.push_back({
			PlayerZOrder,
			Order++,
			DrawLayerKind::Player,
			0});
	}
	for (std::size_t Index = 0; Index < Area_->RegionLayers.size(); ++Index) {
		if (!Area_->RegionLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->RegionLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Region,
			Index});
	}

	std::stable_sort(
		DrawOrder.begin(),
		DrawOrder.end(),
		[](const DrawLayerEntry& Left, const DrawLayerEntry& Right) {
			if (Left.ZOrder != Right.ZOrder) {
				return Left.ZOrder < Right.ZOrder;
			}
			return Left.Order < Right.Order;
		});

	for (const DrawLayerEntry& Entry : DrawOrder) {
		switch (Entry.Kind) {
		case DrawLayerKind::Tile:
			DrawTileLayer(Area_->TileLayers[Entry.Index]);
			break;
		case DrawLayerKind::Object:
			DrawObjectLayer(Area_->ObjectLayers[Entry.Index]);
			break;
		case DrawLayerKind::Player:
			DrawPlayer();
			break;
		case DrawLayerKind::Region:
			DrawRegionLayer(Area_->RegionLayers[Entry.Index]);
			break;
		}
	}
	DrawTransitions();

	DrawString(
		16, 16,
		"Native Stage: LEFT/RIGHT, Z jump / R reload / D debug / Esc",
		GetColor(255, 255, 255));
	DrawFormatString(
		16, 42,
		GetColor(210, 230, 255),
		"Stage:%s  Area:%s  %dx%d tiles",
		Stage_.Id.c_str(),
		Area_->Id.c_str(),
		Area_->Width,
		Area_->Height);
	DrawString(
		16, 68,
		"Yellow=Player  Red=Enemy  Blue=Lift  Green=Goal  Yellow=Transition",
		GetColor(230, 235, 255));
	DrawFormatString(
		16, 92,
		Dead_ ? GetColor(255, 100, 100) : GetColor(255, 245, 180),
		"Coins:%d  HP:%d  Lives:%d  Score:%d%s",
		Coins_, Health_, Lives_, Score_,
		Dead_ ? "  DEAD (R: reload)" : "");

	if (Completion_.Cleared) {
		DrawFormatString(
			16, 116,
			GetColor(120, 255, 160),
			"GOAL CLEAR: %s  (R: reload)",
			Completion_.Goal == uchinoko::GoalKind::Secret
				? "Secret" : "Normal");
	} else if (Pipe_.IsActive()) {
		DrawFormatString(
			16, 116,
			GetColor(255, 220, 100),
			"Transition:%s -> %s",
			ActiveTransitionId_.c_str(),
			ActiveTransitionTargetAreaId_.c_str());
	} else {
		DrawString(
			16, 116,
			"Pipe: stand on marker + direction key / Goal: enter green region",
			GetColor(210, 220, 255));
	}

	const int FadeAlpha = Pipe_.FadeAlpha();
	if (FadeAlpha > 0) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, FadeAlpha);
		DrawBox(0, 0, WINDOWX, WINDOWY, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}
