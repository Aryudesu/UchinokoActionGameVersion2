#include "GimmickSandboxScene.h"

#include "DxLib.h"
#include "Conf.h"
#include "InputKey.h"
#include "SceneChanger.h"
#include "Foundation/TerrainStageLoader.h"

#include <utility>
#include <vector>

namespace {

constexpr int LadderTileId = 55;

bool HasAction(const uchinoko::TileDefinition& Definition, uchinoko::TileAction Action) {
	for (std::size_t Index = 0; Index < Definition.Rules.size(); ++Index) {
		if (Definition.Rules[Index].Action == Action) return true;
	}
	return false;
}

bool IsPipeTile(int Id) {
	return Id >= 61 && Id <= 68;
}

const char* PipePhaseName(uchinoko::PipeTransportPhase Phase) {
	switch (Phase) {
	case uchinoko::PipeTransportPhase::Idle: return "IDLE";
	case uchinoko::PipeTransportPhase::Entering: return "IN";
	case uchinoko::PipeTransportPhase::FadeOut: return "FADE OUT";
	case uchinoko::PipeTransportPhase::FadeIn: return "FADE IN";
	case uchinoko::PipeTransportPhase::Emerging: return "OUT";
	}
	return "?";
}

} // namespace

GimmickSandboxScene::GimmickSandboxScene() {
	Reload();
}

void GimmickSandboxScene::Reload() {
	uchinoko::Result<uchinoko::TerrainStageData> Loaded =
		uchinoko::TerrainStageLoader::Load("dat/stage/goal-test/stage.ini");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}

	Map_ = std::move(Loaded.Value().Map);
	Catalog_ = std::move(Loaded.Value().Catalog);
	Pipes_ = std::move(Loaded.Value().Pipes);
	Pipe_.Reset();
	Runtime_.Reset(Map_);
	Items_.Reset();
	// Version1 の GameData 初期値と同じく ON から開始する。
	World_.Reset(2, true);
	World_.Synchronize(Map_, Catalog_);

	uchinoko::CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	Player_ = uchinoko::CharacterController(Body);

	Coins_ = 0;
	Score_ = 0;
	Health_ = 0;
	Lives_ = 0;
	Broken_ = 0;
	Dead_ = false;
	Progress_.Reset();
	HasLastGoal_ = false;
	LastGoal_ = uchinoko::GoalKind::Normal;
	SynchronizeConditionalTerrain();
	LoadError_.clear();
}

void GimmickSandboxScene::ApplyEffectList(
	const std::vector<uchinoko::TileEffect>& Effects) {
	for (std::size_t Index = 0; Index < Effects.size(); ++Index) {
		switch (Effects[Index].Type) {
		case uchinoko::TileEffectType::AddCoin:
			Coins_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::AddHealth:
			Health_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::AddLife:
			Lives_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::AddScore:
			Score_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::TileBroken:
			++Broken_;
			break;
		case uchinoko::TileEffectType::InstantDeath:
			Dead_ = true;
			break;
		case uchinoko::TileEffectType::Goal: {
			uchinoko::GoalKind Kind;
			if (uchinoko::TryParseGoalKind(Effects[Index].Value, Kind)) {
				Progress_.MarkCleared(SandboxStageId, Kind);
				LastGoal_ = Kind;
				HasLastGoal_ = true;
			}
			break;
		}
		default:
			break;
		}
	}
}

uchinoko::GameStateSnapshot GimmickSandboxScene::MakeGameStateSnapshot() const {
	uchinoko::GameStateSnapshot State;
	State.Coins = Coins_;
	State.Health = Health_;
	State.Lives = Lives_;
	State.Score = Score_;
	return State;
}

void GimmickSandboxScene::SynchronizeConditionalTerrain() {
	const uchinoko::ConditionalTerrainUpdate Update =
		uchinoko::ConditionalTerrain::Synchronize(
			Map_, Catalog_, MakeGameStateSnapshot());
	const uchinoko::CharacterSafetyResult Safety =
		uchinoko::CharacterSafety::ResolveActivatedSolids(
			Player_, Map_, Catalog_, Update.ActivatedSolidTiles);
	ApplyEffectList(Safety.Effects);
}

void GimmickSandboxScene::ApplyEffects() {
	const std::vector<uchinoko::TileEffect> TileEffects =
		uchinoko::TileBehaviorSystem::ApplyAll(
			Player_.Interactions(), Map_, Catalog_, Runtime_);
	Items_.ConsumeTileEffects(TileEffects, Map_.TileWidth(), Map_.TileHeight());
	ApplyEffectList(TileEffects);

	// 共有状態を切り替えて地形を同期した直後だけ、安全判定を行う。
	const uchinoko::WorldStateUpdate WorldUpdate =
		World_.ApplyEffects(TileEffects, Map_, Catalog_);
	const uchinoko::CharacterSafetyResult Safety =
		uchinoko::CharacterSafety::ResolveActivatedSolids(
			Player_, Map_, Catalog_, WorldUpdate.ActivatedSolidTiles);
	ApplyEffectList(Safety.Effects);
	if (Dead_) return;

	// V1 の HiddenTime と同様、毎フレーム進めて周期到達時だけ反転する。
	const uchinoko::WorldStateUpdate TimedUpdate =
		World_.AdvanceFrame(Map_, Catalog_);
	const uchinoko::CharacterSafetyResult TimedSafety =
		uchinoko::CharacterSafety::ResolveActivatedSolids(
			Player_, Map_, Catalog_, TimedUpdate.ActivatedSolidTiles);
	ApplyEffectList(TimedSafety.Effects);
	if (Dead_) return;

	// V1のLadderMaker相当。報酬Itemとは別に地形生成Itemを先に進める。
	Items_.UpdateTerrainItems(Map_, Catalog_, LadderTileId);

	const std::vector<uchinoko::TileEffect> ItemEffects = Items_.Update();
	ApplyEffectList(ItemEffects);
	if (Dead_) return;

	// V1の条件ブロックと同様、最新のプレイヤー状態を毎フレーム反映する。
	SynchronizeConditionalTerrain();
}

void GimmickSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	if (ReturnKey(KEY_INPUT_R) == 1) Reload();
	if (!LoadError_.empty() || Dead_) return;

	// 条件ブロックの境界値確認用デバッグキー。
	if (ReturnKey(KEY_INPUT_1) == 1) Coins_ = 0;
	if (ReturnKey(KEY_INPUT_2) == 1) Coins_ = 49;
	if (ReturnKey(KEY_INPUT_3) == 1) Coins_ = 50;

	uchinoko::CharacterInput Input;
	if (ReturnKey(KEY_INPUT_LEFT) != 0) Input.Horizontal -= 1.0f;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) Input.Horizontal += 1.0f;
	if (ReturnKey(KEY_INPUT_UP) != 0) Input.Vertical -= 1.0f;
	if (ReturnKey(KEY_INPUT_DOWN) != 0) Input.Vertical += 1.0f;
	Input.JumpPressed = ReturnKey(KEY_INPUT_Z) == 1;

	// V1のMovingUpdate相当。土管移動中は通常物理・通常ギミック更新を止める。
	if (Pipe_.IsActive()) {
		Pipe_.Update(Player_);
		return;
	}
	if (Pipe_.TryBegin(Input, Player_, Pipes_)) {
		return;
	}

	Player_.Step(Input, Map_, Catalog_);
	ApplyEffects();
}

void GimmickSandboxScene::draw() {
	if (!LoadError_.empty()) {
		DrawString(16, 16, "Stage load error (R: retry, Esc: menu)", GetColor(255, 100, 100));
		DrawString(16, 44, LoadError_.c_str(), GetColor(255, 255, 255));
		return;
	}

	for (int Row = 0; Row < Map_.Height(); ++Row) {
		for (int Column = 0; Column < Map_.Width(); ++Column) {
			const int* Id = Map_.TryGet({Column, Row});
			const uchinoko::TileDefinition* Definition =
				Id == nullptr ? nullptr : Catalog_.Find(*Id);
			if (Definition == nullptr) continue;

			const int Left = Column * Map_.TileWidth();
			const int Top = Row * Map_.TileHeight();
			const int Right = Left + Map_.TileWidth();
			const int Bottom = Top + Map_.TileHeight();

			const bool SpawnsItem = HasAction(*Definition, uchinoko::TileAction::SpawnItem);
			const bool Hidden =
				Definition->Collision == uchinoko::CollisionShape::HitFromBelowOnly;

			const bool SwitchBound = Definition->SwitchChannel >= 0;
			const bool ConditionBound =
				Definition->ConditionField != uchinoko::GameStateField::None;
			const bool SwitchTile =
				HasAction(*Definition, uchinoko::TileAction::ToggleSwitch);

			if (ConditionBound) {
				DrawBox(
					Left + 1, Top + 1, Right - 1, Bottom - 1,
					Definition->Collision == uchinoko::CollisionShape::Solid
						? GetColor(150, 90, 190)
						: GetColor(150, 90, 190),
					Definition->Collision == uchinoko::CollisionShape::Solid ? TRUE : FALSE);
			} else if (SwitchBound) {
				DrawBox(
					Left + 1, Top + 1, Right - 1, Bottom - 1,
					Definition->Collision == uchinoko::CollisionShape::Solid
						? GetColor(210, 90, 90)
						: GetColor(80, 110, 180),
					Definition->Collision == uchinoko::CollisionShape::Solid ? TRUE : FALSE);
			} else if (Definition->Collision == uchinoko::CollisionShape::Solid) {
				DrawBox(
					Left, Top, Right, Bottom,
					IsPipeTile(*Id)
						? GetColor(70, 170, 90)
						: (SpawnsItem ? GetColor(210, 160, 70) : GetColor(80, 130, 190)),
					TRUE);
				if (IsPipeTile(*Id)) {
					DrawBox(Left + 3, Top + 3, Right - 3, Bottom - 3,
						GetColor(160, 230, 170), FALSE);
				}
			} else if (Definition->Collision == uchinoko::CollisionShape::OneWay) {
				DrawBox(Left, Top, Right, Top + 6, GetColor(90, 180, 230), TRUE);
			} else if (Definition->Collision == uchinoko::CollisionShape::DropThroughOneWay) {
				DrawBox(Left, Top, Right, Top + 6, GetColor(230, 170, 80), TRUE);
				DrawString(Left + 10, Top + 8, "v", GetColor(255, 230, 180));
			}
			if (Definition->Movement == uchinoko::MovementRegion::Water) {
				DrawBox(
					Left, Top, Right, Bottom,
					GetColor(70, 130, 220), TRUE);
				DrawLine(
					Left, Top + 3, Right, Top + 3,
					GetColor(170, 220, 255), 2);
			}
			if (Definition->Movement == uchinoko::MovementRegion::GravityUp) {
				DrawBox(Left + 2, Top + 2, Right - 2, Bottom - 2,
					GetColor(130, 90, 210), FALSE);
				DrawString(Left + 6, Top + 7, "G^", GetColor(220, 200, 255));
			}
			if (Definition->Movement == uchinoko::MovementRegion::GravityDown) {
				DrawBox(Left + 2, Top + 2, Right - 2, Bottom - 2,
					GetColor(210, 120, 80), FALSE);
				DrawString(Left + 6, Top + 7, "Gv", GetColor(255, 220, 190));
			}
			if (Definition->Movement == uchinoko::MovementRegion::Ladder) {
				const int Center = Left + Map_.TileWidth() / 2;
				DrawLine(Center - 7, Top + 2, Center - 7, Bottom - 2,
					GetColor(190, 150, 80), 2);
				DrawLine(Center + 7, Top + 2, Center + 7, Bottom - 2,
					GetColor(190, 150, 80), 2);
				for (int Y = Top + 6; Y < Bottom; Y += 8) {
					DrawLine(Center - 7, Y, Center + 7, Y,
						GetColor(220, 190, 120), 2);
				}
			}
			if (SwitchTile) {
				DrawBox(Left, Top, Right, Bottom, GetColor(80, 190, 110), TRUE);
				DrawString(Left + 10, Top + 7, "S", GetColor(255, 255, 255));
			}
			if (Definition->AutoTogglePeriod > 0) {
				DrawString(Left + 10, Top + 7, "T", GetColor(255, 255, 255));
			}
			if (ConditionBound) {
				const char* Label = "C";
				if (Definition->ConditionThreshold == 0) Label = "C0";
				else if (Definition->ConditionOperator ==
					uchinoko::ComparisonOperator::GreaterEqual) Label = "C+";
				else if (Definition->ConditionOperator ==
					uchinoko::ComparisonOperator::LessThan) Label = "C-";
				DrawString(Left + 5, Top + 7, Label, GetColor(255, 255, 255));
			}
			if (SpawnsItem && !Hidden) {
				DrawString(
					Left + 10, Top + 7, "?", GetColor(255, 255, 255));
			}
			if (HasAction(*Definition, uchinoko::TileAction::AddCoin)) {
				DrawCircle(
					Left + Map_.TileWidth() / 2,
					Top + Map_.TileHeight() / 2,
					9, GetColor(240, 210, 70), TRUE);
			}
			if (HasAction(*Definition, uchinoko::TileAction::BreakTile)) {
				DrawBox(Left + 2, Top + 2, Right - 2, Bottom - 2,
					GetColor(190, 110, 70), FALSE);
			}
			for (std::size_t RuleIndex = 0; RuleIndex < Definition->Rules.size(); ++RuleIndex) {
				const uchinoko::TileRule& Rule = Definition->Rules[RuleIndex];
				if (Rule.Action != uchinoko::TileAction::Goal) continue;
				const bool Secret = Rule.Value == uchinoko::GoalKindValue(
					uchinoko::GoalKind::Secret);
				DrawCircle(
					Left + Map_.TileWidth() / 2,
					Top + Map_.TileHeight() / 2,
					12,
					Secret ? GetColor(190, 110, 240) : GetColor(110, 220, 140),
					TRUE);
				DrawString(
					Left + 11, Top + 8,
					Secret ? "S" : "N",
					GetColor(255, 255, 255));
				break;
			}
			const uchinoko::TileRuntimeState* State = Runtime_.TryGet({Column, Row});
			if (State != nullptr && State->Count > 0) {
				DrawFormatString(
					Left + 3, Top + 18, GetColor(255, 255, 255),
					"%d", State->Count);
			}
		}
	}

	for (std::size_t Index = 0; Index < Pipes_.size(); ++Index) {
		const uchinoko::PipeLink& Link = Pipes_[Index];
		DrawFormatString(
			static_cast<int>(Link.EntryPosition.X),
			static_cast<int>(Link.EntryPosition.Y) - 18,
			GetColor(180, 255, 190),
			"Pipe %d v", static_cast<int>(Index + 1));
	}

	for (std::size_t Index = 0; Index < Items_.Items().size(); ++Index) {
		const uchinoko::SpawnedItem& Item = Items_.Items()[Index];
		const int X = static_cast<int>(Item.Position.X) + 16;
		const int Y = static_cast<int>(Item.Position.Y) + 16;
		switch (Item.Kind) {
		case uchinoko::ItemKind::Coin:
			DrawCircle(X, Y, 8, GetColor(240, 210, 70), TRUE);
			break;
		case uchinoko::ItemKind::Healing:
			DrawCircle(X, Y, 8, GetColor(100, 220, 130), TRUE);
			DrawLine(X - 4, Y, X + 4, Y, GetColor(255, 255, 255), 2);
			DrawLine(X, Y - 4, X, Y + 4, GetColor(255, 255, 255), 2);
			break;
		case uchinoko::ItemKind::OneUp:
			DrawCircle(X, Y, 9, GetColor(120, 210, 255), TRUE);
			DrawString(X - 7, Y - 7, "1", GetColor(20, 40, 70));
			break;
		case uchinoko::ItemKind::LadderBuilder:
			DrawLine(X - 6, Y - 10, X - 6, Y + 10,
				GetColor(220, 190, 120), 2);
			DrawLine(X + 6, Y - 10, X + 6, Y + 10,
				GetColor(220, 190, 120), 2);
			DrawLine(X - 6, Y, X + 6, Y,
				GetColor(220, 190, 120), 2);
			break;
		}
	}

	const uchinoko::CharacterBody& Body = Player_.Body();
	DrawBox(
		static_cast<int>(Body.Position.X), static_cast<int>(Body.Position.Y),
		static_cast<int>(Body.Position.X + Body.Width),
		static_cast<int>(Body.Position.Y + Body.Height),
		GetColor(240, 210, 80), TRUE);

	// V1は土管移動中だけ主人公をMapより先に描画していた。
	// Sandboxでは土管タイルを再描画し、潜り込み/出現部分を隠す。
	if (Pipe_.IsActive()) {
		for (int Row = 0; Row < Map_.Height(); ++Row) {
			for (int Column = 0; Column < Map_.Width(); ++Column) {
				const int* Id = Map_.TryGet({Column, Row});
				if (Id == nullptr || !IsPipeTile(*Id)) continue;
				const int Left = Column * Map_.TileWidth();
				const int Top = Row * Map_.TileHeight();
				const int Right = Left + Map_.TileWidth();
				const int Bottom = Top + Map_.TileHeight();
				DrawBox(Left, Top, Right, Bottom, GetColor(70, 170, 90), TRUE);
				DrawBox(Left + 3, Top + 3, Right - 3, Bottom - 3,
					GetColor(160, 230, 170), FALSE);
			}
		}
	}

	const uchinoko::StageClearState ClearState =
		Progress_.GetOrDefault(SandboxStageId);
	DrawString(16, 16,
		"Goal test: LEFT/RIGHT move, Z jump, R reset, Esc",
		GetColor(255, 255, 255));
	DrawFormatString(16, 40, GetColor(255, 255, 255),
		"Normal:%s  Secret:%s  Either:%s  Both:%s",
		ClearState.NormalCleared ? "YES" : "NO",
		ClearState.SecretCleared ? "YES" : "NO",
		Progress_.Satisfies(SandboxStageId, uchinoko::ClearRequirement::Either) ? "YES" : "NO",
		Progress_.Satisfies(SandboxStageId, uchinoko::ClearRequirement::Both) ? "YES" : "NO");
	if (HasLastGoal_) {
		DrawFormatString(16, 64, GetColor(220, 230, 255),
			LastGoal_ == uchinoko::GoalKind::Secret
				? "Last goal: SECRET (+1000)  Score:%d"
				: "Last goal: NORMAL (+1000)  Score:%d",
			Score_);
	} else {
		DrawFormatString(16, 64, GetColor(220, 230, 255),
			"Green N = Normal Goal / Purple S = Secret Goal  Score:%d",
			Score_);
	}
	if (Dead_) {
		DrawString(16, 88,
			"CRUSHED - InstantDeath (R: reload)",
			GetColor(255, 100, 100));
	}

	// V1のSetBrightによる暗転と同じタイミングを、
	// Sandboxでは黒いオーバーレイで再現する。
	const int FadeAlpha = Pipe_.FadeAlpha();
	if (FadeAlpha > 0) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, FadeAlpha);
		DrawBox(0, 0, WINDOWX, WINDOWY, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}
