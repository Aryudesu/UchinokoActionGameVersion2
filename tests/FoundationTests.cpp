#include "../Actgame/Foundation/AssetPaths.h"
#include "../Actgame/Foundation/CharacterController.h"
#include "../Actgame/Foundation/CharacterSafety.h"
#include "../Actgame/Foundation/ConditionalTerrain.h"
#include "../Actgame/Foundation/CanvasMasaoTerrain.h"
#include "../Actgame/Foundation/ExtendedSlopeTerrain.h"
#include "../Actgame/Foundation/GridDataLoader.h"
#include "../Actgame/Foundation/ItemSystem.h"
#include "../Actgame/Foundation/LayeredMap.h"
#include "../Actgame/Foundation/StageDefinition.h"
#include "../Actgame/Foundation/TerrainCollision.h"
#include "../Actgame/Foundation/TerrainStageLoader.h"
#include "../Actgame/Foundation/TileDefinition.h"
#include "../Actgame/Foundation/TileInteraction.h"
#include "../Actgame/Foundation/TileMap.h"
#include "../Actgame/Foundation/WorldState.h"

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
	TileDefinition OneWay;
	OneWay.Id = 12;
	OneWay.Collision = CollisionShape::OneWay;
	assert(Catalog.Register(OneWay).IsSuccess());
	return Catalog;
}

void TestTileRuleCatalogAndLegacyCompatibility() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Coin = Loaded.Value().Find(20);
	assert(Coin != nullptr);
	assert(Coin->Rules.size() == 3);
	assert(Coin->Rules[0].Trigger == TileTrigger::Touch);
	assert(Coin->Rules[0].Action == TileAction::AddCoin);
	assert(Coin->Rules[0].Value == 1);
	assert(Coin->Rules[0].Once);

	const TileDefinition* Breakable = Loaded.Value().Find(21);
	assert(Breakable != nullptr);
	assert(Breakable->Rules.size() == 1);
	assert(Breakable->Rules[0].Trigger == TileTrigger::HitFromBelow);
	assert(Breakable->Rules[0].Action == TileAction::BreakTile);
	assert(Breakable->Rules[0].Once);

	const TileDefinition* Damaging = Loaded.Value().Find(22);
	assert(Damaging != nullptr);
	assert(Damaging->Rules.size() == 1);
	assert(Damaging->Rules[0].Trigger == TileTrigger::Touch);
	assert(Damaging->Rules[0].Action == TileAction::Damage);
	assert(!Damaging->Rules[0].Once);
}

void TestTileBehaviorComposesEffectsWithoutManagers() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({{20}});
	TileRuntimeMap Runtime(Map);
	TileInteraction Interaction;
	Interaction.Trigger = TileTrigger::Touch;
	Interaction.Position = {0, 0};
	Interaction.TileId = 20;

	TileBehaviorResult Result =
		TileBehaviorSystem::Apply(Interaction, Map, Loaded.Value(), Runtime);
	assert(Result.Handled);
	assert(Result.Effects.size() == 2);
	assert(Result.Effects[0].Type == TileEffectType::AddCoin);
	assert(Result.Effects[0].Value == 1);
	assert(Result.Effects[1].Type == TileEffectType::AddScore);
	assert(Result.Effects[1].Value == 100);
	assert(*Map.TryGet({0, 0}) == 0);
	assert(Runtime.TryGet({0, 0})->Used);

	// 置換後の古いイベントや once ルールを再実行してはいけない。
	TileBehaviorResult Again =
		TileBehaviorSystem::Apply(Interaction, Map, Loaded.Value(), Runtime);
	assert(!Again.Handled);
	assert(Again.Effects.empty());
}

void TestLegacyBreakableBecomesRuleDriven() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({{21}});
	TileRuntimeMap Runtime(Map);
	TileInteraction Interaction;
	Interaction.Trigger = TileTrigger::HitFromBelow;
	Interaction.Position = {0, 0};
	Interaction.TileId = 21;

	TileBehaviorResult Result =
		TileBehaviorSystem::Apply(Interaction, Map, Loaded.Value(), Runtime);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::TileBroken);
	assert(*Map.TryGet({0, 0}) == 0);
	assert(Runtime.TryGet({0, 0})->Used);
}

void TestTileOnceRulesAreIndependent() {
	TileCatalog Catalog;
	TileDefinition Tile;
	Tile.Id = 30;
	Tile.Collision = CollisionShape::None;
	Tile.Rules.push_back({TileTrigger::Touch, TileAction::AddScore, 10, true});
	Tile.Rules.push_back({TileTrigger::HitFromBelow, TileAction::AddCoin, 1, true});
	assert(Catalog.Register(Tile).IsSuccess());

	TileMap Map = MakeMap({{30}});
	TileRuntimeMap Runtime(Map);

	TileInteraction Touch;
	Touch.Trigger = TileTrigger::Touch;
	Touch.Position = {0, 0};
	Touch.TileId = 30;
	TileBehaviorResult First = TileBehaviorSystem::Apply(Touch, Map, Catalog, Runtime);
	assert(First.Effects.size() == 1);
	assert(First.Effects[0].Type == TileEffectType::AddScore);

	TileInteraction Hit = Touch;
	Hit.Trigger = TileTrigger::HitFromBelow;
	TileBehaviorResult Second = TileBehaviorSystem::Apply(Hit, Map, Catalog, Runtime);
	assert(Second.Effects.size() == 1);
	assert(Second.Effects[0].Type == TileEffectType::AddCoin);

	assert(TileBehaviorSystem::Apply(Touch, Map, Catalog, Runtime).Effects.empty());
	assert(TileBehaviorSystem::Apply(Hit, Map, Catalog, Runtime).Effects.empty());
}

void TestExternalInteractionStage() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/interaction-test/stage.ini");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().Map.Width() == 24);
	assert(Loaded.Value().Map.Height() == 9);
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.X, 96.0f));
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.Y, 160.0f));
	assert(*Loaded.Value().Map.TryGet({2, 5}) == 20);
	assert(*Loaded.Value().Map.TryGet({5, 4}) == 21);
	assert(*Loaded.Value().Map.TryGet({0, 4}) == 58);
	assert(Loaded.Value().Catalog.Find(20) != nullptr);
	assert(Loaded.Value().Catalog.Find(20)->Rules.size() == 3);
	assert(Loaded.Value().Catalog.Find(21) != nullptr);
	assert(Loaded.Value().Catalog.Find(21)->Rules.size() == 1);
}

void TestItemBlockDefinitions() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* CoinBlock = Loaded.Value().Find(31);
	assert(CoinBlock != nullptr);
	assert(CoinBlock->Collision == CollisionShape::Solid);
	assert(CoinBlock->Rules.size() == 2);
	assert(CoinBlock->Rules[0].Trigger == TileTrigger::HitFromBelow);
	assert(CoinBlock->Rules[0].Action == TileAction::SpawnItem);
	assert(CoinBlock->Rules[0].Value == static_cast<int>(ItemKind::Coin));
	assert(CoinBlock->Rules[1].Action == TileAction::ReplaceTile);
	assert(CoinBlock->Rules[1].Value == 30);

	const TileDefinition* Hidden = Loaded.Value().Find(34);
	assert(Hidden != nullptr);
	assert(Hidden->Collision == CollisionShape::HitFromBelowOnly);
	assert(Hidden->Rules.size() == 2);
	assert(Hidden->Rules[0].Action == TileAction::SpawnItem);
}

void TestTenCoinBlockUsesGenericCountRules() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* TenCoin = Loaded.Value().Find(37);
	assert(TenCoin != nullptr);
	assert(TenCoin->Collision == CollisionShape::Solid);
	assert(TenCoin->Rules.size() == 3);

	assert(TenCoin->Rules[0].Action == TileAction::SpawnItem);
	assert(TenCoin->Rules[0].CountCondition == TileCountCondition::LessThan);
	assert(TenCoin->Rules[0].CountValue == 10);
	assert(TenCoin->Rules[1].Action == TileAction::IncrementCount);
	assert(TenCoin->Rules[1].Value == 1);
	assert(TenCoin->Rules[1].CountCondition == TileCountCondition::LessThan);
	assert(TenCoin->Rules[2].Action == TileAction::ReplaceTile);
	assert(TenCoin->Rules[2].Value == 30);
	assert(TenCoin->Rules[2].CountCondition == TileCountCondition::GreaterEqual);
	assert(TenCoin->Rules[2].CountValue == 10);

	TileMap Map = MakeMap({{37}});
	TileRuntimeMap Runtime(Map);
	TileInteraction Hit;
	Hit.Trigger = TileTrigger::HitFromBelow;
	Hit.Position = {0, 0};
	Hit.TileId = 37;

	std::vector<TileEffect> SpawnEffects;
	for (int HitCount = 1; HitCount <= 10; ++HitCount) {
		TileBehaviorResult Result =
			TileBehaviorSystem::Apply(Hit, Map, Loaded.Value(), Runtime);
		assert(Result.Handled);
		assert(Result.Effects.size() == 1);
		assert(Result.Effects[0].Type == TileEffectType::SpawnItem);
		assert(Result.Effects[0].Value == static_cast<int>(ItemKind::Coin));
		SpawnEffects.push_back(Result.Effects[0]);
		assert(Runtime.TryGet({0, 0})->Count == HitCount);
		if (HitCount < 10) assert(*Map.TryGet({0, 0}) == 37);
		else assert(*Map.TryGet({0, 0}) == 30);
	}

	// 使用済みへ置換した後は、古いイベントを再実行しても11枚目を出さない。
	TileBehaviorResult Eleventh =
		TileBehaviorSystem::Apply(Hit, Map, Loaded.Value(), Runtime);
	assert(!Eleventh.Handled);
	assert(Eleventh.Effects.empty());
	assert(Runtime.TryGet({0, 0})->Count == 10);

	ItemSystem Items;
	Items.ConsumeTileEffects(SpawnEffects);
	assert(Items.Items().size() == 10);

	int Coins = 0;
	int Score = 0;
	for (int Frame = 0; Frame < 30; ++Frame) {
		const std::vector<TileEffect> Rewards = Items.Update();
		for (std::size_t Index = 0; Index < Rewards.size(); ++Index) {
			if (Rewards[Index].Type == TileEffectType::AddCoin) Coins += Rewards[Index].Value;
			if (Rewards[Index].Type == TileEffectType::AddScore) Score += Rewards[Index].Value;
		}
	}
	assert(Items.Items().empty());
	assert(Coins == 10);
	assert(Score == 1000);
}

void TestOnOffDefinitionsAndWorldState() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Switch = Loaded.Value().Find(40);
	assert(Switch != nullptr);
	assert(Switch->Collision == CollisionShape::Solid);
	assert(Switch->Rules.size() == 1);
	assert(Switch->Rules[0].Action == TileAction::ToggleSwitch);
	assert(Switch->Rules[0].Value == 0);

	const TileDefinition* OnSolid = Loaded.Value().Find(41);
	const TileDefinition* OnEmpty = Loaded.Value().Find(42);
	const TileDefinition* OffEmpty = Loaded.Value().Find(43);
	const TileDefinition* OffSolid = Loaded.Value().Find(44);
	assert(OnSolid != nullptr && OnEmpty != nullptr &&
		OffEmpty != nullptr && OffSolid != nullptr);
	assert(OnSolid->SwitchChannel == 0);
	assert(OnSolid->SwitchOnTileId == 41);
	assert(OnSolid->SwitchOffTileId == 42);
	assert(OffEmpty->SwitchOnTileId == 43);
	assert(OffEmpty->SwitchOffTileId == 44);

	TileMap Map = MakeMap({{41, 43, 43}});
	WorldState World;
	World.Reset(1, true);
	WorldStateUpdate Initial = World.Synchronize(Map, Loaded.Value());
	assert(Initial.ChangedTiles.empty());
	assert(*Map.TryGet({0, 0}) == 41);
	assert(*Map.TryGet({1, 0}) == 43);
	assert(*Map.TryGet({2, 0}) == 43);

	TileEffect Toggle;
	Toggle.Type = TileEffectType::ToggleSwitch;
	Toggle.Value = 0;
	WorldStateUpdate Off = World.ApplyEffects({Toggle}, Map, Loaded.Value());
	assert(!World.GetSwitch(0));
	assert(*Map.TryGet({0, 0}) == 42);
	assert(*Map.TryGet({1, 0}) == 44);
	assert(*Map.TryGet({2, 0}) == 44);
	assert(Off.ChangedTiles.size() == 3);
	assert(Off.ActivatedSolidTiles.size() == 2);

	WorldStateUpdate On = World.ApplyEffects({Toggle}, Map, Loaded.Value());
	assert(World.GetSwitch(0));
	assert(*Map.TryGet({0, 0}) == 41);
	assert(*Map.TryGet({1, 0}) == 43);
	assert(*Map.TryGet({2, 0}) == 43);
	assert(On.ActivatedSolidTiles.size() == 1);
	assert(On.ActivatedSolidTiles[0].Column == 0);
}

void TestCoinConditionalDefinitions() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* ZeroPass = Loaded.Value().Find(49);
	const TileDefinition* FiftyPass = Loaded.Value().Find(51);
	const TileDefinition* UnderFiftyPass = Loaded.Value().Find(53);
	assert(ZeroPass != nullptr && FiftyPass != nullptr && UnderFiftyPass != nullptr);

	assert(ZeroPass->ConditionField == GameStateField::Coins);
	assert(ZeroPass->ConditionOperator == ComparisonOperator::Equal);
	assert(ZeroPass->ConditionThreshold == 0);
	assert(ZeroPass->ConditionTrueTileId == 49);
	assert(ZeroPass->ConditionFalseTileId == 50);

	assert(FiftyPass->ConditionField == GameStateField::Coins);
	assert(FiftyPass->ConditionOperator == ComparisonOperator::GreaterEqual);
	assert(FiftyPass->ConditionThreshold == 50);
	assert(FiftyPass->ConditionTrueTileId == 51);
	assert(FiftyPass->ConditionFalseTileId == 52);

	assert(UnderFiftyPass->ConditionField == GameStateField::Coins);
	assert(UnderFiftyPass->ConditionOperator == ComparisonOperator::LessThan);
	assert(UnderFiftyPass->ConditionThreshold == 50);
	assert(UnderFiftyPass->ConditionTrueTileId == 53);
	assert(UnderFiftyPass->ConditionFalseTileId == 54);
}

void TestCoinConditionalBlocksMatchVersion1() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({{49, 51, 53}});
	GameStateSnapshot State;

	// V1:
	// ZeroCoinBlock   : 0枚なら通れる
	// O50CoinBlock    : 50枚以上なら通れる
	// U50CoinBlock    : 50枚未満なら通れる
	State.Coins = 0;
	ConditionalTerrainUpdate AtZero =
		ConditionalTerrain::Synchronize(Map, Loaded.Value(), State);
	assert(*Map.TryGet({0, 0}) == 49);
	assert(*Map.TryGet({1, 0}) == 52);
	assert(*Map.TryGet({2, 0}) == 53);
	assert(AtZero.ActivatedSolidTiles.size() == 1);
	assert(AtZero.ActivatedSolidTiles[0].Column == 1);

	State.Coins = 49;
	ConditionalTerrainUpdate AtFortyNine =
		ConditionalTerrain::Synchronize(Map, Loaded.Value(), State);
	assert(*Map.TryGet({0, 0}) == 50);
	assert(*Map.TryGet({1, 0}) == 52);
	assert(*Map.TryGet({2, 0}) == 53);
	assert(AtFortyNine.ActivatedSolidTiles.size() == 1);
	assert(AtFortyNine.ActivatedSolidTiles[0].Column == 0);

	State.Coins = 50;
	ConditionalTerrainUpdate AtFifty =
		ConditionalTerrain::Synchronize(Map, Loaded.Value(), State);
	assert(*Map.TryGet({0, 0}) == 50);
	assert(*Map.TryGet({1, 0}) == 51);
	assert(*Map.TryGet({2, 0}) == 54);
	assert(AtFifty.ActivatedSolidTiles.size() == 1);
	assert(AtFifty.ActivatedSolidTiles[0].Column == 2);

	// 50→0へ戻した時も元のV1条件へ復帰する。
	State.Coins = 0;
	ConditionalTerrainUpdate BackToZero =
		ConditionalTerrain::Synchronize(Map, Loaded.Value(), State);
	assert(*Map.TryGet({0, 0}) == 49);
	assert(*Map.TryGet({1, 0}) == 52);
	assert(*Map.TryGet({2, 0}) == 53);
	assert(BackToZero.ActivatedSolidTiles.size() == 1);
	assert(BackToZero.ActivatedSolidTiles[0].Column == 1);
}

void TestConditionalTerrainComparisonOperators() {
	assert(ConditionalTerrain::Compare(5, ComparisonOperator::Equal, 5));
	assert(ConditionalTerrain::Compare(5, ComparisonOperator::NotEqual, 4));
	assert(ConditionalTerrain::Compare(4, ComparisonOperator::LessThan, 5));
	assert(ConditionalTerrain::Compare(5, ComparisonOperator::LessEqual, 5));
	assert(ConditionalTerrain::Compare(5, ComparisonOperator::GreaterEqual, 5));
	assert(ConditionalTerrain::Compare(6, ComparisonOperator::GreaterThan, 5));

	GameStateSnapshot State;
	State.Coins = 10;
	State.Health = 3;
	State.Lives = 7;
	State.Score = 1234;
	assert(ConditionalTerrain::ReadValue(GameStateField::Coins, State) == 10);
	assert(ConditionalTerrain::ReadValue(GameStateField::Health, State) == 3);
	assert(ConditionalTerrain::ReadValue(GameStateField::Lives, State) == 7);
	assert(ConditionalTerrain::ReadValue(GameStateField::Score, State) == 1234);
}

void TestTimedDisappearingBlocksToggleEvery80Frames() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* PhaseASolid = Loaded.Value().Find(45);
	const TileDefinition* PhaseAEmpty = Loaded.Value().Find(46);
	const TileDefinition* PhaseBEmpty = Loaded.Value().Find(47);
	const TileDefinition* PhaseBSolid = Loaded.Value().Find(48);
	assert(PhaseASolid != nullptr && PhaseAEmpty != nullptr &&
		PhaseBEmpty != nullptr && PhaseBSolid != nullptr);
	assert(PhaseASolid->SwitchChannel == 1);
	assert(PhaseASolid->AutoTogglePeriod == 80);
	assert(PhaseBEmpty->SwitchChannel == 1);
	assert(PhaseBEmpty->AutoTogglePeriod == 80);

	TileMap Map = MakeMap({{45, 47}});
	WorldState World;
	World.Reset(2, true);
	World.Synchronize(Map, Loaded.Value());

	// V1 の HiddenTime と同じく、79フレームまでは初期相を保つ。
	for (int Frame = 1; Frame <= 79; ++Frame) {
		WorldStateUpdate Update = World.AdvanceFrame(Map, Loaded.Value());
		assert(Update.ChangedTiles.empty());
		assert(*Map.TryGet({0, 0}) == 45);
		assert(*Map.TryGet({1, 0}) == 47);
		assert(World.GetSwitch(1));
		assert(World.GetSwitch(0));
	}
	assert(World.GetAutoToggleCounter(1) == 79);

	// 80フレーム目で反転。47(None) -> 48(Solid) が安全判定対象になる。
	WorldStateUpdate Eightieth = World.AdvanceFrame(Map, Loaded.Value());
	assert(!World.GetSwitch(1));
	assert(World.GetSwitch(0));
	assert(World.GetAutoToggleCounter(1) == 0);
	assert(*Map.TryGet({0, 0}) == 46);
	assert(*Map.TryGet({1, 0}) == 48);
	assert(Eightieth.ChangedTiles.size() == 2);
	assert(Eightieth.ActivatedSolidTiles.size() == 1);
	assert(Eightieth.ActivatedSolidTiles[0].Column == 1);

	for (int Frame = 1; Frame <= 80; ++Frame) {
		World.AdvanceFrame(Map, Loaded.Value());
	}
	assert(World.GetSwitch(1));
	assert(*Map.TryGet({0, 0}) == 45);
	assert(*Map.TryGet({1, 0}) == 47);
}

void TestOnOffSwitchRuleProducesToggleEffect() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({{40}});
	TileRuntimeMap Runtime(Map);
	TileInteraction Hit;
	Hit.Trigger = TileTrigger::HitFromBelow;
	Hit.Position = {0, 0};
	Hit.TileId = 40;

	TileBehaviorResult Result =
		TileBehaviorSystem::Apply(Hit, Map, Loaded.Value(), Runtime);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::ToggleSwitch);
	assert(Result.Effects[0].Value == 0);
	// repeat なので連続した別Hitでも切替Effectを出せる。
	assert(TileBehaviorSystem::Apply(Hit, Map, Loaded.Value(), Runtime).Effects.size() == 1);
}

void TestActivatedOnOffBlockPushesCharacterToSafety() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 44, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = true;

	CharacterController Controller(Body);
	CharacterSafetyResult Safety =
		CharacterSafety::ResolveActivatedSolids(
			Controller, Map, Loaded.Value(), {{1, 1}});
	Body = Controller.Body();
	assert(!Safety.Crushed);
	assert(Safety.Repositioned);
	assert(Safety.Effects.empty());
	// 上は空いているため、最短の押し出し先の一つへ脱出できる。
	assert(!NearlyEqual(Body.Position.X, 32.0f) ||
		!NearlyEqual(Body.Position.Y, 32.0f));
}

void TestActivatedOnOffBlocksKillWhenCharacterIsCrushed() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	// 中央のキャラクターを、同時出現した左右ブロック・上のスイッチ・下の床で囲む。
	TileMap Map = MakeMap({
		{0, 40, 0},
		{44, 44, 44},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = true;

	const std::vector<TilePosition> Activated = {
		{0, 1}, {1, 1}, {2, 1}
	};
	CharacterController Controller(Body);
	CharacterSafetyResult Safety =
		CharacterSafety::ResolveActivatedSolids(
			Controller, Map, Loaded.Value(), Activated);
	Body = Controller.Body();
	assert(Safety.Crushed);
	assert(!Safety.Repositioned);
	assert(Safety.Effects.size() == 1);
	assert(Safety.Effects[0].Type == TileEffectType::InstantDeath);
}

void TestItemSystemConvertsSpawnToV1Rewards() {
	ItemSystem Items;
	const ItemKind Kinds[] = {
		ItemKind::Coin,
		ItemKind::Healing,
		ItemKind::OneUp
	};

	for (int KindIndex = 0; KindIndex < 3; ++KindIndex) {
		Items.Reset();
		TileEffect Spawn;
		Spawn.Type = TileEffectType::SpawnItem;
		Spawn.Position = {2, 3};
		Spawn.Value = static_cast<int>(Kinds[KindIndex]);
		Items.ConsumeTileEffects({Spawn}, 32, 32);
		assert(Items.Items().size() == 1);
		assert(Items.Items()[0].Kind == Kinds[KindIndex]);
		assert(NearlyEqual(Items.Items()[0].Position.X, 64.0f));
		assert(NearlyEqual(Items.Items()[0].Position.Y, 64.0f));

		std::vector<TileEffect> Rewards;
		for (int Frame = 0; Frame < 30; ++Frame) {
			std::vector<TileEffect> Current = Items.Update();
			Rewards.insert(Rewards.end(), Current.begin(), Current.end());
		}
		assert(Items.Items().empty());

		if (Kinds[KindIndex] == ItemKind::Coin) {
			assert(Rewards.size() == 2);
			assert(Rewards[0].Type == TileEffectType::AddCoin);
			assert(Rewards[0].Value == 1);
			assert(Rewards[1].Type == TileEffectType::AddScore);
			assert(Rewards[1].Value == 100);
		} else if (Kinds[KindIndex] == ItemKind::Healing) {
			assert(Rewards.size() == 2);
			assert(Rewards[0].Type == TileEffectType::AddHealth);
			assert(Rewards[0].Value == 1);
			assert(Rewards[1].Type == TileEffectType::AddScore);
			assert(Rewards[1].Value == 1000);
		} else {
			assert(Rewards.size() == 1);
			assert(Rewards[0].Type == TileEffectType::AddLife);
			assert(Rewards[0].Value == 1);
		}
	}
}

void TestQuestionBlockSpawnsItemAndBecomesUsed() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 31, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(0.0f, true, Map, Loaded.Value());

	bool FoundHit = false;
	for (std::size_t Index = 0; Index < Player.Interactions().size(); ++Index) {
		const TileInteraction& Interaction = Player.Interactions()[Index];
		if (Interaction.Trigger == TileTrigger::HitFromBelow &&
			Interaction.Position.Column == 1 && Interaction.Position.Row == 1 &&
			Interaction.TileId == 31) {
			FoundHit = true;
		}
	}
	assert(FoundHit);

	TileRuntimeMap Runtime(Map);
	const std::vector<TileEffect> Effects =
		TileBehaviorSystem::ApplyAll(Player.Interactions(), Map, Loaded.Value(), Runtime);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::SpawnItem);
	assert(Effects[0].Value == static_cast<int>(ItemKind::Coin));
	assert(*Map.TryGet({1, 1}) == 30);

	ItemSystem Items;
	Items.ConsumeTileEffects(Effects);
	assert(Items.Items().size() == 1);
}

void TestHiddenItemBlockOnlyBlocksFromBelow() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	// 横からは存在しないものとして通過できる。
	TileMap SideMap = MakeMap({
		{0, 0, 0},
		{0, 34, 0},
		{1, 1, 1}
	});
	CharacterBody SideBody;
	SideBody.Position = {0.0f, 32.0f};
	SideBody.Grounded = true;
	CharacterController SidePlayer(SideBody);
	for (int Frame = 0; Frame < 15; ++Frame) {
		SidePlayer.Step(1.0f, false, SideMap, Loaded.Value());
	}
	assert(SidePlayer.Body().Position.X > 32.0f);

	// 上から落ちても足場にならない。
	TileMap FallMap = MakeMap({
		{0, 0, 0},
		{0, 34, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	CharacterBody FallBody;
	FallBody.Position = {32.0f, -16.0f};
	FallBody.Velocity.Y = 4.0f;
	FallBody.Grounded = false;
	CharacterMotion FallMotion;
	FallMotion.MoveSpeed = 0.0f;
	FallMotion.Gravity = 0.0f;
	CharacterController FallPlayer(FallBody, FallMotion);
	for (int Frame = 0; Frame < 14; ++Frame) {
		FallPlayer.Step(0.0f, false, FallMap, Loaded.Value());
	}
	assert(FallPlayer.Body().Position.Y > 32.0f);
	assert(!FallPlayer.Body().Grounded);

	// 下からだけ頭を止め、HitFromBelowを通知する。
	TileMap HitMap = MakeMap({
		{0, 0, 0},
		{0, 34, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	CharacterBody HitBody;
	HitBody.Position = {32.0f, 64.0f};
	HitBody.Grounded = true;
	CharacterController HitPlayer(HitBody);
	HitPlayer.Step(0.0f, true, HitMap, Loaded.Value());
	assert(NearlyEqual(HitPlayer.Body().Position.Y, 64.0f));
	assert(!HitPlayer.Body().Grounded);

	bool FoundHiddenHit = false;
	for (std::size_t Index = 0; Index < HitPlayer.Interactions().size(); ++Index) {
		const TileInteraction& Interaction = HitPlayer.Interactions()[Index];
		if (Interaction.Trigger == TileTrigger::HitFromBelow &&
			Interaction.Position.Column == 1 && Interaction.Position.Row == 1 &&
			Interaction.TileId == 34) {
			FoundHiddenHit = true;
		}
	}
	assert(FoundHiddenHit);

	TileRuntimeMap Runtime(HitMap);
	const std::vector<TileEffect> Effects =
		TileBehaviorSystem::ApplyAll(
			HitPlayer.Interactions(), HitMap, Loaded.Value(), Runtime);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::SpawnItem);
	assert(*HitMap.TryGet({1, 1}) == 30);
}

void TestCharacterEmitsTouchForCollectible() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileDefinition Coin;
	Coin.Id = 20;
	Coin.Collision = CollisionShape::None;
	Coin.Rules.push_back({TileTrigger::Touch, TileAction::AddCoin, 1, true});
	Coin.Rules.push_back({TileTrigger::Touch, TileAction::ReplaceTile, 0, true});
	assert(Catalog.Register(Coin).IsSuccess());

	TileMap Map = MakeMap({
		{0, 20, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(1.0f, false, Map, Catalog);

	bool FoundTouch = false;
	for (std::size_t Index = 0; Index < Player.Interactions().size(); ++Index) {
		const TileInteraction& Interaction = Player.Interactions()[Index];
		if (Interaction.Trigger == TileTrigger::Touch &&
			Interaction.Position.Column == 1 && Interaction.Position.Row == 0 &&
			Interaction.TileId == 20) {
			FoundTouch = true;
		}
	}
	assert(FoundTouch);

	TileRuntimeMap Runtime(Map);
	std::vector<TileEffect> Effects =
		TileBehaviorSystem::ApplyAll(Player.Interactions(), Map, Catalog, Runtime);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::AddCoin);
	assert(*Map.TryGet({1, 0}) == 0);
}

void TestCharacterTouchIncludesSolidContact() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0},
		{22}
	});
	CharacterBody Body;
	Body.Position = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(0.0f, false, Map, Loaded.Value());

	TileRuntimeMap Runtime(Map);
	std::vector<TileEffect> Effects =
		TileBehaviorSystem::ApplyAll(Player.Interactions(), Map, Loaded.Value(), Runtime);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::Damage);
	assert(Effects[0].Value == 1);
}

void TestCharacterEmitsHitFromBelowForBlock() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileDefinition Breakable;
	Breakable.Id = 21;
	Breakable.Collision = CollisionShape::Solid;
	Breakable.Breakable = true;
	assert(Catalog.Register(Breakable).IsSuccess());

	TileMap Map = MakeMap({
		{0, 21, 0},
		{0, 0, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(0.0f, true, Map, Catalog);

	bool FoundHit = false;
	for (std::size_t Index = 0; Index < Player.Interactions().size(); ++Index) {
		const TileInteraction& Interaction = Player.Interactions()[Index];
		if (Interaction.Trigger == TileTrigger::HitFromBelow &&
			Interaction.Position.Column == 1 && Interaction.Position.Row == 0 &&
			Interaction.TileId == 21) {
			FoundHit = true;
		}
	}
	assert(FoundHit);

	TileRuntimeMap Runtime(Map);
	std::vector<TileEffect> Effects =
		TileBehaviorSystem::ApplyAll(Player.Interactions(), Map, Catalog, Runtime);
	assert(*Map.TryGet({1, 0}) == 0);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::TileBroken);
}

void TestCanvasMasaoTerrainCodesAndCoordinates() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({{1, 2, 3, 12}});
	assert(CanvasMasaoTerrain::CodeAt(Map, Catalog, 1, 1) == 20);
	assert(CanvasMasaoTerrain::CodeAt(Map, Catalog, 33, 1) == 18);
	assert(CanvasMasaoTerrain::CodeAt(Map, Catalog, 65, 1) == 19);
	assert(CanvasMasaoTerrain::CodeAt(Map, Catalog, 97, 1) == 15);
	assert(CanvasMasaoTerrain::RoundDown(2.9) == 2);
	assert(CanvasMasaoTerrain::RoundDown(-2.9) == -2);
	for (int LocalX = 0; LocalX < 32; ++LocalX) {
		assert(CanvasMasaoTerrain::GetSakamichiY(Map, Catalog, 32 + LocalX, 31) == -LocalX);
		assert(CanvasMasaoTerrain::GetSakamichiY(Map, Catalog, 64 + LocalX, 31) == LocalX - 31);
	}
	int X = -15;
	assert(CanvasMasaoTerrain::ResolveHorizontalSolid(Map, Catalog, X, 0, true));
	assert(X == -16);
	int Y = -1;
	assert(CanvasMasaoTerrain::ResolveVerticalSolid(Map, Catalog, 0, Y, true));
	assert(Y == -32);
}

void TestCanvasMasaoVerticalCrossings() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 2, 0},
		{0, 12, 0},
		{1, 1, 1}
	});
	int RisingY = 31;
	assert(CanvasMasaoTerrain::ResolveRisingSlope(Map, Catalog, 32, 32, RisingY));
	assert(RisingY == 32);
	int SameRowY = 20;
	assert(!CanvasMasaoTerrain::ResolveRisingSlope(Map, Catalog, 32, 21, SameRowY));
	int OneWayY = 1;
	assert(CanvasMasaoTerrain::ResolveFallingOneWay(Map, Catalog, 32, 0, OneWayY));
	assert(OneWayY == 0);
}

void TestExtended2x1SlopeIsOneContinuousSurface() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap UpRight = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1}
	});
	ExtendedSlopeTerrain::Slope2x1 Slope;
	assert(ExtendedSlopeTerrain::TryFind2x1(UpRight, Catalog, 40, 40, Slope));
	assert(Slope.LeftColumn == 1 && Slope.Row == 1 && Slope.UpRight);
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 32.0f), 64.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 64.0f), 48.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 95.0f), 33.0f));
	float LeftY = 0.0f;
	float RightY = 0.0f;
	assert(ExtendedSlopeTerrain::TryCharacterY(UpRight, Catalog, 63.999f, 48.0f, LeftY));
	assert(ExtendedSlopeTerrain::TryCharacterY(UpRight, Catalog, 64.001f, 48.0f, RightY));
	assert(std::fabs(LeftY - RightY) <= 1.0f);

	TileMap UpLeft = MakeMap({
		{0, 0, 0, 0},
		{0, 6, 7, 0},
		{1, 1, 1, 1}
	});
	assert(ExtendedSlopeTerrain::TryFind2x1(UpLeft, Catalog, 40, 40, Slope));
	assert(Slope.LeftColumn == 1 && Slope.Row == 1 && !Slope.UpRight);
	// 1x1左上がりの -31 をそのまま横2倍へ一般化する。
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 32.0f), 33.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 64.0f), 49.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 95.0f), 64.0f));

	// 片方だけ配置された不完全な坂は、64x32坂として認識しない。
	TileMap Broken = MakeMap({{0, 0, 0}, {0, 4, 0}, {1, 1, 1}});
	assert(!ExtendedSlopeTerrain::TryFind2x1(Broken, Catalog, 40, 40, Slope));
}

void TestExtended2x1SlopeEdgeVelocityFollowsGradient() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Floating = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{0, 0, 0, 0}
	});

	float NewY = 2.0f;
	int VelocityY10 = 0;
	bool Grounded = true;
	assert(ExtendedSlopeTerrain::FollowHorizontal(
		Floating, Catalog,
		78.0f, 81.0f, 2.0f, NewY,
		30, VelocityY10, true, Grounded));
	assert(NearlyEqual(NewY, 0.0f));
	assert(!Grounded);
	assert(VelocityY10 == -15);

	// 64px進んで32px下るため、低い端では横速度の1/2で落下を開始する。
	NewY = 32.0f;
	VelocityY10 = 0;
	Grounded = true;
	assert(ExtendedSlopeTerrain::FollowHorizontal(
		Floating, Catalog,
		18.0f, 15.0f, 32.0f, NewY,
		-30, VelocityY10, true, Grounded));
	assert(NearlyEqual(NewY, 32.0f));
	assert(!Grounded);
	assert(VelocityY10 == 15);
}

void TestExtended2x1SlopeHighSideAndLanding() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1}
	});
	float X = 78.0f;
	assert(ExtendedSlopeTerrain::ResolveHighSide(
		Map, Catalog, 85.0f, X, 32.0f, false, false));
	assert(NearlyEqual(X, 81.0f));
	float FallingY = 20.0f;
	assert(ExtendedSlopeTerrain::ResolveFalling(Map, Catalog, 45.0f, 15.0f, FallingY));
	assert(NearlyEqual(FallingY, 18.0f));
}

void TestCharacterCanJumpAcrossConnected2x1Peak() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 4, 5, 6, 7, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// x+15=93。右上がり坂の頂上直前から、接続された左上がり坂へ飛び越す。
	Body.Position = {78.0f, 1.5f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(1.0f, true, Map, Catalog);
	assert(Player.Body().Position.X > 78.0f);
	assert(Player.Body().Position.Y < 0.0f);
	const float AfterJumpX = Player.Body().Position.X;
	Player.Step(1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X > AfterJumpX);
}

void TestCharacterDescendsConnected2x1PeakWithoutFloorWarp() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 4, 5, 6, 7, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {78.0f, 1.5f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 12; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		// 右側の下り坂を1フレームで追従し、下段床へ瞬間移動しない。
		assert(Player.Body().Position.Y - PreviousY <= 2.0f);
		assert(Player.Body().Position.Y < 20.0f);
		PreviousY = Player.Body().Position.Y;
	}
}

void TestCharacterCrossesConnected2x1PeakToLeftWithoutFallingThrough() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 4, 5, 6, 7, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 右側の左上がり坂から山頂を越え、左側の右上がり坂を左へ下る。
	Body.Position = {84.0f, 1.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 20; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		assert(std::fabs(Player.Body().Position.Y - PreviousY) <= 2.0f);
		PreviousY = Player.Body().Position.Y;
	}
}

void TestCharacterCrossesConnected2x1ValleyToLeftWithoutFallingThrough() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 6, 7, 4, 5, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 右側の右上がり坂を左へ下り、2x1坂同士の谷間を越える。
	Body.Position = {142.0f, 2.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 20; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		assert(std::fabs(Player.Body().Position.Y - PreviousY) <= 2.0f);
		PreviousY = Player.Body().Position.Y;
	}
}

void TestCharacterDescendsStacked2x1BoundaryToLeft() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 4, 5, 0},
		{0, 4, 5, 0, 0, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 1行上の右上がり2x1坂から、左下の右上がり2x1坂へ連続して下る。
	Body.Position = {84.0f, 31.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 20; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		assert(Player.Body().Position.Y >= PreviousY - 0.01f);
		assert(Player.Body().Position.Y - PreviousY <= 2.0f);
		PreviousY = Player.Body().Position.Y;
	}
}

void TestCharacterDescends2x1SlopeToLeftWithoutFallingThrough() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	// 右上がり2x1坂の高い側から、左入力を続けて坂を下る。
	Body.Position = {78.0f, 1.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 18; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		float ExpectedY = 0.0f;
		assert(ExtendedSlopeTerrain::TryCharacterY(
			Map, Catalog, Player.Body().Position.X + 15.0f,
			Player.Body().Position.Y + 31.0f, ExpectedY));
		assert(NearlyEqual(Player.Body().Position.Y, ExpectedY));
	}
}

void TestCharacterEnters2x1HighEdgeAndDescendsToLeft() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 4, 5, 1, 0},
		{1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 高い端に接続した平地から左へ入り、そのまま坂を下り切る。
	Body.Position = {81.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 30; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		assert(Player.Body().Position.Y >= PreviousY - 0.01f);
		assert(Player.Body().Position.Y - PreviousY <= 2.0f);
		PreviousY = Player.Body().Position.Y;
	}
}

void TestCharacterDescends2x1ToLeftFromEverySurfacePixel() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1}
	});
	ExtendedSlopeTerrain::Slope2x1 Slope;
	assert(ExtendedSlopeTerrain::TryFind2x1(Map, Catalog, 40, 40, Slope));
	for (int CenterX = 34; CenterX <= 94; ++CenterX) {
		CharacterBody Body;
		Body.Position = {
			static_cast<float>(CenterX - 15),
			ExtendedSlopeTerrain::SurfaceY(Slope, static_cast<float>(CenterX)) - 32.0f};
		Body.Grounded = true;
		CharacterController Player(Body);
		for (int Frame = 0; Frame < 30; ++Frame) {
			Player.Step(-1.0f, false, Map, Catalog);
			assert(Player.Body().Grounded);
		}
	}
}

void TestCharacterDescends2x1ToLeftOnExternalStage() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/slope-test/stage.ini");
	assert(Loaded.IsSuccess());
	const TileMap& Map = Loaded.Value().Map;
	const TileCatalog& Catalog = Loaded.Value().Catalog;
	// 外部テストステージの右上がり2x1坂（列11～12）を全位置から左へ下る。
	ExtendedSlopeTerrain::Slope2x1 Slope;
	assert(ExtendedSlopeTerrain::TryFind2x1(Map, Catalog, 11 * 32, 10 * 32, Slope));
	for (int CenterX = 11 * 32 + 2; CenterX <= 13 * 32 - 2; ++CenterX) {
		CharacterBody Body;
		Body.Position = {
			static_cast<float>(CenterX - 15),
			ExtendedSlopeTerrain::SurfaceY(Slope, static_cast<float>(CenterX)) - 32.0f};
		Body.Grounded = true;
		CharacterController Player(Body);
		for (int Frame = 0; Frame < 4; ++Frame) {
			Player.Step(-1.0f, false, Map, Catalog);
			assert(Player.Body().Grounded);
		}
	}
	// 画像で隣に見える反対向き2x1坂（列14～15）も、左入力で接地を失わない。
	assert(ExtendedSlopeTerrain::TryFind2x1(Map, Catalog, 14 * 32, 10 * 32, Slope));
	for (int CenterX = 14 * 32 + 2; CenterX <= 16 * 32 - 2; ++CenterX) {
		CharacterBody Body;
		Body.Position = {
			static_cast<float>(CenterX - 15),
			ExtendedSlopeTerrain::SurfaceY(Slope, static_cast<float>(CenterX)) - 32.0f};
		Body.Grounded = true;
		CharacterController Player(Body);
		for (int Frame = 0; Frame < 4; ++Frame) {
			Player.Step(-1.0f, false, Map, Catalog);
			assert(Player.Body().Grounded);
		}
	}
}

void TestCharacterLeavesFloating2x1LowEdgeNaturally() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{0, 0, 0, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {78.0f, 1.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	bool LeftSlope = false;
	float PreviousY = Player.Body().Position.Y;
	for (int Frame = 0; Frame < 40; ++Frame) {
		Player.Step(-1.0f, false, Map, Catalog);
		const float CenterX = Player.Body().Position.X + 15.0f;
		if (CenterX >= 32.0f) {
			assert(Player.Body().Grounded);
			assert(Player.Body().Position.Y - PreviousY <= 2.0f);
		} else {
			LeftSlope = true;
			assert(!Player.Body().Grounded);
			// 低い端の高さから通常落下し、下の床へ瞬間移動しない。
			assert(Player.Body().Position.Y < 40.0f);
			break;
		}
		PreviousY = Player.Body().Position.Y;
	}
	assert(LeftSlope);
}

void TestCharacterLeaves2x1HighEdgeWithoutWarpingToLowerFloor() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {78.0f, 1.5f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X > 78.0f);
	assert(!Player.Body().Grounded);
	// 高い端のY=0付近から落下を開始し、下段床のY=32へ飛ばない。
	assert(Player.Body().Position.Y < 5.0f);
	const float PeakY = Player.Body().Position.Y;
	Player.Step(1.0f, false, Map, Catalog);
	assert(Player.Body().Position.Y >= PeakY);
	assert(Player.Body().Position.Y < 5.0f);
}

void TestRisingCharacterCannotPass2x1HighSideInsideColumn() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	// 坂より低い位置で右側から列へ入り、その後同じ列内で上昇して頭が側面へ重なる。
	Body.Position = {84.0f, 64.0f};
	Body.Velocity.Y = -10.0f;
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);
	Player.Step(-1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X < 84.0f);
	Player.Step(-1.0f, false, Map, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, 81.0f));
	assert(NearlyEqual(Player.Body().Velocity.X, 0.0f));
}

void TestCharacterJumpsLeftAlong2x1HighSideAndLands() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{0, 0, 0, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	// 2x1坂の右側面に接した床上から、左入力を続けながらジャンプする。
	Body.Position = {84.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	bool RoseAboveSlope = false;
	bool Landed = false;
	for (int Frame = 0; Frame < 80; ++Frame) {
		Player.Step(-1.0f, Frame == 0, Map, Catalog);
		assert(Player.Body().Position.Y <= 64.0f);
		if (Player.Body().Position.Y < 0.0f) RoseAboveSlope = true;
		if (Frame > 0 && Player.Body().Grounded) {
			Landed = true;
			break;
		}
	}
	assert(RoseAboveSlope);
	assert(Landed);
}

void TestCharacterJumpsLeftAlong2x1HighSideConnectedToBlock() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 4, 5, 1, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 高い端にブロックが接続された坂の右下から、左入力で側面をこすって上昇する。
	Body.Position = {84.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	bool ClearedTerrain = false;
	bool RoseAfterClearing = false;
	for (int Frame = 0; Frame < 40; ++Frame) {
		Player.Step(-1.0f, Frame == 0, Map, Catalog);
		const float Center = Player.Body().Position.X + 15.0f;
		if (Center >= 32.0f && Center < 128.0f) {
			// 坂とブロックの下面にいる間は、上の床へ抜けない。
			assert(Player.Body().Position.Y >= 64.0f);
		} else if (Center < 32.0f) {
			ClearedTerrain = true;
			if (Player.Body().Position.Y < 64.0f) RoseAfterClearing = true;
		}
		assert(Player.Body().Position.Y <= 64.0f);
	}
	assert(ClearedTerrain);
	assert(RoseAfterClearing);
}

void TestCharacterCannotRiseThroughStacked2x1RightEdge() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 4, 5, 0},
		{0, 4, 5, 0, 0, 0},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 下側2x1坂の高い右端を左へこすりながらジャンプする。
	// 横方向には上側坂と連続しているが、下側坂の下面は通過できない。
	Body.Position = {84.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 8; ++Frame) {
		Player.Step(-1.0f, Frame == 0, Map, Catalog);
		assert(Player.Body().Position.Y >= 64.0f);
	}
}

void TestCharacterJumpArcUnderLongStacked2x1Slope() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {496.0f, 320.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	for (int Frame = 0; Frame < 80; ++Frame) {
		Player.Step(-1.0f, Frame == 0, Map, Catalog);
		const float CenterX = Player.Body().Position.X + 15.0f;
		ExtendedSlopeTerrain::Slope2x1 Slope;
		for (int Row = 7; Row <= 10; ++Row) {
			if (!ExtendedSlopeTerrain::TryFind2x1(
				Map, Catalog, static_cast<int>(CenterX), Row * 32 + 16, Slope)) continue;
			const float SurfaceY = ExtendedSlopeTerrain::SurfaceY(Slope, CenterX);
			const float Top = Player.Body().Position.Y;
			const float Foot = Top + 31.0f;
			const float Bottom = static_cast<float>((Row + 1) * 32);
			assert(Top >= Bottom || Foot <= SurfaceY + 0.01f);
		}
	}
}

void TestJumpingCharacterCanMoveAbove2x1SurfaceInsideColumn() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 4, 5, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	// 2x1坂の高い側半分に接地した状態から、左入力を続けてジャンプする。
	Body.Position = {60.0f, 10.5f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(-1.0f, true, Map, Catalog);
	const float JumpX = Player.Body().Position.X;
	assert(JumpX < 60.0f);
	assert(Player.Body().Position.Y < 10.5f);
	Player.Step(-1.0f, false, Map, Catalog);
	// 同じタイル列内の再判定で坂側面へ戻されず、上の空間を移動できる。
	assert(Player.Body().Position.X < JumpX);
}

void TestCharacterCanJumpFromBlockInto2x1UpperSpace() {
	TileCatalog Catalog = MakeTerrainCatalog();
	// 実テストステージと同じ「右上がり2x1坂・ブロック・左上がり2x1坂」。
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 4, 5, 1, 6, 7},
		{1, 1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 中央ブロックの左端から、左入力を続けて坂上の空間へジャンプする。
	Body.Position = {81.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	Player.Step(-1.0f, true, Map, Catalog);
	assert(Player.Body().Position.X < 81.0f);
	assert(Player.Body().Position.Y < 0.0f);
	const float FirstX = Player.Body().Position.X;
	Player.Step(-1.0f, false, Map, Catalog);
	assert(Player.Body().Position.X < FirstX);
}

void TestExtended1x2SlopeIsOneContinuousSurface() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap UpRight = MakeMap({
		{0, 0, 0, 0},
		{0, 9, 0, 0},
		{0, 8, 0, 0},
		{1, 1, 1, 1}
	});
	ExtendedSlopeTerrain::Slope1x2 Slope;
	assert(ExtendedSlopeTerrain::TryFind1x2(UpRight, Catalog, 40, 40, Slope));
	assert(Slope.Column == 1 && Slope.TopRow == 1 && Slope.UpRight);
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 32.0f), 96.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 48.0f), 64.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 63.0f), 34.0f));

	// y=64 の内部タイル境界をまたいでも、1本の32x64坂として連続する。
	float LowerY = 0.0f;
	float UpperY = 0.0f;
	assert(ExtendedSlopeTerrain::TryCharacterY1x2(
		UpRight, Catalog, 47.0f, 65.0f, LowerY));
	assert(ExtendedSlopeTerrain::TryCharacterY1x2(
		UpRight, Catalog, 48.0f, 63.0f, UpperY));
	assert(NearlyEqual(LowerY - UpperY, 2.0f));

	TileMap UpLeft = MakeMap({
		{0, 0, 0, 0},
		{0, 10, 0, 0},
		{0, 11, 0, 0},
		{1, 1, 1, 1}
	});
	assert(ExtendedSlopeTerrain::TryFind1x2(UpLeft, Catalog, 40, 40, Slope));
	assert(Slope.Column == 1 && Slope.TopRow == 1 && !Slope.UpRight);
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 32.0f), 34.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 48.0f), 66.0f));
	assert(NearlyEqual(ExtendedSlopeTerrain::SurfaceY(Slope, 63.0f), 96.0f));

	TileMap Broken = MakeMap({
		{0, 0, 0},
		{0, 9, 0},
		{0, 0, 0}
	});
	assert(!ExtendedSlopeTerrain::TryFind1x2(Broken, Catalog, 40, 40, Slope));
}

void TestExtended1x2SlopeEdgeVelocityFollowsGradient() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Floating = MakeMap({
		{0, 0, 0, 0},
		{0, 9, 0, 0},
		{0, 8, 0, 0},
		{0, 0, 0, 0}
	});

	float NewY = 2.0f;
	int VelocityY10 = 0;
	bool Grounded = true;
	assert(ExtendedSlopeTerrain::FollowHorizontal1x2(
		Floating, Catalog,
		48.0f, 51.0f, 2.0f, NewY,
		30, VelocityY10, true, Grounded));
	assert(NearlyEqual(NewY, 0.0f));
	assert(!Grounded);
	assert(VelocityY10 == -60);

	NewY = 64.0f;
	VelocityY10 = 0;
	Grounded = true;
	assert(ExtendedSlopeTerrain::FollowHorizontal1x2(
		Floating, Catalog,
		17.0f, 14.0f, 64.0f, NewY,
		-30, VelocityY10, true, Grounded));
	assert(NearlyEqual(NewY, 64.0f));
	assert(!Grounded);
	assert(VelocityY10 == 60);
}

void TestExtended1x2SlopeIgnoresInternalVerticalBoundary() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 9, 0},
		{0, 8, 0},
		{0, 0, 0}
	});

	// 上下2タイルの境界 y=64 は論理坂の内部なので、天井衝突にしない。
	float NewY = 55.0f;
	assert(!ExtendedSlopeTerrain::ResolveRising1x2(
		Map, Catalog, 33.0f, 64.0f, NewY));
	assert(NearlyEqual(NewY, 55.0f));

	// 32x64坂そのものの下面 y=96 を下から跨いだ場合だけ止める。
	NewY = 87.0f;
	assert(ExtendedSlopeTerrain::ResolveRising1x2(
		Map, Catalog, 33.0f, 96.0f, NewY));
	assert(NearlyEqual(NewY, 96.0f));
}

void TestCharacterTraverses1x2SlopeWithoutSeamSnag() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 0, 9, 1, 0},
		{0, 0, 8, 0, 0},
		{1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {43.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	for (int Frame = 0; Frame < 18; ++Frame) {
		Player.Step(1.0f, false, Map, Catalog);
		assert(Player.Body().Grounded);
		const float CenterX = Player.Body().Position.X + 15.0f;
		float LogicalY = 0.0f;
		if (ExtendedSlopeTerrain::TryCharacterY1x2(
			Map, Catalog, CenterX, Player.Body().Position.Y + 31.0f, LogicalY)) {
			assert(NearlyEqual(Player.Body().Position.Y, LogicalY));
		}
	}
	assert(Player.Body().Position.X > 80.0f);
	assert(NearlyEqual(Player.Body().Position.Y, 0.0f));
}

void TestCharacterCannotEnter1x2HighSide() {
	TileCatalog Catalog = MakeTerrainCatalog();

	TileMap UpRight = MakeMap({
		{0, 0, 0, 0},
		{0, 9, 0, 0},
		{0, 8, 0, 0},
		{1, 1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {50.0f, 64.0f};
	Body.Grounded = true;
	CharacterController FromRight(Body);
	FromRight.Step(-1.0f, false, UpRight, Catalog);
	assert(NearlyEqual(FromRight.Body().Position.X, 49.0f));

	TileMap UpLeft = MakeMap({
		{0, 0, 0, 0},
		{0, 10, 0, 0},
		{0, 11, 0, 0},
		{1, 1, 1, 1}
	});
	Body.Position = {16.0f, 64.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController FromLeft(Body);
	FromLeft.Step(1.0f, false, UpLeft, Catalog);
	assert(NearlyEqual(FromLeft.Body().Position.X, 16.0f));
}

void TestCharacterLandsOn1x2Slope() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 9, 0},
		{0, 8, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	// 中央X=48では坂面Y=64、キャラクターY=32。
	Body.Position = {33.0f, 28.0f};
	Body.Velocity.Y = 5.0f;
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.MoveSpeed = 0.0f;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);
	Player.Step(0.0f, false, Map, Catalog);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
}

void TestCharacterRepositionResetsInternalVelocity() {
	TileMap Map = MakeMap({
		{0, 0},
		{0, 0}
	});
	TileCatalog Catalog = MakeTerrainCatalog();
	CharacterBody Body;
	Body.Position = {0.0f, 0.0f};
	Body.Velocity = {2.0f, -4.0f};
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.MoveSpeed = 0.0f;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);

	Player.Reposition({10.0f, 10.0f}, true);
	assert(NearlyEqual(Player.Body().Velocity.X, 0.0f));
	assert(NearlyEqual(Player.Body().Velocity.Y, 0.0f));
	Player.Step(0.0f, false, Map, Catalog);
	assert(NearlyEqual(Player.Body().Position.X, 10.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 10.0f));
}

void TestLadderDefinitions() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Ladder = Loaded.Value().Find(55);
	assert(Ladder != nullptr);
	assert(Ladder->Collision == CollisionShape::None);
	assert(Ladder->Movement == MovementRegion::Ladder);

	const TileDefinition* Maker = Loaded.Value().Find(56);
	assert(Maker != nullptr);
	assert(Maker->Collision == CollisionShape::Solid);
	assert(Maker->Rules.size() == 2);
	assert(Maker->Rules[0].Action == TileAction::SpawnItem);
	assert(Maker->Rules[0].Value == static_cast<int>(ItemKind::LadderBuilder));
	assert(Maker->Rules[1].Action == TileAction::ReplaceTile);
	assert(Maker->Rules[1].Value == 30);

	const TileDefinition* HiddenMaker = Loaded.Value().Find(57);
	assert(HiddenMaker != nullptr);
	assert(HiddenMaker->Collision == CollisionShape::HitFromBelowOnly);
	assert(HiddenMaker->Rules[0].Value ==
		static_cast<int>(ItemKind::LadderBuilder));
}

void TestCharacterClimbsLadderWithoutGravity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 55, 0},
		{0, 55, 0},
		{0, 55, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Up;
	Up.Vertical = -1.0f;
	Player.Step(Up, Map, Loaded.Value());
	assert(Player.Mode() == MovementMode::Climbing);
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 61.0f));
	assert(NearlyEqual(Player.Body().Velocity.Y, -3.0f));

	// 入力を離しても重力で落ちず、その場に留まる。
	const float HoldY = Player.Body().Position.Y;
	CharacterInput Idle;
	Player.Step(Idle, Map, Loaded.Value());
	assert(Player.Mode() == MovementMode::Climbing);
	assert(NearlyEqual(Player.Body().Position.Y, HoldY));
	assert(NearlyEqual(Player.Body().Velocity.Y, 0.0f));

	// V1のLadderActと同じく登攀中の横速度は2。
	CharacterInput Right;
	Right.Horizontal = 1.0f;
	const float OldX = Player.Body().Position.X;
	Player.Step(Right, Map, Loaded.Value());
	assert(Player.Mode() == MovementMode::Climbing);
	assert(NearlyEqual(Player.Body().Position.X, OldX + 2.0f));
}

void TestLadderEntryRulesMatchVersion1() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 55, 0},
		{0, 55, 0},
		{0, 55, 0},
		{1, 1, 1}
	});

	// 地上で下を押しても登攀モードには入らない。
	CharacterBody GroundBody;
	GroundBody.Position = {32.0f, 64.0f};
	GroundBody.Grounded = true;
	CharacterController GroundPlayer(GroundBody);
	CharacterInput Down;
	Down.Vertical = 1.0f;
	GroundPlayer.Step(Down, Map, Loaded.Value());
	assert(GroundPlayer.Mode() == MovementMode::Normal);
	assert(GroundPlayer.Body().Grounded);

	// 空中では下入力でもはしごを掴める。
	CharacterBody AirBody;
	AirBody.Position = {32.0f, 60.0f};
	AirBody.Grounded = false;
	CharacterController AirPlayer(AirBody);
	AirPlayer.Step(Down, Map, Loaded.Value());
	assert(AirPlayer.Mode() == MovementMode::Climbing);
	assert(NearlyEqual(AirPlayer.Body().Position.Y, 63.0f));

	// 横へ外れれば通常モードへ戻る。
	CharacterInput Right;
	Right.Horizontal = 1.0f;
	for (int Frame = 0; Frame < 8 &&
		AirPlayer.Mode() == MovementMode::Climbing; ++Frame) {
		AirPlayer.Step(Right, Map, Loaded.Value());
	}
	assert(AirPlayer.Mode() == MovementMode::Normal);
}

void TestLadderBuilderCreatesTilesUntilSolidCeiling() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{1, 1, 1},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{1, 1, 1}
	});

	ItemSystem Items;
	TileEffect Spawn;
	Spawn.Type = TileEffectType::SpawnItem;
	Spawn.Position = {1, 4};
	Spawn.Value = static_cast<int>(ItemKind::LadderBuilder);
	Items.ConsumeTileEffects({Spawn}, 32, 32);
	assert(Items.Items().size() == 1);
	assert(Items.Items()[0].Kind == ItemKind::LadderBuilder);
	assert(NearlyEqual(Items.Items()[0].VelocityY, -4.0f));

	for (int Frame = 0; Frame < 40 && !Items.Items().empty(); ++Frame) {
		Items.UpdateTerrainItems(Map, Loaded.Value(), 55);
	}

	assert(Items.Items().empty());
	assert(*Map.TryGet({1, 3}) == 55);
	assert(*Map.TryGet({1, 2}) == 55);
	assert(*Map.TryGet({1, 1}) == 55);
	assert(*Map.TryGet({1, 0}) == 1);
}

void TestGravityRegionDefinitions() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Up = Loaded.Value().Find(59);
	const TileDefinition* Down = Loaded.Value().Find(60);
	assert(Up != nullptr && Down != nullptr);
	assert(Up->Collision == CollisionShape::None);
	assert(Down->Collision == CollisionShape::None);
	assert(Up->Movement == MovementRegion::GravityUp);
	assert(Down->Movement == MovementRegion::GravityDown);
}

void TestGravityUpFallsToCeilingAndJumpsAway() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{1, 1, 1},
		{0, 0, 0},
		{0, 59, 0},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Idle;
	Player.Step(Idle, Map, Loaded.Value());
	assert(Player.Gravity() == GravityDirection::Up);
	assert(!Player.Body().Grounded);

	for (int Frame = 0; Frame < 40 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}

	assert(Player.Gravity() == GravityDirection::Up);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
	assert(NearlyEqual(Player.Body().Velocity.Y, 0.0f));

	bool FoundCeilingStand = false;
	for (const TileInteraction& Interaction : Player.Interactions()) {
		if (Interaction.Trigger == TileTrigger::StandOn &&
			Interaction.Position.Column == 1 &&
			Interaction.Position.Row == 0) {
			FoundCeilingStand = true;
		}
	}
	assert(FoundCeilingStand);

	CharacterInput Jump;
	Jump.JumpPressed = true;
	Player.Step(Jump, Map, Loaded.Value());
	assert(Player.Gravity() == GravityDirection::Up);
	assert(!Player.Body().Grounded);
	assert(Player.Body().Velocity.Y > 0.0f);
	assert(Player.Body().Position.Y > 32.0f);
}

void TestGravityDownRegionRestoresNormalGravity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{1, 1, 1, 1},
		{0, 0, 60, 0},
		{0, 59, 0, 0},
		{1, 1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	CharacterInput Idle;

	for (int Frame = 0; Frame < 50 && !Player.IsGravityUp(); ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	assert(Player.IsGravityUp());

	for (int Frame = 0; Frame < 50 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));

	CharacterInput Right;
	Right.Horizontal = 1.0f;
	for (int Frame = 0;
		Frame < 12 && Player.Gravity() != GravityDirection::Down;
		++Frame) {
		Player.Step(Right, Map, Loaded.Value());
	}

	assert(Player.Gravity() == GravityDirection::Down);
	assert(!Player.Body().Grounded);

	for (int Frame = 0; Frame < 50 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}

	assert(Player.Gravity() == GravityDirection::Down);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 64.0f));
}

void TestUpGravityUsesNegativeTerminalVelocity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{0, 59, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	CharacterInput Idle;

	for (int Frame = 0; Frame < 40; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}

	assert(Player.Gravity() == GravityDirection::Up);
	assert(Player.Body().Velocity.Y >= -10.0f - 0.001f);
	assert(NearlyEqual(Player.Body().Velocity.Y, -10.0f));
}

void TestWaterDefinition() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Water = Loaded.Value().Find(58);
	assert(Water != nullptr);
	assert(Water->Collision == CollisionShape::None);
	assert(Water->Movement == MovementRegion::Water);
}

void TestCharacterUsesWaterGravityAndTerminalVelocity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	IntegerGrid WaterRows(
		30, std::vector<int>(3, 58));
	TileMap WaterMap = MakeMap(WaterRows);

	CharacterBody WaterBody;
	WaterBody.Position = {32.0f, 64.0f};
	WaterBody.Grounded = false;
	CharacterController WaterPlayer(WaterBody);

	CharacterInput Idle;
	WaterPlayer.Step(Idle, WaterMap, Loaded.Value());
	assert(WaterPlayer.IsInWater());
	// 0.5 / 3 は0.1刻み速度へ丸められ、この実装では0.2になる。
	assert(NearlyEqual(WaterPlayer.Body().Velocity.Y, 0.2f));

	for (int Frame = 0; Frame < 100; ++Frame) {
		WaterPlayer.Step(Idle, WaterMap, Loaded.Value());
	}
	assert(WaterPlayer.IsInWater());
	assert(WaterPlayer.Body().Velocity.Y <= 5.0f + 0.001f);
	assert(NearlyEqual(WaterPlayer.Body().Velocity.Y, 5.0f));
}

void TestWaterHorizontalMovementIsSlower() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 58, 58, 58},
		{58, 58, 58, 58},
		{58, 58, 58, 58},
		{58, 58, 58, 58}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 48.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Right;
	Right.Horizontal = 1.0f;
	const float OldX = Player.Body().Position.X;
	Player.Step(Right, Map, Loaded.Value());
	assert(Player.IsInWater());
	assert(NearlyEqual(Player.Body().Position.X, OldX + 1.0f));
	assert(NearlyEqual(Player.Body().Velocity.X, 1.0f));
}

void TestWaterStateStaysTrueAgainstRightWall() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 58, 1},
		{58, 58, 1},
		{1, 1, 1}
	});

	CharacterBody Body;
	// CanvasMasao互換の右壁接触位置。x+15=63 はWater、
	// x+16=64 は右隣Solidなので、旧実装ではWater=falseになっていた。
	Body.Position = {48.0f, 32.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Right;
	Right.Horizontal = 1.0f;
	Player.Step(Right, Map, Loaded.Value());

	assert(NearlyEqual(Player.Body().Position.X, 48.0f));
	assert(Player.IsInWater());
}

void TestHorizontalWaterBoundaryDoesNotMultiplyVerticalSpeed() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 0, 0},
		{58, 0, 0},
		{58, 0, 0},
		{58, 0, 0}
	});

	CharacterBody Body;
	// x+15=31 でWater。右へ1px動くと x+15=32 で空気へ出る。
	Body.Position = {16.0f, 48.0f};
	Body.Velocity.Y = -4.0f;
	Body.Grounded = false;
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);

	CharacterInput Right;
	Right.Horizontal = 1.0f;
	Player.Step(Right, Map, Loaded.Value());

	assert(!Player.IsInWater());
	// V1の2.5倍補正はMoveY内だけ。横境界では -4 -> -10 にしてはいけない。
	assert(NearlyEqual(Player.Body().Velocity.Y, -4.0f));
}

void TestRepeatedWallSwimmingDoesNotAccumulateBoundaryBoost() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 58, 1},
		{58, 58, 1},
		{58, 58, 1},
		{58, 58, 1},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {48.0f, 64.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	// 報告された「右壁に沿って何度か泳ぐ」状況を簡略再現。
	for (int Count = 0; Count < 6; ++Count) {
		CharacterInput SwimRight;
		SwimRight.Horizontal = 1.0f;
		SwimRight.JumpPressed = true;
		Player.Step(SwimRight, Map, Loaded.Value());
		assert(Player.IsInWater());
		assert(Player.Body().Velocity.Y >= -6.0f - 0.001f);
	}

	CharacterInput SwimLeft;
	SwimLeft.Horizontal = -1.0f;
	SwimLeft.JumpPressed = true;
	Player.Step(SwimLeft, Map, Loaded.Value());

	assert(Player.IsInWater());
	assert(Player.Body().Velocity.Y >= -6.0f - 0.001f);
}

void TestJumpingLeftIntoWaterFromAirDoesNotLaunch() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{58, 58, 0, 0},
		{58, 58, 0, 0},
		{1, 1, 1, 1}
	});

	CharacterBody Body;
	// x+15=64 で右側の空気、床上から左へジャンプして入水する。
	Body.Position = {49.0f, 64.0f};
	Body.Grounded = true;
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	CharacterController Player(Body, Motion);

	CharacterInput JumpLeft;
	JumpLeft.Horizontal = -1.0f;
	JumpLeft.JumpPressed = true;
	Player.Step(JumpLeft, Map, Loaded.Value());

	// 入水した同じフレームは通常ジャンプの -9 のまま。
	assert(Player.IsInWater());
	assert(NearlyEqual(Player.Body().Position.X, 46.0f));
	assert(NearlyEqual(Player.Body().Velocity.Y, -9.0f));

	// そのままZを押さずに水面を抜けても、通常ジャンプを2.5倍しない。
	float MostNegativeVelocity = Player.Body().Velocity.Y;
	CharacterInput Idle;
	for (int Frame = 0; Frame < 8 && Player.IsInWater(); ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
		MostNegativeVelocity =
			std::min(MostNegativeVelocity, Player.Body().Velocity.Y);
	}

	assert(!Player.IsInWater());
	assert(MostNegativeVelocity >= -9.0f - 0.001f);
	assert(NearlyEqual(Player.Body().Velocity.Y, -9.0f));
}

void TestWaterJumpSpeedsMatchVersion1() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 58, 58},
		{58, 58, 58},
		{58, 58, 58},
		{58, 58, 58}
	});

	auto MakeWaterPlayer = []() {
		CharacterBody Body;
		Body.Position = {32.0f, 48.0f};
		Body.Grounded = false;
		return CharacterController(Body);
	};

	CharacterInput Neutral;
	Neutral.JumpPressed = true;
	CharacterController NeutralPlayer = MakeWaterPlayer();
	NeutralPlayer.Step(Neutral, Map, Loaded.Value());
	assert(NeutralPlayer.IsInWater());
	assert(NearlyEqual(NeutralPlayer.Body().Velocity.Y, -4.0f));

	CharacterInput Up = Neutral;
	Up.Vertical = -1.0f;
	CharacterController UpPlayer = MakeWaterPlayer();
	UpPlayer.Step(Up, Map, Loaded.Value());
	assert(UpPlayer.IsInWater());
	assert(NearlyEqual(UpPlayer.Body().Velocity.Y, -6.0f));

	CharacterInput Down = Neutral;
	Down.Vertical = 1.0f;
	CharacterController DownPlayer = MakeWaterPlayer();
	DownPlayer.Step(Down, Map, Loaded.Value());
	assert(DownPlayer.IsInWater());
	assert(NearlyEqual(DownPlayer.Body().Velocity.Y, -2.0f));
}

void TestWaterJumpCanBeRepeatedWhileAirborne() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 58, 58},
		{58, 58, 58},
		{58, 58, 58},
		{58, 58, 58}
	});
	CharacterBody Body;
	Body.Position = {32.0f, 48.0f};
	Body.Velocity.Y = 3.0f;
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Swim;
	Swim.JumpPressed = true;
	Player.Step(Swim, Map, Loaded.Value());
	assert(Player.IsInWater());
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Velocity.Y, -4.0f));
}

void TestLeavingWaterUpwardBoostsVelocity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{58, 58, 58},
		{58, 58, 58},
		{58, 58, 58}
	});
	CharacterBody Body;
	// 中心Y=33で水中。-4移動すると中心Y=29となり水面を上へ抜ける。
	Body.Position = {32.0f, 17.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Swim;
	Swim.JumpPressed = true;
	Player.Step(Swim, Map, Loaded.Value());

	assert(!Player.IsInWater());
	// V1の水面遷移: speed.y *= 2.5
	assert(NearlyEqual(Player.Body().Velocity.Y, -10.0f));
}

void TestWaterUsesCharacterCenterPoint() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{58, 0, 0},
		{58, 0, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	// 左端はWaterにかかるが、中心X=39は隣の空気タイル。
	Body.Position = {23.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Idle;
	Player.Step(Idle, Map, Loaded.Value());
	assert(!Player.IsInWater());
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
	float LogicalY = 0.0f;
	if (ExtendedSlopeTerrain::TryCharacterY(
		Map, Catalog, CenterX, Player.Body().Position.Y + 31.0f, LogicalY)) {
		ExpectedY = LogicalY;
	}
	if (ExtendedSlopeTerrain::TryCharacterY1x2(
		Map, Catalog, CenterX, Player.Body().Position.Y + 31.0f, LogicalY)) {
		ExpectedY = LogicalY;
	}
	assert(std::fabs(Player.Body().Position.Y - ExpectedY) <= 0.5f);
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

	Body.Position = {4.0f, -48.0f};
	Body.Velocity = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController InAir(Body);
	InAir.Step(0.0f, false, Map, Catalog);
	assert(!InAir.Body().Grounded);
	assert(NearlyEqual(InAir.Body().Position.Y, -48.0f));
	InAir.Step(0.0f, false, Map, Catalog);
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
	TestTileRuleCatalogAndLegacyCompatibility();
	TestTileBehaviorComposesEffectsWithoutManagers();
	TestLegacyBreakableBecomesRuleDriven();
	TestTileOnceRulesAreIndependent();
	TestExternalInteractionStage();
	TestItemBlockDefinitions();
	TestTenCoinBlockUsesGenericCountRules();
	TestOnOffDefinitionsAndWorldState();
	TestCoinConditionalDefinitions();
	TestCoinConditionalBlocksMatchVersion1();
	TestConditionalTerrainComparisonOperators();
	TestTimedDisappearingBlocksToggleEvery80Frames();
	TestOnOffSwitchRuleProducesToggleEffect();
	TestActivatedOnOffBlockPushesCharacterToSafety();
	TestActivatedOnOffBlocksKillWhenCharacterIsCrushed();
	TestItemSystemConvertsSpawnToV1Rewards();
	TestQuestionBlockSpawnsItemAndBecomesUsed();
	TestHiddenItemBlockOnlyBlocksFromBelow();
	TestCharacterEmitsTouchForCollectible();
	TestCharacterTouchIncludesSolidContact();
	TestCharacterEmitsHitFromBelowForBlock();
	TestCanvasMasaoTerrainCodesAndCoordinates();
	TestCanvasMasaoVerticalCrossings();
	TestExtended2x1SlopeIsOneContinuousSurface();
	TestExtended2x1SlopeEdgeVelocityFollowsGradient();
	TestExtended2x1SlopeHighSideAndLanding();
	TestCharacterCanJumpAcrossConnected2x1Peak();
	TestCharacterDescendsConnected2x1PeakWithoutFloorWarp();
	TestCharacterCrossesConnected2x1PeakToLeftWithoutFallingThrough();
	TestCharacterCrossesConnected2x1ValleyToLeftWithoutFallingThrough();
	TestCharacterDescendsStacked2x1BoundaryToLeft();
	TestCharacterDescends2x1SlopeToLeftWithoutFallingThrough();
	TestCharacterEnters2x1HighEdgeAndDescendsToLeft();
	TestCharacterDescends2x1ToLeftFromEverySurfacePixel();
	TestCharacterDescends2x1ToLeftOnExternalStage();
	TestCharacterLeavesFloating2x1LowEdgeNaturally();
	TestCharacterLeaves2x1HighEdgeWithoutWarpingToLowerFloor();
	TestRisingCharacterCannotPass2x1HighSideInsideColumn();
	TestCharacterJumpsLeftAlong2x1HighSideAndLands();
	TestCharacterJumpsLeftAlong2x1HighSideConnectedToBlock();
	TestCharacterCannotRiseThroughStacked2x1RightEdge();
	TestCharacterJumpArcUnderLongStacked2x1Slope();
	TestJumpingCharacterCanMoveAbove2x1SurfaceInsideColumn();
	TestCharacterCanJumpFromBlockInto2x1UpperSpace();
	TestExtended1x2SlopeIsOneContinuousSurface();
	TestExtended1x2SlopeEdgeVelocityFollowsGradient();
	TestExtended1x2SlopeIgnoresInternalVerticalBoundary();
	TestCharacterTraverses1x2SlopeWithoutSeamSnag();
	TestCharacterCannotEnter1x2HighSide();
	TestCharacterLandsOn1x2Slope();
	TestCharacterRepositionResetsInternalVelocity();
	TestLadderDefinitions();
	TestCharacterClimbsLadderWithoutGravity();
	TestLadderEntryRulesMatchVersion1();
	TestLadderBuilderCreatesTilesUntilSolidCeiling();
	TestGravityRegionDefinitions();
	TestGravityUpFallsToCeilingAndJumpsAway();
	TestGravityDownRegionRestoresNormalGravity();
	TestUpGravityUsesNegativeTerminalVelocity();
	TestWaterDefinition();
	TestCharacterUsesWaterGravityAndTerminalVelocity();
	TestWaterHorizontalMovementIsSlower();
	TestWaterStateStaysTrueAgainstRightWall();
	TestHorizontalWaterBoundaryDoesNotMultiplyVerticalSpeed();
	TestRepeatedWallSwimmingDoesNotAccumulateBoundaryBoost();
	TestJumpingLeftIntoWaterFromAirDoesNotLaunch();
	TestWaterJumpSpeedsMatchVersion1();
	TestWaterJumpCanBeRepeatedWhileAirborne();
	TestLeavingWaterUpwardBoostsVelocity();
	TestWaterUsesCharacterCenterPoint();
	TestCharacterMovement();
	TestCharacterRecomputesGroundFromMasaoProbes();
	TestCharacterUsesGetSakamichiYCoordinates();
	TestCharacterSlopeFollow();
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
