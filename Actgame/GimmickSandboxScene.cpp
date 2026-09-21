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

} // namespace

GimmickSandboxScene::GimmickSandboxScene() {
	Reload();
}

void GimmickSandboxScene::Reload() {
	uchinoko::Result<uchinoko::TerrainStageData> Loaded =
		uchinoko::TerrainStageLoader::Load("dat/stage/hazard-test/stage.ini");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}

	Map_ = std::move(Loaded.Value().Map);
	Catalog_ = std::move(Loaded.Value().Catalog);
	Pipes_ = std::move(Loaded.Value().Pipes);
	Pipe_.Reset();
	DamageReaction_.Reset();
	Runtime_.Reset(Map_);
	Items_.Reset();
	Bricks_.Reset();
	// Version1 の GameData 初期値と同じく ON から開始する。
	World_.Reset(2, true);
	World_.Synchronize(Map_, Catalog_);

	uchinoko::CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	Player_ = uchinoko::CharacterController(Body);

	Coins_ = 0;
	Score_ = 0;
	Health_ = 5;
	Lives_ = 0;
	Broken_ = 0;
	FacingDirection_ = 1;
	Completion_.Reset();
	Dead_ = false;
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
		case uchinoko::TileEffectType::Damage:
			if (Effects[Index].Actor == uchinoko::TileActor::Player && !Dead_) {
				const uchinoko::CharacterBody& Body = Player_.Body();
				const float PlayerCenterX =
					Body.Position.X + Body.Width * 0.5f;
				const float HazardCenterX =
					(static_cast<float>(Effects[Index].Position.Column) + 0.5f) *
					static_cast<float>(Map_.TileWidth());
				const int KnockbackDirection =
					uchinoko::DamageReactionState::DirectionAwayFromSource(
						PlayerCenterX, HazardCenterX, -FacingDirection_);

				if (DamageReaction_.Begin(KnockbackDirection)) {
					// V1 Damaged(): 被ダメージ開始時に speed.y=0。
					Player_.Reposition(Player_.Body().Position);
					Health_ -= Effects[Index].Value;
					if (Health_ <= 0) {
						Health_ = 0;
						Dead_ = true;
					}
				}
			}
			break;
		case uchinoko::TileEffectType::InstantDeath:
			if (Effects[Index].Actor == uchinoko::TileActor::Player) {
				Dead_ = true;
			}
			break;
		case uchinoko::TileEffectType::Goal: {
			uchinoko::GoalKind Kind;
			if (!Completion_.Cleared &&
				uchinoko::TryGoalKindFromValue(Effects[Index].Value, Kind)) {
				ClearState_.Record(Kind);
				Completion_.Complete(Kind);
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
	Bricks_.ConsumeTileEffects(TileEffects, MakeGameStateSnapshot());
	ApplyEffectList(TileEffects);
	if (Dead_) return;

	// ゴール取得フレームではGoalと同時に発生したScore等だけ反映し、
	// その後の地形・Item更新へ進まずステージ終了状態で止める。
	if (Completion_.Cleared) return;

	const std::vector<uchinoko::TileEffect> BrickEffects =
		Bricks_.Update(
			Map_, Map_.TileWidth(), Map_.TileHeight(),
			static_cast<float>(WINDOWY + 32 * 3));
	ApplyEffectList(BrickEffects);

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
	if (ReturnKey(KEY_INPUT_R) == 1) {
		Reload();
		return;
	}
	if (!LoadError_.empty() || Dead_) return;

	// ゴール取得後はプレイヤー入力を受け付けず、横移動を0にする。
	// CharacterControllerの通常物理だけを継続し、取得時のY速度と重力で
	// 支持面へ着地するまで移動させる。
	if (Completion_.Cleared) {
		Player_.StepWithoutInput(Map_, Catalog_);
		return;
	}

	// V1 DamageMotion相当。被ダメージ中はユーザー入力を無視し、
	// 1～15Fは3px/frameでノックバック、16F目は横0で通常へ戻る。
	if (DamageReaction_.Active()) {
		uchinoko::CharacterInput DamageInput;
		DamageInput.Horizontal = DamageReaction_.AdvanceFrame();
		Player_.Step(DamageInput, Map_, Catalog_);
		ApplyEffects();
		return;
	}

	uchinoko::CharacterInput Input;
	if (ReturnKey(KEY_INPUT_LEFT) != 0) Input.Horizontal -= 1.0f;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) Input.Horizontal += 1.0f;
	if (ReturnKey(KEY_INPUT_UP) != 0) Input.Vertical -= 1.0f;
	if (ReturnKey(KEY_INPUT_DOWN) != 0) Input.Vertical += 1.0f;
	Input.JumpPressed = ReturnKey(KEY_INPUT_Z) == 1;
	if (Input.Horizontal > 0.0f) FacingDirection_ = 1;
	else if (Input.Horizontal < 0.0f) FacingDirection_ = -1;

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
			const bool BrickTile = HasAction(*Definition, uchinoko::TileAction::HitBrick);
			const bool DamageTile = HasAction(*Definition, uchinoko::TileAction::Damage);
			const bool KillTile = HasAction(*Definition, uchinoko::TileAction::InstantDeath);
			const bool HazardTile = DamageTile || KillTile;
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
						: (HazardTile
							? (KillTile ? GetColor(145, 45, 65) : GetColor(205, 95, 70))
							: (BrickTile
								? GetColor(175, 95, 55)
								: (SpawnsItem ? GetColor(210, 160, 70) : GetColor(80, 130, 190)))),
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
			if (BrickTile) {
				DrawLine(Left + 2, Top + 10, Right - 2, Top + 10,
					GetColor(245, 175, 105), 2);
				DrawLine(Left + 2, Top + 21, Right - 2, Top + 21,
					GetColor(245, 175, 105), 2);
				const uchinoko::ActiveBrick* Brick =
					Bricks_.TryGet({Column, Row});
				if (Brick != nullptr) {
					DrawFormatString(
						Left + 6, Top + 7, GetColor(255, 245, 200),
						"%s%d",
						Brick->Phase == uchinoko::BrickPhase::Breaking ? "X" : "B",
						Bricks_.VisualFrameOffset({Column, Row}));
				}
			}
			if (HazardTile) {
				const uchinoko::TileRule* HazardRule = nullptr;
				for (std::size_t RuleIndex = 0;
					RuleIndex < Definition->Rules.size(); ++RuleIndex) {
					const uchinoko::TileAction Action =
						Definition->Rules[RuleIndex].Action;
					if (Action == uchinoko::TileAction::Damage ||
						Action == uchinoko::TileAction::InstantDeath) {
						HazardRule = &Definition->Rules[RuleIndex];
						break;
					}
				}
				if (HazardRule != nullptr) {
					const char* Target = "P";
					if (HazardRule->Target == uchinoko::TileTarget::Enemy) Target = "E";
					else if (HazardRule->Target == uchinoko::TileTarget::Both) Target = "B";
					DrawFormatString(
						Left + 6, Top + 7, GetColor(255, 255, 255),
						"%s%s", KillTile ? "K" : "D", Target);
				}
			}
			if (HasAction(*Definition, uchinoko::TileAction::Goal)) {
				int GoalValue = 0;
				for (std::size_t RuleIndex = 0;
					RuleIndex < Definition->Rules.size(); ++RuleIndex) {
					if (Definition->Rules[RuleIndex].Action ==
						uchinoko::TileAction::Goal) {
						GoalValue = Definition->Rules[RuleIndex].Value;
						break;
					}
				}
				const bool Secret =
					GoalValue == static_cast<int>(uchinoko::GoalKind::Secret);
				DrawCircle(
					Left + Map_.TileWidth() / 2,
					Top + Map_.TileHeight() / 2,
					11,
					Secret ? GetColor(210, 120, 230) : GetColor(100, 220, 150),
					TRUE);
				DrawString(
					Left + 11, Top + 8,
					Secret ? "S" : "N",
					GetColor(255, 255, 255));
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

	for (std::size_t Index = 0; Index < Bricks_.Fragments().size(); ++Index) {
		const uchinoko::BrickFragment& Fragment = Bricks_.Fragments()[Index];
		const int X = static_cast<int>(Fragment.Position.X);
		const int Y = static_cast<int>(Fragment.Position.Y);
		DrawBox(X, Y, X + 7, Y + 7, GetColor(190, 105, 60), TRUE);
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

	DrawString(16, 16,
		"Hazard test: LEFT/RIGHT, Z jump, R reload, Esc",
		GetColor(255, 255, 255));
	DrawString(16, 40,
		"DP/DE/DB = Damage Player/Enemy/Both",
		GetColor(255, 220, 190));
	DrawString(16, 64,
		"KP/KE/KB = Kill Player/Enemy/Both",
		GetColor(255, 190, 200));
	DrawFormatString(16, 88, GetColor(255, 255, 255),
		"HP:%d  Damage:%s  Frame:%d  Facing:%s",
		Health_,
		DamageReaction_.Active() ? "ON" : "-",
		DamageReaction_.Frame(),
		FacingDirection_ > 0 ? "RIGHT" : "LEFT");
	if (Dead_) {
		DrawString(16, 112,
			"DEAD - R: reload",
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
