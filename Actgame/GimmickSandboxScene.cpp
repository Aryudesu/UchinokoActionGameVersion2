#include "GimmickSandboxScene.h"

#include "DxLib.h"
#include "InputKey.h"
#include "SceneChanger.h"
#include "Foundation/TerrainStageLoader.h"

#include <utility>
#include <vector>

namespace {

bool HasAction(const uchinoko::TileDefinition& Definition, uchinoko::TileAction Action) {
	for (std::size_t Index = 0; Index < Definition.Rules.size(); ++Index) {
		if (Definition.Rules[Index].Action == Action) return true;
	}
	return false;
}

} // namespace

GimmickSandboxScene::GimmickSandboxScene() {
	Reload();
}

void GimmickSandboxScene::Reload() {
	uchinoko::Result<uchinoko::TerrainStageData> Loaded =
		uchinoko::TerrainStageLoader::Load("dat/stage/interaction-test/stage.ini");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}

	Map_ = std::move(Loaded.Value().Map);
	Catalog_ = std::move(Loaded.Value().Catalog);
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
		default:
			break;
		}
	}
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

	const std::vector<uchinoko::TileEffect> ItemEffects = Items_.Update();
	ApplyEffectList(ItemEffects);
}

void GimmickSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	if (ReturnKey(KEY_INPUT_R) == 1) Reload();
	if (!LoadError_.empty() || Dead_) return;

	float Horizontal = 0.0f;
	if (ReturnKey(KEY_INPUT_LEFT) != 0) Horizontal -= 1.0f;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) Horizontal += 1.0f;

	Player_.Step(Horizontal, ReturnKey(KEY_INPUT_Z) == 1, Map_, Catalog_);
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
			const bool SwitchTile =
				HasAction(*Definition, uchinoko::TileAction::ToggleSwitch);

			if (SwitchBound) {
				DrawBox(
					Left + 1, Top + 1, Right - 1, Bottom - 1,
					Definition->Collision == uchinoko::CollisionShape::Solid
						? GetColor(210, 90, 90)
						: GetColor(80, 110, 180),
					Definition->Collision == uchinoko::CollisionShape::Solid ? TRUE : FALSE);
			} else if (Definition->Collision == uchinoko::CollisionShape::Solid) {
				DrawBox(
					Left, Top, Right, Bottom,
					SpawnsItem ? GetColor(210, 160, 70) : GetColor(80, 130, 190), TRUE);
			}
			if (SwitchTile) {
				DrawBox(Left, Top, Right, Bottom, GetColor(80, 190, 110), TRUE);
				DrawString(Left + 10, Top + 7, "S", GetColor(255, 255, 255));
			}
			if (Definition->AutoTogglePeriod > 0) {
				DrawString(Left + 10, Top + 7, "T", GetColor(255, 255, 255));
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
			const uchinoko::TileRuntimeState* State = Runtime_.TryGet({Column, Row});
			if (State != nullptr && State->Count > 0) {
				DrawFormatString(
					Left + 3, Top + 18, GetColor(255, 255, 255),
					"%d", State->Count);
			}
		}
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
		}
	}

	const uchinoko::CharacterBody& Body = Player_.Body();
	DrawBox(
		static_cast<int>(Body.Position.X), static_cast<int>(Body.Position.Y),
		static_cast<int>(Body.Position.X + Body.Width),
		static_cast<int>(Body.Position.Y + Body.Height),
		GetColor(240, 210, 80), TRUE);

	DrawString(16, 16,
		"Gimmick test: Left/Right move, Z jump, R reload, Esc menu",
		GetColor(255, 255, 255));
	DrawFormatString(16, 40, GetColor(255, 255, 255),
		"Coins:%d HP+:%d Lives+:%d Score:%d Broken:%d Switch:%s Timer:%02d",
		Coins_, Health_, Lives_, Score_, Broken_,
		World_.GetSwitch(0) ? "ON" : "OFF",
		World_.GetAutoToggleCounter(1));
	DrawString(16, 64,
		"?:item / col14-15:timed / col18:S+19-20:ONOFF / col22:S crush",
		GetColor(220, 220, 220));
	if (Dead_) {
		DrawString(16, 88,
			"CRUSHED - InstantDeath (R: reload)",
			GetColor(255, 100, 100));
	}
}
