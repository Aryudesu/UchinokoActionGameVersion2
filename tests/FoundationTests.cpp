#include "../Actgame/Foundation/AssetPaths.h"
#include "../Actgame/Foundation/GridDataLoader.h"
#include "../Actgame/Foundation/StageDefinition.h"
#include "../Actgame/Foundation/TileMap.h"

#include <cassert>
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

} // namespace

int main() {
	TestAssetPaths();
	TestGridDataLoader();
	TestTileMapBounds();
	TestGameModes();
	std::cout << "All foundation tests passed.\n";
	return 0;
}
