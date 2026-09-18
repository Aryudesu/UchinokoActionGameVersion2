#include "../Actgame/Foundation/AssetPaths.h"
#include "../Actgame/Foundation/CharacterController.h"
#include "../Actgame/Foundation/GridDataLoader.h"
#include "../Actgame/Foundation/LayeredMap.h"
#include "../Actgame/Foundation/StageDefinition.h"
#include "../Actgame/Foundation/TerrainCollision.h"
#include "../Actgame/Foundation/TerrainStageLoader.h"
#include "../Actgame/Foundation/TileDefinition.h"
#include "../Actgame/Foundation/TileMap.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace uchinoko;

namespace {

void TestAssetPaths() {
	AssetPaths Paths("dat/");
	assert(Paths.Image("player.bmp") == "dat/img/player.bmp");
	assert(Paths.SoundEffect("jump.wav") == "dat/SE/jump.wav");
	assert(Paths.Bgm("stage.wav") == "dat/BGM/stage.wav");
	assert(Paths.Stage(3, "Data0.inf") == "dat/stage/3/Data0.inf");
}

void TestGridDataLoader() {
	Result<IntegerGrid> Loaded = GridDataLoader::Parse("1,2,3\r\n4,999,-2\r\n");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().size() == 2);
	assert(Loaded.Value()[1][1] == 999);

	Result<IntegerGrid> Broken = GridDataLoader::Parse("1,2\n3\n");
	assert(Broken.IsFailure());
	assert(GridDataLoader::Parse("1,x\n").IsFailure());
	assert(GridDataLoader::Parse("\xEF\xBB\xBF" "7,8\n").IsSuccess());
}

void TestTileMapBounds() {
	IntegerGrid Tiles = {{1, 2, 3}, {4, 5, 6}};
	Result<TileMap> Created = TileMap::Create(Tiles);
	assert(Created.IsSuccess());
	TileMap Map = Created.Value();

	TilePosition Position;
	assert(Map.TryWorldToTile({31.0f, 32.0f}, Position));
	assert(Position.Column == 0);
	assert(Position.Row == 1);
	assert(*Map.TryGet(Position) == 4);
	assert(!Map.TryWorldToTile({-1.0f, 0.0f}, Position));
	assert(!Map.TryWorldToTile({96.0f, 0.0f}, Position));
	assert(Map.TryGet({3, 0}) == nullptr);
	int* MutableTile = Map.TryGet({1, 0});
	assert(MutableTile != nullptr);
	*MutableTile = 9;
	assert(*Map.TryGet({1, 0}) == 9);
}

void TestGameModes() {
	assert(ParseGameMode(0).Value() == GameMode::Action);
	assert(ParseGameMode(1).Value() == GameMode::Sokoban);
	assert(ParseGameMode(2).IsFailure());
}

bool NearlyEqual(float Left, float Right) {
	return std::fabs(Left - Right) < 0.001f;
}

void TestExternalTerrainStage() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/slope-test/stage.ini");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().Map.Width() == 24);
	assert(Loaded.Value().Map.Height() == 14);
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.X, 64.0f));
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.Y, 320.0f));
	assert(Loaded.Value().Catalog.Find(4)->Collision ==
		CollisionShape::Stair2x1UpRightLow);
	assert(Loaded.Value().Catalog.Find(9)->Collision ==
		CollisionShape::Stair1x2UpRightTop);
	assert(*Loaded.Value().Map.TryGet({18, 9}) == 9);
}

TileMap MakeMap(IntegerGrid Tiles) {
	Result<TileMap> Created = TileMap::Create(Tiles);
	assert(Created.IsSuccess());
	return Created.Value();
}

void TestTileCatalog() {
	TileCatalog Catalog;
	TileDefinition Slope;
	Slope.Id = 10;
	Slope.ImageIndex = 3;
	Slope.Collision = CollisionShape::SlopeUpRight;
	assert(Catalog.Register(Slope).IsSuccess());
	assert(Catalog.Find(10) != nullptr);
	assert(Catalog.Find(10)->ImageIndex == 3);
	assert(Catalog.Find(999) == nullptr);
	assert(Catalog.Register(Slope).IsFailure());
}

void TestSlopeSurfaces() {
	float SurfaceY = 0.0f;
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::SlopeUpRight, {2, 1}, 64.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 64.0f));
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::SlopeUpRight, {2, 1}, 80.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 48.0f));
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::SlopeUpRight, {2, 1}, 96.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 32.0f));

	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::SlopeUpLeft, {2, 1}, 64.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 32.0f));
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::SlopeUpLeft, {2, 1}, 96.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 64.0f));
	assert(!TerrainCollision::TryGetSurfaceY(
		CollisionShape::None, {0, 0}, 0.0f, 32, 32, SurfaceY));
}

void TestStairSurfaces() {
	float SurfaceY = 0.0f;
	// 2x1 は2タイルをつないで 64px 進む間に 32px 上がる。
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::Stair2x1UpRightLow, {0, 1}, 16.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 56.0f));
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::Stair2x1UpRightHigh, {1, 1}, 48.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 40.0f));

	// 1x2 は上下2タイルの半幅ずつを使い、32px 進む間に 64px 上がる。
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::Stair1x2UpRightBottom, {0, 2}, 8.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 80.0f));
	assert(!TerrainCollision::TryGetSurfaceY(
		CollisionShape::Stair1x2UpRightBottom, {0, 2}, 24.0f, 32, 32, SurfaceY));
	assert(TerrainCollision::TryGetSurfaceY(
		CollisionShape::Stair1x2UpRightTop, {0, 1}, 24.0f, 32, 32, SurfaceY));
	assert(NearlyEqual(SurfaceY, 48.0f));
}

void TestSlopeSolidRegions() {
	assert(TerrainCollision::ContainsSolidPoint(
		CollisionShape::SlopeUpRight, {0, 0}, {16.0f, 24.0f}, 32, 32));
	assert(!TerrainCollision::ContainsSolidPoint(
		CollisionShape::SlopeUpRight, {0, 0}, {16.0f, 8.0f}, 32, 32));
	assert(!TerrainCollision::ContainsSolidPoint(
		CollisionShape::OneWay, {0, 0}, {16.0f, 24.0f}, 32, 32));
	// 1x2急坂の下段は、斜面がない側の半分も実体で埋まっている。
	assert(TerrainCollision::ContainsSolidPoint(
		CollisionShape::Stair1x2UpRightBottom, {0, 0}, {24.0f, 1.0f}, 32, 32));
	assert(TerrainCollision::ContainsSolidPoint(
		CollisionShape::Stair1x2UpLeftBottom, {0, 0}, {8.0f, 1.0f}, 32, 32));
}

void TestSlopeSideBlocks() {
	float Top = 0.0f;
	float Bottom = 0.0f;
	using Side = TerrainCollision::TileSide;
	assert(!TerrainCollision::TryGetSideBlock(
		CollisionShape::SlopeUpRight, {0, 1}, Side::Left, 32, 32, Top, Bottom));
	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::SlopeUpRight, {0, 1}, Side::Right, 32, 32, Top, Bottom));
	assert(NearlyEqual(Top, 32.0f));
	assert(NearlyEqual(Bottom, 64.0f));
	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::SlopeUpLeft, {0, 1}, Side::Left, 32, 32, Top, Bottom));
	assert(!TerrainCollision::TryGetSideBlock(
		CollisionShape::SlopeUpLeft, {0, 1}, Side::Right, 32, 32, Top, Bottom));

	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::Stair2x1UpRightLow, {0, 1}, Side::Right, 32, 32, Top, Bottom));
	assert(NearlyEqual(Top, 48.0f));
	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::Stair2x1UpLeftLow, {0, 1}, Side::Left, 32, 32, Top, Bottom));
	assert(NearlyEqual(Top, 48.0f));

	// 1x2 の下半分は、急斜面を越えた側がタイル全面で埋まる。
	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::Stair1x2UpRightBottom, {0, 2}, Side::Right, 32, 32, Top, Bottom));
	assert(NearlyEqual(Top, 64.0f));
	assert(!TerrainCollision::TryGetSideBlock(
		CollisionShape::Stair1x2UpRightBottom, {0, 2}, Side::Left, 32, 32, Top, Bottom));
	assert(TerrainCollision::TryGetSideBlock(
		CollisionShape::Stair1x2UpLeftBottom, {0, 2}, Side::Left, 32, 32, Top, Bottom));
	assert(!TerrainCollision::TryGetSideBlock(
		CollisionShape::OneWay, {0, 1}, Side::Left, 32, 32, Top, Bottom));
}

void TestSlopeGroundSnap() {
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	assert(Catalog.Register(Empty).IsSuccess());
	TileDefinition DownSlope;
	DownSlope.Id = 1;
	DownSlope.Collision = CollisionShape::SlopeUpLeft;
	assert(Catalog.Register(DownSlope).IsSuccess());
	TileDefinition UpSlope;
	UpSlope.Id = 2;
	UpSlope.Collision = CollisionShape::SlopeUpRight;
	assert(Catalog.Register(UpSlope).IsSuccess());

	TileMap Map = MakeMap({{0, 0}, {1, 2}, {0, 0}});
	GroundHit Hit;
	// 右へ8px進むと坂面も8px下がる。落下前でも足元探索で接地を保てる。
	assert(TerrainCollision::FindGround(Map, Catalog, {24.0f, 48.0f}, 4.0f, 8.0f, Hit));
	assert(NearlyEqual(Hit.SurfaceY, 56.0f));
	assert(Hit.Shape == CollisionShape::SlopeUpLeft);
	// 右上がり坂では同じ探索が上方向への追従にも使える。
	assert(TerrainCollision::FindGround(Map, Catalog, {40.0f, 64.0f}, 8.0f, 4.0f, Hit));
	assert(NearlyEqual(Hit.SurfaceY, 56.0f));
	assert(Hit.Shape == CollisionShape::SlopeUpRight);
	assert(!TerrainCollision::FindGround(Map, Catalog, {-1.0f, 0.0f}, 8.0f, 8.0f, Hit));
	assert(!TerrainCollision::FindGround(Map, Catalog, {100.0f, 0.0f}, 8.0f, 8.0f, Hit));
}

void TestLayeredMap() {
	TileMap Terrain = MakeMap({{1, 1}, {1, 1}});
	TileMap Visual = MakeMap({{2, 2}, {2, 2}});
	TileMap Object = MakeMap({{0, 0}, {3, 0}});
	TileMap Event = MakeMap({{0, 4}, {0, 0}});
	Result<LayeredMap> Created = LayeredMap::Create(Terrain, Visual, Object, Event);
	assert(Created.IsSuccess());
	assert(*Created.Value().Layer(MapLayerKind::Object).TryGet({0, 1}) == 3);
	assert(*Created.Value().Layer(MapLayerKind::Event).TryGet({1, 0}) == 4);

	TileMap WrongSize = MakeMap({{0}});
	assert(LayeredMap::Create(Terrain, Visual, Object, WrongSize).IsFailure());
}

TileCatalog MakeTerrainCatalog() {
	TileCatalog Catalog;
	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());
	TileDefinition UpRight;
	UpRight.Id = 2;
	UpRight.Collision = CollisionShape::SlopeUpRight;
	assert(Catalog.Register(UpRight).IsSuccess());
	TileDefinition UpLeft;
	UpLeft.Id = 3;
	UpLeft.Collision = CollisionShape::SlopeUpLeft;
	assert(Catalog.Register(UpLeft).IsSuccess());
	const CollisionShape StairShapes[] = {
		CollisionShape::Stair2x1UpRightLow,
		CollisionShape::Stair2x1UpRightHigh,
		CollisionShape::Stair2x1UpLeftHigh,
		CollisionShape::Stair2x1UpLeftLow,
		CollisionShape::Stair1x2UpRightBottom,
		CollisionShape::Stair1x2UpRightTop,
		CollisionShape::Stair1x2UpLeftTop,
		CollisionShape::Stair1x2UpLeftBottom
	};
	for (int Index = 0; Index < 8; ++Index) {
		TileDefinition Stair;
		Stair.Id = 4 + Index;
		Stair.Collision = StairShapes[Index];
		assert(Catalog.Register(Stair).IsSuccess());
	}
	return Catalog;
}

void TestCharacterMovement() {
	TileMap FlatMap = MakeMap({{0, 0, 0}, {1, 1, 1}, {0, 0, 0}});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(1.0f, false, FlatMap, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, 7.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 0.0f));
	assert(Player.Body().Grounded);
	Player.Step(0.0f, true, FlatMap, Catalog);
	assert(!Player.Body().Grounded);
	assert(Player.Body().Velocity.Y < 0.0f);
	assert(Player.Body().Position.Y < 0.0f);
	for (int Frame = 0; Frame < 60; ++Frame) Player.Step(0.0f, false, FlatMap, Catalog);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 0.0f));
}

void AssertCharacterCenterOnGround(
	const CharacterController& Player, const TileMap& Map, const TileCatalog& Catalog) {
	if (!Player.Body().Grounded) return;
	const float CenterX = Player.Body().Position.X + 15.0f;
	GroundHit Hit;
	assert(TerrainCollision::FindGround(
		Map, Catalog, {CenterX, Player.Body().Position.Y + 31.0f},
		Player.Body().Height, Player.Body().Height, Hit));
	float ExpectedY = Hit.SurfaceY - 32.0f;
	const float LocalX = CenterX - Hit.Tile.Column * Map.TileWidth();
	if (Hit.Shape == CollisionShape::SlopeUpRight) {
		ExpectedY = Hit.Tile.Row * Map.TileHeight() - std::floor(LocalX);
	} else if (Hit.Shape == CollisionShape::SlopeUpLeft) {
		ExpectedY = Hit.Tile.Row * Map.TileHeight() + std::floor(LocalX) - 31.0f;
	}
	assert(NearlyEqual(Player.Body().Position.Y, ExpectedY));
}

void TestCharacterRecomputesGroundFromMasaoProbes() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{1, 1, 1},
		{0, 0, 0}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 0.0f};
	// 呼び出し側の値が誤っていても、x+15, y+32 から接地を再構築する。
	Body.Grounded = false;
	CharacterController OnFloor(Body);
	OnFloor.Step(0.0f, false, Map, Catalog);
	assert(OnFloor.Body().Grounded);
	assert(NearlyEqual(OnFloor.Body().Position.Y, 0.0f));
	Body.Position = {4.0f, 0.5f};
	Body.Velocity = {0.0f, 0.5f};
	Body.Grounded = false;
	CharacterController SlightlyInsideFloor(Body);
	SlightlyInsideFloor.Step(0.0f, false, Map, Catalog);
	assert(SlightlyInsideFloor.Body().Grounded);
	assert(NearlyEqual(SlightlyInsideFloor.Body().Position.Y, 0.0f));

	Body.Position = {4.0f, -48.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController InAir(Body);
	InAir.Step(0.0f, false, Map, Catalog);
	assert(!InAir.Body().Grounded);
	assert(InAir.Body().Position.Y > -48.0f);
}

void TestCharacterUsesGetSakamichiYCoordinates() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 2, 3},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {33.0f, 32.0f}; // 中央X=48、右上がり坂の localX=16
	Body.Grounded = false;
	CharacterController UpRight(Body);
	UpRight.Step(0.0f, false, Map, Catalog);
	assert(UpRight.Body().Grounded);
	assert(NearlyEqual(UpRight.Body().Position.Y, 16.0f));

	Body.Position = {65.0f, 32.0f}; // 中央X=80、左上がり坂の localX=16
	Body.Velocity = {0.0f, 0.0f};
	CharacterController UpLeft(Body);
	UpLeft.Step(0.0f, false, Map, Catalog);
	assert(UpLeft.Body().Grounded);
	assert(NearlyEqual(UpLeft.Body().Position.Y, 17.0f));
}

void TestCharacterSlopeFollow() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 2, 1, 3, 0},
		{1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 17; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		AssertCharacterCenterOnGround(Player, Map, Catalog);
	}
	assert(Player.Body().Grounded);
	assert(Player.Body().Position.Y < 32.0f);
	for (int Frame = 0; Frame < 25; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		AssertCharacterCenterOnGround(Player, Map, Catalog);
	}
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
}

void TestCharacterLeavesSlopePeakWithoutWarpingToLowerFloor() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 2, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 右上がり坂の頂上直前。右へ抜けた先は空白で、その下に床がある。
	Body.Position = {48.0f, 1.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(1.0f, false, Map, Catalog);

	assert(Player.Body().Position.X > 48.0f);
	assert(!Player.Body().Grounded);
	// 下段床の Y=32 へ瞬間移動せず、頂上付近から通常落下を始める。
	assert(Player.Body().Position.Y < 10.0f);

	TileMap ReverseMap = MakeMap({
		{0, 0, 0, 0},
		{0, 0, 3, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	});
	Body.Position = {50.0f, 2.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController ReversePlayer(Body);
	ReversePlayer.Step(-1.0f, false, ReverseMap, Catalog);
	assert(ReversePlayer.Body().Position.X < 50.0f);
	assert(!ReversePlayer.Body().Grounded);
	assert(ReversePlayer.Body().Position.Y < 10.0f);
}

void TestCharacterWall() {
	TileMap Map = MakeMap({{0, 1, 0}, {0, 1, 0}, {1, 1, 1}});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 10; ++Frame) Player.Step(1.0f, false, Map, Catalog);
	// 横壁は身体中央がタイル境界へ到達した位置で止まる。
	assert(NearlyEqual(Player.Body().Position.X, 16.0f));
}

void TestCharacterSideUsesTopAndBottomProbes() {
	TileMap Map = MakeMap({
		{0, 1, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 身体中央は上段ブロックより下だが、頭側はブロックに重なっている。
	Body.Position = {4.0f, 24.0f};
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);
	for (int Frame = 0; Frame < 8; ++Frame) Player.Step(1.0f, false, Map, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, 16.0f));
}

void TestCharacterDropsFromBlockWithoutCornerSnag() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 1, 0, 0},
		{1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {36.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 20; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
	}
	// 足元中央が段差を越えたら、矩形の角に止められず下の床へ降りる。
	assert(Player.Body().Position.X > 80.0f);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
}

void TestCharacterCeiling() {
	TileMap Map = MakeMap({{1, 0}, {0, 0}, {1, 1}});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float MinimumY = Body.Position.Y;
	Player.Step(0.0f, true, Map, Catalog);
	for (int Frame = 0; Frame < 10; ++Frame) {
		Player.Step(0.0f, false, Map, Catalog);
		MinimumY = std::min(MinimumY, Player.Body().Position.Y);
	}
	assert(MinimumY >= 32.0f);
}

void TestCharacterCeilingUsesCenterPoint() {
	TileMap Map = MakeMap({
		{1, 0, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 左上角は天井ブロックの下にあるが、頭上中央点は右隣の空間にある。
	Body.Position = {24.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float MinimumY = Body.Position.Y;
	Player.Step(0.0f, true, Map, Catalog);
	for (int Frame = 0; Frame < 10; ++Frame) {
		Player.Step(0.0f, false, Map, Catalog);
		MinimumY = std::min(MinimumY, Player.Body().Position.Y);
	}
	assert(MinimumY < 32.0f);
}

void TestCharacterHitsSlopeFromBelow() {
	TileMap Map = MakeMap({
		{2, 0},
		{0, 0},
		{1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float MinimumY = Body.Position.Y;
	Player.Step(0.0f, true, Map, Catalog);
	for (int Frame = 0; Frame < 10; ++Frame) {
		Player.Step(0.0f, false, Map, Catalog);
		MinimumY = std::min(MinimumY, Player.Body().Position.Y);
	}
	assert(MinimumY >= 32.0f);
}

void TestCharacterCeilingSeamUsesDirectionalProbe() {
	TileMap Map = MakeMap({
		{1, 0, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 頭上中央X=32はタイル境界。正男では左入力中だけ x+14 を補助確認する。
	Body.Position = {17.0f, 32.0f};
	Body.Grounded = true;
	CharacterMotion Motion;
	Motion.MoveSpeed = 0.0f;
	CharacterController Player(Body, Motion);
	float MinimumY = Body.Position.Y;
	Player.Step(-1.0f, true, Map, Catalog);
	for (int Frame = 0; Frame < 10; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		MinimumY = std::min(MinimumY, Player.Body().Position.Y);
	}
	assert(MinimumY >= 32.0f);
}

void TestCharacterCanJumpWhileTouchingSlopeTip() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 2, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 足元中央は右上がり坂の低い先端にあり、頭上中央も同じ坂タイル内にある。
	Body.Position = {21.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(0.0f, true, Map, Catalog);
	assert(!Player.Body().Grounded);
	assert(Player.Body().Velocity.Y < 0.0f);

	// 坂タイルの外側なら通常どおりジャンプできる。
	Body.Position = {16.0f, 32.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController OutsidePlayer(Body);
	OutsidePlayer.Step(0.0f, true, Map, Catalog);
	assert(!OutsidePlayer.Body().Grounded);
	assert(OutsidePlayer.Body().Velocity.Y < 0.0f);
}

void TestCharacterLandingAcrossSlope() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 0, 0, 3, 0},
		{1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 足元中央が坂面の上から下降して横切る、通常の着地経路を再現する。
	Body.Position = {97.0f, 16.0f};
	Body.Velocity.Y = 2.0f;
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.MoveSpeed = 0.0f;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);
	Player.Step(0.0f, false, Map, Catalog);
	assert(Player.Body().Grounded);
	// getSakamichiY の式により、x+15=112 ではキャラクターY=17で止まる。
	assert(NearlyEqual(Player.Body().Position.Y, 17.0f));
}

void TestCharacterFollowsStairs() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap GentleMap = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 4, 5, 1, 0},
		{1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {4.0f, 32.0f};
	Body.Grounded = true;
	CharacterController GentlePlayer(Body);
	for (int Frame = 0; Frame < 30; ++Frame) GentlePlayer.Step(1.0f, false, GentleMap, Catalog);
	assert(GentlePlayer.Body().Grounded);
	assert(GentlePlayer.Body().Position.Y < 10.0f);

	TileMap SteepMap = MakeMap({
		{0, 0, 0, 0},
		{0, 9, 1, 0},
		{0, 8, 0, 0},
		{1, 1, 1, 1}
	});
	Body.Position = {4.0f, 64.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController SteepPlayer(Body);
	for (int Frame = 0; Frame < 20; ++Frame) SteepPlayer.Step(1.0f, false, SteepMap, Catalog);
	assert(SteepPlayer.Body().Grounded);
	assert(SteepPlayer.Body().Position.Y < 10.0f);
}

void TestCharacterCannotEnterSlopeHighSide() {
	TileMap UpRightMap = MakeMap({
		{0, 0, 0},
		{0, 2, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {50.0f, 32.0f};
	Body.Grounded = true;
	CharacterController FromRight(Body);
	FromRight.Step(-1.0f, false, UpRightMap, Catalog);
	assert(NearlyEqual(FromRight.Body().Position.X, 49.0f));

	// 低い側からは従来どおり坂へ進入して登れる。
	Body.Position = {8.0f, 32.0f};
	Body.Velocity = {0.0f, 0.0f};
	CharacterController FromLeft(Body);
	FromLeft.Step(1.0f, false, UpRightMap, Catalog);
	assert(FromLeft.Body().Position.X > 8.0f);
	assert(FromLeft.Body().Grounded);

	TileMap UpLeftMap = MakeMap({
		{0, 0, 0},
		{0, 3, 0},
		{1, 1, 1}
	});
	Body.Position = {16.0f, 32.0f};
	Body.Velocity = {0.0f, 0.0f};
	CharacterController LeftHighSide(Body);
	LeftHighSide.Step(1.0f, false, UpLeftMap, Catalog);
	assert(NearlyEqual(LeftHighSide.Body().Position.X, 16.0f));
}

void TestRisingCharacterCannotPassSlopeSideInsideColumn() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 2, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 右側から坂の列へ入った時点では身体中央が側壁より下にある。
	// その後、同じ列内で上昇して側壁の高さへ入っても通過させない。
	Body.Position = {50.0f, 60.0f};
	Body.Velocity.Y = -20.0f;
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);
	Player.Step(-1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X < 50.0f);
	Player.Step(-1.0f, false, Map, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, 49.0f));
	assert(NearlyEqual(Player.Body().Velocity.X, 0.0f));
}

void TestCharacterDescendsSteepSlopeWithoutSidePushback() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 9, 1, 10, 0},
		{0, 8, 0, 11, 0},
		{1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {68.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousX = Player.Body().Position.X;
	for (int Frame = 0; Frame < 12; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		// 下り坂タイル内で入口の左側面を再判定して、Xを戻してはならない。
		assert(Player.Body().Position.X >= PreviousX);
		PreviousX = Player.Body().Position.X;
	}
	assert(Player.Body().Position.X > 90.0f);
	assert(Player.Body().Grounded);
}

void TestCharacterUsesCenterAcrossSteepSlopePeak() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 9, 1, 10, 0},
		{0, 8, 0, 11, 0},
		{1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {4.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 35; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		AssertCharacterCenterOnGround(Player, Map, Catalog);
	}
	Body.Position = {132.0f, 64.0f};
	Body.Velocity = {0.0f, 0.0f};
	CharacterController ReversePlayer(Body);
	for (int Frame = 0; Frame < 35; ++Frame) {
		ReversePlayer.Step(-1.0f, false, Map, Catalog);
		AssertCharacterCenterOnGround(ReversePlayer, Map, Catalog);
	}

}

void TestCharacterStopsAtOverlappingSlopeSide() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 2, 0},
		{1, 1, 1},
		{1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 坂三角形の内部にいる状態を再現する。正男はフレーム冒頭で坂面へ補正する。
	Body.Position = {45.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(-1.0f, false, Map, Catalog);
	assert(Player.Body().Grounded);
	assert(Player.Body().Position.X < 45.0f);
	assert(Player.Body().Position.Y < 32.0f);
}

void TestCharacterMovesPastSlopeSideAfterJumpingAboveIt() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	// 2x1 坂の右側面に接した状態から、左を押したままジャンプする。
	Body.Position = {81.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	const float WallX = Player.Body().Position.X;

	Player.Step(-1.0f, true, Map, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, WallX));
	assert(!Player.Body().Grounded);

	// 身体と坂の右側面が重なる間は、左入力でも壁の手前に留まる。
	while (Player.Body().Position.Y + 31.0f >= 32.0f) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(NearlyEqual(Player.Body().Position.X, WallX));
	}

	// 坂の上の空白へ身体が抜けたら、そのフレーム以降は左へ移動できる。
	Player.Step(-1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X < WallX);
}

void TestExternalStageWalkingFollowsCenterGround() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/slope-test/stage.ini");
	assert(Loaded.IsSuccess());
	CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 240; ++Frame) {
		Player.Step(1.0f, false, Loaded.Value().Map, Loaded.Value().Catalog);
		AssertCharacterCenterOnGround(Player, Loaded.Value().Map, Loaded.Value().Catalog);
	}
}

void TestExternalStageSpawnStaysOnFloor() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/slope-test/stage.ini");
	assert(Loaded.IsSuccess());
	CharacterBody Body;
	Body.Position = Loaded.Value().PlayerSpawn;
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 60; ++Frame) {
		Player.Step(0.0f, false, Loaded.Value().Map, Loaded.Value().Catalog);
		assert(Player.Body().Grounded);
		assert(NearlyEqual(Player.Body().Position.Y, 320.0f));
	}
}

} // namespace

int main() {
	TestAssetPaths();
	TestGridDataLoader();
	TestExternalTerrainStage();
	TestTileMapBounds();
	TestGameModes();
	TestTileCatalog();
	TestSlopeSurfaces();
	TestStairSurfaces();
	TestSlopeSolidRegions();
	TestSlopeSideBlocks();
	TestSlopeGroundSnap();
	TestLayeredMap();
	TestCharacterMovement();
	TestCharacterRecomputesGroundFromMasaoProbes();
	TestCharacterUsesGetSakamichiYCoordinates();
	TestCharacterSlopeFollow();
	TestCharacterLeavesSlopePeakWithoutWarpingToLowerFloor();
	TestCharacterWall();
	TestCharacterSideUsesTopAndBottomProbes();
	TestCharacterDropsFromBlockWithoutCornerSnag();
	TestCharacterCeiling();
	TestCharacterCeilingUsesCenterPoint();
	TestCharacterHitsSlopeFromBelow();
	TestCharacterCeilingSeamUsesDirectionalProbe();
	TestCharacterCanJumpWhileTouchingSlopeTip();
	TestCharacterLandingAcrossSlope();
	TestCharacterFollowsStairs();
	TestCharacterCannotEnterSlopeHighSide();
	TestRisingCharacterCannotPassSlopeSideInsideColumn();
	TestCharacterDescendsSteepSlopeWithoutSidePushback();
	TestCharacterUsesCenterAcrossSteepSlopePeak();
	TestCharacterStopsAtOverlappingSlopeSide();
	TestCharacterMovesPastSlopeSideAfterJumpingAboveIt();
	TestExternalStageSpawnStaysOnFloor();
	TestExternalStageWalkingFollowsCenterGround();
	std::cout << "All foundation tests passed.\n";
	return 0;
}
