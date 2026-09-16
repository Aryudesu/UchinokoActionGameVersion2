#include "SlopeSandboxScene.h"

#include "Conf.h"
#include "DxLib.h"
#include "InputKey.h"
#include "SceneChanger.h"
#include "Foundation/TerrainStageLoader.h"

#include <utility>

SlopeSandboxScene::SlopeSandboxScene() {
	Reload();
}

void SlopeSandboxScene::Reload() {
	uchinoko::Result<uchinoko::TerrainStageData> Loaded =
		uchinoko::TerrainStageLoader::Load("dat/stage/slope-test/stage.ini");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}
	Map_ = std::move(Loaded.Value().Map);
	Catalog_ = std::move(Loaded.Value().Catalog);
	uchinoko::CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	Player_ = uchinoko::CharacterController(Body);
	LoadError_.clear();
}

void SlopeSandboxScene::update() {
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
}

void SlopeSandboxScene::draw() {
	if (!LoadError_.empty()) {
		DrawString(16, 16, "Stage load error (R: retry, Esc: menu)", GetColor(255, 100, 100));
		DrawString(16, 44, LoadError_.c_str(), GetColor(255, 255, 255));
		return;
	}
	const unsigned int SolidColor = GetColor(70, 130, 190);
	const unsigned int SlopeColor = GetColor(90, 180, 120);
	const unsigned int GentleColor = GetColor(110, 170, 220);
	const unsigned int SteepColor = GetColor(220, 140, 90);
	for (int Row = 0; Row < Map_.Height(); ++Row) {
		for (int Column = 0; Column < Map_.Width(); ++Column) {
			const int* Id = Map_.TryGet({Column, Row});
			const uchinoko::TileDefinition* Definition = Id == nullptr ? nullptr : Catalog_.Find(*Id);
			if (Definition == nullptr) continue;
			const int Left = Column * Map_.TileWidth();
			const int Top = Row * Map_.TileHeight();
			const int Right = Left + Map_.TileWidth();
			const int Bottom = Top + Map_.TileHeight();
			switch (Definition->Collision) {
			case uchinoko::CollisionShape::Solid:
				DrawBox(Left, Top, Right, Bottom, SolidColor, TRUE);
				break;
			case uchinoko::CollisionShape::SlopeUpRight:
				DrawTriangle(Left, Bottom, Right, Top, Right, Bottom, SlopeColor, TRUE);
				break;
			case uchinoko::CollisionShape::SlopeUpLeft:
				DrawTriangle(Left, Top, Left, Bottom, Right, Bottom, SlopeColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair2x1UpRightLow:
				DrawTriangle(Left, Bottom, Right, Top + Map_.TileHeight() / 2,
					Right, Bottom, GentleColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair2x1UpRightHigh:
				DrawQuadrangle(Left, Top + Map_.TileHeight() / 2, Right, Top,
					Right, Bottom, Left, Bottom, GentleColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair2x1UpLeftHigh:
				DrawQuadrangle(Left, Top, Right, Top + Map_.TileHeight() / 2,
					Right, Bottom, Left, Bottom, GentleColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair2x1UpLeftLow:
				DrawTriangle(Left, Top + Map_.TileHeight() / 2, Left, Bottom,
					Right, Bottom, GentleColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair1x2UpRightBottom:
				DrawTriangle(Left, Bottom, Left + Map_.TileWidth() / 2, Top,
					Left + Map_.TileWidth() / 2, Bottom, SteepColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair1x2UpRightTop:
				DrawTriangle(Left + Map_.TileWidth() / 2, Bottom, Right, Top,
					Right, Bottom, SteepColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair1x2UpLeftTop:
				DrawTriangle(Left, Top, Left, Bottom,
					Left + Map_.TileWidth() / 2, Bottom, SteepColor, TRUE);
				break;
			case uchinoko::CollisionShape::Stair1x2UpLeftBottom:
				DrawTriangle(Left + Map_.TileWidth() / 2, Top, Right, Bottom,
					Left + Map_.TileWidth() / 2, Bottom, SteepColor, TRUE);
				break;
			default:
				break;
			}
		}
	}
	const uchinoko::CharacterBody& Body = Player_.Body();
	DrawBox(static_cast<int>(Body.Position.X), static_cast<int>(Body.Position.Y),
		static_cast<int>(Body.Position.X + Body.Width), static_cast<int>(Body.Position.Y + Body.Height),
		GetColor(240, 210, 80), TRUE);
	DrawCircle(static_cast<int>(Body.Position.X + Body.Width * 0.5f),
		static_cast<int>(Body.Position.Y + Body.Height), 3, GetColor(255, 80, 80), TRUE);
	DrawCircle(static_cast<int>(Body.Position.X + Body.Width * 0.5f),
		static_cast<int>(Body.Position.Y + Body.Height * 0.5f), 3, GetColor(255, 80, 80), TRUE);
	DrawCircle(static_cast<int>(Body.Position.X + Body.Width * 0.5f),
		static_cast<int>(Body.Position.Y), 3, GetColor(255, 80, 80), TRUE);
	DrawString(16, 16, "Slope/Stair test: Left/Right move, Z jump, R reload, Esc menu", GetColor(255, 255, 255));
	DrawString(16, 40, Body.Grounded ? "Grounded" : "Airborne", GetColor(255, 255, 255));
}
