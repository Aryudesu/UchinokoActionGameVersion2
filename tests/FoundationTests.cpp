#include "../Actgame/Foundation/AssetPaths.h"
#include "../Actgame/Foundation/GridDataLoader.h"
#include "../Actgame/Foundation/LayeredMap.h"
#include "../Actgame/Foundation/StageDefinition.h"
#include "../Actgame/Foundation/TerrainCollision.h"
#include "../Actgame/Foundation/TileDefinition.h"
#include "../Actgame/Foundation/TileMap.h"

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

} // namespace

int main() {
	TestAssetPaths();
	TestGridDataLoader();
	TestTileMapBounds();
	TestGameModes();
	TestTileCatalog();
	TestSlopeSurfaces();
	TestSlopeGroundSnap();
	TestLayeredMap();
	std::cout << "All foundation tests passed.\n";
	return 0;
}
