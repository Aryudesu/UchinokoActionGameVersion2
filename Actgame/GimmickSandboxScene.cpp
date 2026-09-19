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

	uchinoko::CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	Player_ = uchinoko::CharacterController(Body);

	Coins_ = 0;
	Score_ = 0;
	Broken_ = 0;
	LoadError_.clear();
}

void GimmickSandboxScene::ApplyEffects() {
	const std::vector<uchinoko::TileEffect> Effects =
		uchinoko::TileBehaviorSystem::ApplyAll(
			Player_.Interactions(), Map_, Catalog_, Runtime_);

	for (std::size_t Index = 0; Index < Effects.size(); ++Index) {
		switch (Effects[Index].Type) {
		case uchinoko::TileEffectType::AddCoin:
			Coins_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::AddScore:
			Score_ += Effects[Index].Value;
			break;
		case uchinoko::TileEffectType::TileBroken:
			++Broken_;
			break;
		default:
			break;
		}
	}
}

void GimmickSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	if (ReturnKey(KEY_INPUT_R) == 1) Reload();
	if (!LoadError_.empty()) return;

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

			if (Definition->Collision == uchinoko::CollisionShape::Solid) {
				DrawBox(Left, Top, Right, Bottom, GetColor(80, 130, 190), TRUE);
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
		"Coins: %d  Score: %d  Broken: %d", Coins_, Score_, Broken_);
	DrawString(16, 64,
		"Yellow circles: collectible rules / outlined blocks: breakable rules",
		GetColor(220, 220, 220));
}
