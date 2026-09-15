#include "SlopeSandboxScene.h"

#include "Conf.h"
#include "DxLib.h"
#include "InputKey.h"
#include "SceneChanger.h"

#include <utility>

namespace {

uchinoko::TileMap CreateTestMap() {
	uchinoko::IntegerGrid Tiles(SCREENY, std::vector<int>(SCREENX, 0));
	for (int Column = 0; Column < SCREENX; ++Column) Tiles[11][Column] = 1;
	Tiles[10][5] = 2;
	Tiles[10][6] = 1;
	Tiles[10][7] = 1;
	Tiles[10][8] = 1;
	Tiles[10][9] = 3;
	Tiles[8][13] = 1;
	Tiles[8][14] = 1;
	Tiles[8][15] = 1;
	return uchinoko::TileMap::Create(std::move(Tiles)).Value();
}

uchinoko::TileCatalog CreateCatalog() {
	uchinoko::TileCatalog Catalog;
	uchinoko::TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = uchinoko::CollisionShape::Solid;
	Catalog.Register(Solid);
	uchinoko::TileDefinition UpRight;
	UpRight.Id = 2;
	UpRight.Collision = uchinoko::CollisionShape::SlopeUpRight;
	Catalog.Register(UpRight);
	uchinoko::TileDefinition UpLeft;
	UpLeft.Id = 3;
	UpLeft.Collision = uchinoko::CollisionShape::SlopeUpLeft;
	Catalog.Register(UpLeft);
	return Catalog;
}

uchinoko::CharacterController CreatePlayer() {
	uchinoko::CharacterBody Body;
	Body.Position = {64.0f, 322.0f};
	Body.Grounded = true;
	return uchinoko::CharacterController(Body);
}

} // namespace

SlopeSandboxScene::SlopeSandboxScene()
	: Map_(CreateTestMap()), Catalog_(CreateCatalog()), Player_(CreatePlayer()) {}

void SlopeSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	float Horizontal = 0.0f;
	if (ReturnKey(KEY_INPUT_LEFT) != 0) Horizontal -= 1.0f;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) Horizontal += 1.0f;
	Player_.Step(Horizontal, ReturnKey(KEY_INPUT_Z) == 1, Map_, Catalog_);
}

void SlopeSandboxScene::draw() {
	const unsigned int SolidColor = GetColor(70, 130, 190);
	const unsigned int SlopeColor = GetColor(90, 180, 120);
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
			default:
				break;
			}
		}
	}
	const uchinoko::CharacterBody& Body = Player_.Body();
	DrawBox(static_cast<int>(Body.Position.X), static_cast<int>(Body.Position.Y),
		static_cast<int>(Body.Position.X + Body.Width), static_cast<int>(Body.Position.Y + Body.Height),
		GetColor(240, 210, 80), TRUE);
	DrawString(16, 16, "Slope test: Left/Right move, Z jump, Esc menu", GetColor(255, 255, 255));
	DrawString(16, 40, Body.Grounded ? "Grounded" : "Airborne", GetColor(255, 255, 255));
}
