#include "../Actgame/Foundation/AssetPaths.h"
#include "../Actgame/Foundation/BrickSystem.h"
#include "../Actgame/Foundation/CharacterController.h"
#include "../Actgame/Foundation/CharacterSafety.h"
#include "../Actgame/Foundation/ConditionalTerrain.h"
#include "../Actgame/Foundation/DamageReactionState.h"
#include "../Actgame/Foundation/CanvasMasaoTerrain.h"
#include "../Actgame/Foundation/ExtendedSlopeTerrain.h"
#include "../Actgame/Foundation/GridDataLoader.h"
#include "../Actgame/Foundation/GoalState.h"
#include "../Actgame/Foundation/StageProgress.h"
#include "../Actgame/Foundation/ItemSystem.h"
#include "../Actgame/Foundation/LayeredMap.h"
#include "../Actgame/Foundation/NativeObjectRuntime.h"
#include "../Actgame/Foundation/NativeStageDataLoader.h"
#include "../Actgame/Foundation/PipeTransport.h"
#include "../Actgame/Foundation/PlayerResourceRules.h"
#include "../Actgame/Foundation/StageDefinition.h"
#include "../Actgame/Foundation/StageData.h"
#include "../Actgame/Foundation/TerrainCollision.h"
#include "../Actgame/Foundation/TerrainStageLoader.h"
#include "../Actgame/Foundation/TileDefinition.h"
#include "../Actgame/Foundation/TileInteraction.h"
#include "../Actgame/Foundation/TileMap.h"
#include "../Actgame/Foundation/WorldState.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
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
	assert(Loaded.Value().Pipes.empty());
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

void TestBrickDefinitionAndHitEffect() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/brick-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Brick = Loaded.Value().Find(72);
	assert(Brick != nullptr);
	assert(Brick->Collision == CollisionShape::Solid);
	assert(Brick->Rules.size() == 1);
	assert(Brick->Rules[0].Trigger == TileTrigger::HitFromBelow);
	assert(Brick->Rules[0].Action == TileAction::HitBrick);
	assert(Brick->Rules[0].Value == BrickSystem::Version1RequiredHealth);
	assert(!Brick->Rules[0].Once);

	TileMap Map = MakeMap({{72}});
	TileRuntimeMap Runtime(Map);
	TileInteraction Hit;
	Hit.Trigger = TileTrigger::HitFromBelow;
	Hit.Position = {0, 0};
	Hit.TileId = 72;

	const TileBehaviorResult Result =
		TileBehaviorSystem::Apply(Hit, Map, Loaded.Value(), Runtime);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::BrickHit);
	assert(Result.Effects[0].Value == 5);
	assert(Result.Effects[0].SourceTileId == 72);
	assert(*Map.TryGet({0, 0}) == 72);
}

void TestBrickWithLowHealthBumpsButDoesNotBreak() {
	TileMap Map = MakeMap({{72}});
	BrickSystem Bricks;

	TileEffect Hit;
	Hit.Type = TileEffectType::BrickHit;
	Hit.Position = {0, 0};
	Hit.Value = 5;
	Hit.SourceTileId = 72;

	GameStateSnapshot State;
	State.Health = 4;
	Bricks.ConsumeTileEffects({Hit}, State);

	const ActiveBrick* Active = Bricks.TryGet({0, 0});
	assert(Active != nullptr);
	assert(Active->Phase == BrickPhase::Bumping);

	for (int Frame = 0; Frame < BrickSystem::Version1BumpFrames - 1; ++Frame) {
		const std::vector<TileEffect> Effects =
			Bricks.Update(Map, 32, 32, 1000.0f);
		assert(Effects.empty());
		assert(*Map.TryGet({0, 0}) == 72);
		assert(Bricks.TryGet({0, 0}) != nullptr);
	}

	const std::vector<TileEffect> Last =
		Bricks.Update(Map, 32, 32, 1000.0f);
	assert(Last.empty());
	assert(*Map.TryGet({0, 0}) == 72);
	assert(Bricks.TryGet({0, 0}) == nullptr);
	assert(Bricks.Fragments().empty());
}

void TestBrickWithFullHealthBreaksAfterVersion1Delay() {
	TileMap Map = MakeMap({{72}});
	BrickSystem Bricks;

	TileEffect Hit;
	Hit.Type = TileEffectType::BrickHit;
	Hit.Position = {0, 0};
	Hit.Value = 5;
	Hit.SourceTileId = 72;

	GameStateSnapshot State;
	State.Health = 5;
	Bricks.ConsumeTileEffects({Hit}, State);

	const ActiveBrick* Active = Bricks.TryGet({0, 0});
	assert(Active != nullptr);
	assert(Active->Phase == BrickPhase::Breaking);

	for (int Frame = 0; Frame < BrickSystem::Version1BreakFrames - 1; ++Frame) {
		const std::vector<TileEffect> Effects =
			Bricks.Update(Map, 32, 32, 1000.0f);
		assert(Effects.empty());
		assert(*Map.TryGet({0, 0}) == 72);
		assert(Bricks.TryGet({0, 0}) != nullptr);
	}

	std::srand(1);
	const std::vector<TileEffect> Effects =
		Bricks.Update(Map, 32, 32, 1000.0f);

	assert(*Map.TryGet({0, 0}) == 0);
	assert(Bricks.TryGet({0, 0}) == nullptr);
	assert(Effects.size() == 2);
	assert(Effects[0].Type == TileEffectType::AddScore);
	assert(Effects[0].Value == 10);
	assert(Effects[1].Type == TileEffectType::TileBroken);
	assert(Bricks.Fragments().size() == BrickSystem::Version1FragmentCount);

	for (const BrickFragment& Fragment : Bricks.Fragments()) {
		assert(Fragment.Position.X == 0.0f);
		assert(Fragment.Position.Y == 0.0f);
		assert(Fragment.Velocity.X >= -5.0f);
		assert(Fragment.Velocity.X <= 5.0f);
		assert(Fragment.Velocity.Y >= -14.0f);
		assert(Fragment.Velocity.Y <= 0.0f);
	}

	const std::vector<BrickFragment> Before = Bricks.Fragments();
	Bricks.Update(Map, 32, 32, 1000.0f);
	assert(Bricks.Fragments().size() == Before.size());
	for (std::size_t Index = 0; Index < Before.size(); ++Index) {
		assert(NearlyEqual(
			Bricks.Fragments()[Index].Velocity.Y,
			std::min(10.0f, Before[Index].Velocity.Y + 0.5f)));
		assert(NearlyEqual(
			Bricks.Fragments()[Index].Position.X,
			Before[Index].Position.X + Before[Index].Velocity.X));
		assert(NearlyEqual(
			Bricks.Fragments()[Index].Position.Y,
			Before[Index].Position.Y +
			std::min(10.0f, Before[Index].Velocity.Y + 0.5f)));
	}
}

void TestBrickIgnoresRepeatedHitsWhileAnimating() {
	TileMap Map = MakeMap({{72}});
	BrickSystem Bricks;

	TileEffect Hit;
	Hit.Type = TileEffectType::BrickHit;
	Hit.Position = {0, 0};
	Hit.Value = 5;
	Hit.SourceTileId = 72;

	GameStateSnapshot Low;
	Low.Health = 4;
	Bricks.ConsumeTileEffects({Hit}, Low);
	Bricks.Update(Map, 32, 32, 1000.0f);

	GameStateSnapshot Full;
	Full.Health = 5;
	Bricks.ConsumeTileEffects({Hit}, Full);

	const ActiveBrick* Active = Bricks.TryGet({0, 0});
	assert(Active != nullptr);
	assert(Active->Phase == BrickPhase::Bumping);
}

void TestHazardDefinitionsMatchVersion1Targets() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/hazard-test/tiles.csv");
	assert(Loaded.IsSuccess());

	struct ExpectedHazard {
		int Id;
		TileAction Action;
		TileTarget Target;
		int Value;
	};
	const ExpectedHazard Expected[] = {
		{73, TileAction::Damage, TileTarget::Player, 1},
		{74, TileAction::Damage, TileTarget::Enemy, 1},
		{75, TileAction::Damage, TileTarget::Both, 1},
		{76, TileAction::InstantDeath, TileTarget::Player, 0},
		{77, TileAction::InstantDeath, TileTarget::Enemy, 0},
		{78, TileAction::InstantDeath, TileTarget::Both, 0}
	};

	for (const ExpectedHazard& Hazard : Expected) {
		const TileDefinition* Definition = Loaded.Value().Find(Hazard.Id);
		assert(Definition != nullptr);
		assert(Definition->Collision == CollisionShape::Solid);
		assert(Definition->Rules.size() == 1);
		assert(Definition->Rules[0].Trigger == TileTrigger::Touch);
		assert(Definition->Rules[0].Action == Hazard.Action);
		assert(Definition->Rules[0].Target == Hazard.Target);
		assert(Definition->Rules[0].Value == Hazard.Value);
		assert(!Definition->Rules[0].Once);
	}
}

void TestHazardTargetsFilterPlayerAndEnemyActors() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/hazard-test/tiles.csv");
	assert(Loaded.IsSuccess());

	auto ApplyHazard = [&](int TileId, TileActor Actor) {
		TileMap Map = MakeMap({{TileId}});
		TileRuntimeMap Runtime(Map);
		TileInteraction Interaction;
		Interaction.Trigger = TileTrigger::Touch;
		Interaction.Position = {0, 0};
		Interaction.TileId = TileId;
		Interaction.Actor = Actor;
		return TileBehaviorSystem::Apply(
			Interaction, Map, Loaded.Value(), Runtime);
	};

	TileBehaviorResult Result = ApplyHazard(73, TileActor::Player);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::Damage);
	assert(Result.Effects[0].Actor == TileActor::Player);

	Result = ApplyHazard(74, TileActor::Player);
	assert(!Result.Handled);
	assert(Result.Effects.empty());

	Result = ApplyHazard(75, TileActor::Player);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::Damage);
	assert(Result.Effects[0].Actor == TileActor::Player);

	Result = ApplyHazard(76, TileActor::Player);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::InstantDeath);

	Result = ApplyHazard(77, TileActor::Player);
	assert(!Result.Handled);
	assert(Result.Effects.empty());

	Result = ApplyHazard(78, TileActor::Player);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::InstantDeath);

	Result = ApplyHazard(73, TileActor::Enemy);
	assert(!Result.Handled);
	assert(Result.Effects.empty());

	Result = ApplyHazard(74, TileActor::Enemy);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::Damage);
	assert(Result.Effects[0].Actor == TileActor::Enemy);

	Result = ApplyHazard(75, TileActor::Enemy);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::Damage);
	assert(Result.Effects[0].Actor == TileActor::Enemy);

	Result = ApplyHazard(76, TileActor::Enemy);
	assert(!Result.Handled);
	assert(Result.Effects.empty());

	Result = ApplyHazard(77, TileActor::Enemy);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::InstantDeath);
	assert(Result.Effects[0].Actor == TileActor::Enemy);

	Result = ApplyHazard(78, TileActor::Enemy);
	assert(Result.Handled);
	assert(Result.Effects.size() == 1);
	assert(Result.Effects[0].Type == TileEffectType::InstantDeath);
	assert(Result.Effects[0].Actor == TileActor::Enemy);
}

void TestLegacyDamagingRuleStillTargetsPlayer() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	const TileDefinition* Damaging = Loaded.Value().Find(22);
	assert(Damaging != nullptr);
	assert(Damaging->Rules.size() == 1);
	assert(Damaging->Rules[0].Action == TileAction::Damage);
	assert(Damaging->Rules[0].Target == TileTarget::Player);
}

void TestCharacterExposesActualTouchProbePoints() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/hazard-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Input;
	Player.Step(Input, Map, Loaded.Value());

	const std::vector<WorldPosition>& Probes = Player.TouchProbePoints();
	assert(Probes.size() == 5);

	const CharacterBody& Current = Player.Body();
	const CharacterTouchBounds Bounds = Player.TouchBounds();
	assert(NearlyEqual(Bounds.Left, Current.Position.X + 8.0f));
	assert(NearlyEqual(Bounds.Right, Current.Position.X + 24.0f));
	assert(NearlyEqual(Bounds.Top, Current.Position.Y));
	assert(NearlyEqual(Bounds.Bottom, Current.Position.Y + 32.0f));

	assert(NearlyEqual(Probes[0].X, Bounds.Left));
	assert(NearlyEqual(Probes[0].Y, Bounds.Top));
	assert(NearlyEqual(Probes[1].X, Bounds.Right));
	assert(NearlyEqual(Probes[1].Y, Bounds.Top));
	assert(NearlyEqual(Probes[2].X, Bounds.Left));
	assert(NearlyEqual(Probes[2].Y, Bounds.Bottom));
	assert(NearlyEqual(Probes[3].X, Bounds.Right));
	assert(NearlyEqual(Probes[3].Y, Bounds.Bottom));
	assert(NearlyEqual(Probes[4].X, Current.Position.X + Current.Width * 0.5f));
	assert(NearlyEqual(Probes[4].Y, Current.Position.Y + Current.Height * 0.5f));
}

void TestDamageKnockbackMovesAwayFromHazardCenter() {
	// 危険ブロック中心より左にいれば左へ逃がす。
	assert(DamageReactionState::DirectionAwayFromSource(
		95.0f, 112.0f, 1) == -1);

	// 今回の問題ケース:
	// 危険ブロック右側なら、右向きだったとしても右へ逃がす。
	assert(DamageReactionState::DirectionAwayFromSource(
		129.0f, 112.0f, -1) == 1);

	// 真上/真下などX中心が一致した時だけfallbackを使う。
	assert(DamageReactionState::DirectionAwayFromSource(
		112.0f, 112.0f, -1) == -1);
	assert(DamageReactionState::DirectionAwayFromSource(
		112.0f, 112.0f, 1) == 1);
}

void TestDamageReactionMatchesVersion1SixteenFrames() {
	DamageReactionState Damage;
	assert(!Damage.Active());
	assert(Damage.Begin(-1));
	assert(Damage.Active());
	assert(Damage.KnockbackDirection() == -1);
	assert(!Damage.Begin(1));

	for (int Frame = 1;
		Frame < DamageReactionState::Version1DurationFrames; ++Frame) {
		assert(Damage.AdvanceFrame() == -1.0f);
		assert(Damage.Active());
		assert(Damage.Frame() == Frame);
	}

	assert(Damage.AdvanceFrame() == 0.0f);
	assert(!Damage.Active());
	assert(Damage.Frame() == DamageReactionState::Version1DurationFrames);

	assert(Damage.Begin(1));
	assert(Damage.KnockbackDirection() == 1);
	assert(Damage.AdvanceFrame() == 1.0f);

	Damage.Reset();
	assert(!Damage.Active());
	assert(Damage.Frame() == 0);
}

void TestDirectCollectibleDefinitionsMatchVersion1() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/collectible-test/stage.ini");
	assert(Loaded.IsSuccess());

	const TileDefinition* Coin = Loaded.Value().Catalog.Find(79);
	const TileDefinition* Healing = Loaded.Value().Catalog.Find(80);
	const TileDefinition* OneUp = Loaded.Value().Catalog.Find(81);
	assert(Coin != nullptr && Healing != nullptr && OneUp != nullptr);

	assert(Coin->Collision == CollisionShape::None);
	assert(Healing->Collision == CollisionShape::None);
	assert(OneUp->Collision == CollisionShape::None);

	assert(Coin->Rules.size() == 3);
	assert(Coin->Rules[0].Trigger == TileTrigger::Touch);
	assert(Coin->Rules[0].Action == TileAction::AddCoin);
	assert(Coin->Rules[0].Value == 1);
	assert(Coin->Rules[1].Action == TileAction::AddScore);
	assert(Coin->Rules[1].Value == 100);
	assert(Coin->Rules[2].Action == TileAction::ReplaceTile);
	assert(Coin->Rules[2].Value == 0);

	assert(Healing->Rules.size() == 3);
	assert(Healing->Rules[0].Action == TileAction::AddHealth);
	assert(Healing->Rules[0].Value == 1);
	assert(Healing->Rules[1].Action == TileAction::AddScore);
	assert(Healing->Rules[1].Value == 1000);
	assert(Healing->Rules[2].Action == TileAction::ReplaceTile);
	assert(Healing->Rules[2].Value == 0);

	assert(OneUp->Rules.size() == 2);
	assert(OneUp->Rules[0].Action == TileAction::AddLife);
	assert(OneUp->Rules[0].Value == 1);
	assert(OneUp->Rules[1].Action == TileAction::ReplaceTile);
	assert(OneUp->Rules[1].Value == 0);
}

void TestDirectCollectiblesEmitVersion1RewardsAndDisappear() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/collectible-test/tiles.csv");
	assert(Loaded.IsSuccess());

	auto TouchCollectible = [&](int Id) {
		TileMap Map = MakeMap({{Id}});
		TileRuntimeMap Runtime(Map);
		TileInteraction Touch;
		Touch.Trigger = TileTrigger::Touch;
		Touch.Position = {0, 0};
		Touch.TileId = Id;
		const TileBehaviorResult Result =
			TileBehaviorSystem::Apply(Touch, Map, Loaded.Value(), Runtime);
		assert(*Map.TryGet({0, 0}) == 0);
		return Result.Effects;
	};

	std::vector<TileEffect> Effects = TouchCollectible(79);
	assert(Effects.size() == 2);
	assert(Effects[0].Type == TileEffectType::AddCoin);
	assert(Effects[0].Value == 1);
	assert(Effects[1].Type == TileEffectType::AddScore);
	assert(Effects[1].Value == 100);

	Effects = TouchCollectible(80);
	assert(Effects.size() == 2);
	assert(Effects[0].Type == TileEffectType::AddHealth);
	assert(Effects[0].Value == 1);
	assert(Effects[1].Type == TileEffectType::AddScore);
	assert(Effects[1].Value == 1000);

	Effects = TouchCollectible(81);
	assert(Effects.size() == 1);
	assert(Effects[0].Type == TileEffectType::AddLife);
	assert(Effects[0].Value == 1);
}

void TestPlayerResourceRulesMatchVersion1Limits() {
	int Health = 4;
	PlayerResourceRules::AddHealth(1, Health);
	assert(Health == 5);
	PlayerResourceRules::AddHealth(1, Health);
	assert(Health == PlayerResourceRules::MaxHealth);

	int Lives = 998;
	PlayerResourceRules::AddLife(1, Lives);
	assert(Lives == 999);
	PlayerResourceRules::AddLife(1, Lives);
	assert(Lives == PlayerResourceRules::MaxLives);

	int Coins = 98;
	Lives = 3;
	assert(!PlayerResourceRules::AddCoin(1, Coins, Lives));
	assert(Coins == 99);
	assert(Lives == 3);

	assert(PlayerResourceRules::AddCoin(1, Coins, Lives));
	assert(Coins == 0);
	assert(Lives == 4);

	// V1は残機999でも100コイン到達時にCoinを100減らす。
	Coins = 99;
	Lives = 999;
	assert(PlayerResourceRules::AddCoin(1, Coins, Lives));
	assert(Coins == 0);
	assert(Lives == 999);
}

void TestGoalStageDefinitionsAndEffects() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/goal-test/stage.ini");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().Map.Width() == 16);
	assert(Loaded.Value().Map.Height() == 9);
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.X, 96.0f));
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.Y, 160.0f));

	const TileDefinition* Normal = Loaded.Value().Catalog.Find(70);
	const TileDefinition* Secret = Loaded.Value().Catalog.Find(71);
	assert(Normal != nullptr && Secret != nullptr);
	assert(Normal->Rules.size() == 3);
	assert(Secret->Rules.size() == 3);
	assert(Normal->Rules[0].Action == TileAction::Goal);
	assert(Normal->Rules[0].Value == static_cast<int>(GoalKind::Normal));
	assert(Secret->Rules[0].Action == TileAction::Goal);
	assert(Secret->Rules[0].Value == static_cast<int>(GoalKind::Secret));

	TileMap NormalMap = MakeMap({{70}});
	TileRuntimeMap NormalRuntime(NormalMap);
	TileInteraction Touch;
	Touch.Trigger = TileTrigger::Touch;
	Touch.Position = {0, 0};
	Touch.TileId = 70;
	const TileBehaviorResult NormalResult =
		TileBehaviorSystem::Apply(Touch, NormalMap, Loaded.Value().Catalog, NormalRuntime);
	assert(NormalResult.Handled);
	assert(NormalResult.Effects.size() == 2);
	assert(NormalResult.Effects[0].Type == TileEffectType::Goal);
	assert(NormalResult.Effects[0].Value == static_cast<int>(GoalKind::Normal));
	assert(NormalResult.Effects[1].Type == TileEffectType::AddScore);
	assert(NormalResult.Effects[1].Value == 1000);
	assert(*NormalMap.TryGet({0, 0}) == 0);

	TileMap SecretMap = MakeMap({{71}});
	TileRuntimeMap SecretRuntime(SecretMap);
	Touch.TileId = 71;
	const TileBehaviorResult SecretResult =
		TileBehaviorSystem::Apply(Touch, SecretMap, Loaded.Value().Catalog, SecretRuntime);
	assert(SecretResult.Handled);
	assert(SecretResult.Effects.size() == 2);
	assert(SecretResult.Effects[0].Type == TileEffectType::Goal);
	assert(SecretResult.Effects[0].Value == static_cast<int>(GoalKind::Secret));
	assert(SecretResult.Effects[1].Type == TileEffectType::AddScore);
	assert(SecretResult.Effects[1].Value == 1000);
	assert(*SecretMap.TryGet({0, 0}) == 0);
}

void TestNormalAndSecretGoalProgressAreIndependent() {
	StageClearState State;
	assert(!State.NormalCleared);
	assert(!State.SecretCleared);
	assert(!State.AllCleared());

	State.Record(GoalKind::Normal);
	assert(State.NormalCleared);
	assert(!State.SecretCleared);
	assert(State.IsCleared(GoalKind::Normal));
	assert(!State.IsCleared(GoalKind::Secret));
	assert(!State.AllCleared());

	State.Record(GoalKind::Secret);
	assert(State.NormalCleared);
	assert(State.SecretCleared);
	assert(State.IsCleared(GoalKind::Secret));
	assert(State.AllCleared());

	GoalKind Parsed = GoalKind::Normal;
	assert(TryGoalKindFromValue(0, Parsed));
	assert(Parsed == GoalKind::Normal);
	assert(TryGoalKindFromValue(1, Parsed));
	assert(Parsed == GoalKind::Secret);
	assert(!TryGoalKindFromValue(2, Parsed));
}

void TestStageCompletionEndsRunWithOneGoal() {
	StageCompletionState Completion;
	assert(!Completion.Cleared);

	Completion.Complete(GoalKind::Normal);
	assert(Completion.Cleared);
	assert(Completion.Goal == GoalKind::Normal);

	// ステージ終了後に別ゴールが届いても、今回の結果は上書きしない。
	Completion.Complete(GoalKind::Secret);
	assert(Completion.Goal == GoalKind::Normal);

	Completion.Reset();
	assert(!Completion.Cleared);

	Completion.Complete(GoalKind::Secret);
	assert(Completion.Cleared);
	assert(Completion.Goal == GoalKind::Secret);
}

void TestStageProgressTracksClearStatePerStage() {
	StageProgress Progress;
	assert(!Progress.IsCleared(7, GoalKind::Normal));
	assert(!Progress.IsCleared(7, GoalKind::Secret));
	assert(!Progress.Satisfies(7, ClearRequirement::Either));
	assert(!Progress.Satisfies(7, ClearRequirement::Both));

	assert(Progress.MarkCleared(7, GoalKind::Normal));
	assert(!Progress.MarkCleared(7, GoalKind::Normal));
	assert(Progress.IsCleared(7, GoalKind::Normal));
	assert(!Progress.IsCleared(7, GoalKind::Secret));
	assert(Progress.Satisfies(7, ClearRequirement::Normal));
	assert(!Progress.Satisfies(7, ClearRequirement::Secret));
	assert(Progress.Satisfies(7, ClearRequirement::Either));
	assert(!Progress.Satisfies(7, ClearRequirement::Both));

	assert(Progress.MarkCleared(7, GoalKind::Secret));
	assert(Progress.Satisfies(7, ClearRequirement::Both));

	// 別ステージの進行は独立している。
	assert(!Progress.IsCleared(8, GoalKind::Normal));
	assert(!Progress.Satisfies(8, ClearRequirement::Either));
	assert(!Progress.MarkCleared(-1, GoalKind::Normal));

	Progress.Reset();
	assert(!Progress.IsCleared(7, GoalKind::Normal));
	assert(!Progress.IsCleared(7, GoalKind::Secret));
}

void TestStepWithoutInputStopsHorizontalAndSettlesVertically() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/goal-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Velocity = {4.0f, -3.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	const float StartX = Player.Body().Position.X;
	const float StartY = Player.Body().Position.Y;

	Player.StepWithoutInput(Map, Loaded.Value());

	// 横入力は受け付けず、そのフレームからX座標を固定する。
	assert(NearlyEqual(Player.Body().Position.X, StartX));
	assert(NearlyEqual(Player.Body().Velocity.X, 0.0f));

	// 縦速度は消さず、取得時の上向き慣性へ重力だけを加える。
	assert(Player.Body().Position.Y < StartY);
	assert(Player.Body().Velocity.Y < 0.0f);

	for (int Frame = 0; Frame < 120 && !Player.Body().Grounded; ++Frame) {
		Player.StepWithoutInput(Map, Loaded.Value());
		assert(NearlyEqual(Player.Body().Position.X, StartX));
	}

	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.X, StartX));
	assert(NearlyEqual(Player.Body().Position.Y, 96.0f));
	assert(NearlyEqual(Player.Body().Velocity.Y, 0.0f));
}

void TestExternalPipeStage() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/pipe-test/stage.ini");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().Map.Width() == 16);
	assert(Loaded.Value().Map.Height() == 9);
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.X, 144.0f));
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.Y, 160.0f));
	assert(Loaded.Value().Pipes.size() == 2);

	const PipeLink& First = Loaded.Value().Pipes[0];
	assert(NearlyEqual(First.EntryPosition.X, 4.5f * 32.0f));
	assert(NearlyEqual(First.EntryPosition.Y, 5.0f * 32.0f));
	assert(First.EnterDirection == PipeDirection::Down);
	assert(NearlyEqual(First.ExitPosition.X, 10.5f * 32.0f));
	assert(NearlyEqual(First.ExitPosition.Y, 5.0f * 32.0f));
	assert(First.ExitDirection == PipeDirection::Up);
}

void TestPipeDirectionInputMatching() {
	CharacterInput Input;

	Input.Vertical = 1.0f;
	assert(PipeTransport::MatchesInput(PipeDirection::Down, Input));
	assert(!PipeTransport::MatchesInput(PipeDirection::Up, Input));

	Input = CharacterInput();
	Input.Vertical = -1.0f;
	assert(PipeTransport::MatchesInput(PipeDirection::Up, Input));

	Input = CharacterInput();
	Input.Horizontal = 1.0f;
	assert(PipeTransport::MatchesInput(PipeDirection::Right, Input));

	Input = CharacterInput();
	Input.Horizontal = -1.0f;
	assert(PipeTransport::MatchesInput(PipeDirection::Left, Input));
}

void TestPipeTransportRequiresDirectionAndAlignment() {
	std::vector<PipeLink> Links(1);
	Links[0].EntryPosition = {100.0f, 200.0f};
	Links[0].EnterDirection = PipeDirection::Down;
	Links[0].ExitPosition = {300.0f, 200.0f};
	Links[0].ExitDirection = PipeDirection::Up;

	CharacterBody Body;
	Body.Position = {100.0f, 200.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	PipeTransport Pipe;

	CharacterInput Up;
	Up.Vertical = -1.0f;
	assert(!Pipe.TryBegin(Up, Player, Links));

	CharacterInput Down;
	Down.Vertical = 1.0f;
	Player.Reposition({106.0f, 200.0f});
	assert(!Pipe.TryBegin(Down, Player, Links, 4.0f));

	Player.Reposition({103.0f, 202.0f});
	assert(Pipe.TryBegin(Down, Player, Links, 4.0f));
	assert(Pipe.Phase() == PipeTransportPhase::Entering);
}

void TestSidePipeRequiresGrounded() {
	std::vector<PipeLink> Links(1);
	Links[0].EntryPosition = {64.0f, 64.0f};
	Links[0].EnterDirection = PipeDirection::Right;
	Links[0].ExitPosition = {160.0f, 64.0f};
	Links[0].ExitDirection = PipeDirection::Left;

	CharacterBody Body;
	Body.Position = Links[0].EntryPosition;
	Body.Grounded = false;
	CharacterController Player(Body);
	PipeTransport Pipe;

	CharacterInput Right;
	Right.Horizontal = 1.0f;
	assert(!Pipe.TryBegin(Right, Player, Links));

	Player.Body().Grounded = true;
	assert(Pipe.TryBegin(Right, Player, Links));
}

void TestPipeTransportFadesBeforeEmergence() {
	std::vector<PipeLink> Links(1);
	Links[0].EntryPosition = {100.0f, 200.0f};
	Links[0].EnterDirection = PipeDirection::Down;
	Links[0].ExitPosition = {300.0f, 160.0f};
	Links[0].ExitDirection = PipeDirection::Up;

	CharacterBody Body;
	Body.Position = Links[0].EntryPosition;
	Body.Grounded = true;
	CharacterController Player(Body);
	PipeTransport Pipe;

	CharacterInput Down;
	Down.Vertical = 1.0f;
	assert(Pipe.TryBegin(Down, Player, Links));

	for (int Frame = 0; Frame < PipeTransport::TransitionFrames; ++Frame) {
		Pipe.Update(Player);
	}

	// 32F潜った直後はまだ入口側。ここから暗転を始める。
	assert(Pipe.Phase() == PipeTransportPhase::FadeOut);
	assert(Pipe.FadeAlpha() == 0);
	assert(NearlyEqual(Player.Body().Position.X, 100.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 232.0f));

	// FadeStep=8。約32Fかけて暗転し、完全暗転までは座標を切り替えない。
	for (int Step = 0; Step < 31; ++Step) {
		Pipe.Update(Player);
		assert(Pipe.Phase() == PipeTransportPhase::FadeOut);
		assert(Pipe.FadeAlpha() == (Step + 1) * PipeTransport::FadeStep);
		assert(NearlyEqual(Player.Body().Position.X, 100.0f));
		assert(NearlyEqual(Player.Body().Position.Y, 232.0f));
	}

	// 32回目で255へ到達し、その瞬間だけ出口内部へ移る。
	Pipe.Update(Player);
	assert(Pipe.Phase() == PipeTransportPhase::FadeIn);
	assert(Pipe.FadeAlpha() == 255);
	assert(NearlyEqual(Player.Body().Position.X, 300.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 192.0f));

	// 明るく戻っている間も出口内部で静止する。
	for (int Step = 0; Step < 31; ++Step) {
		Pipe.Update(Player);
		assert(Pipe.Phase() == PipeTransportPhase::FadeIn);
		assert(Pipe.FadeAlpha() == 255 - (Step + 1) * PipeTransport::FadeStep);
		assert(NearlyEqual(Player.Body().Position.X, 300.0f));
		assert(NearlyEqual(Player.Body().Position.Y, 192.0f));
	}

	// 32回目で完全に明るく戻ってからEmergingへ入る。
	Pipe.Update(Player);
	assert(Pipe.Phase() == PipeTransportPhase::Emerging);
	assert(Pipe.FadeAlpha() == 0);
	assert(NearlyEqual(Player.Body().Position.X, 300.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 192.0f));

	for (int Frame = 0; Frame < PipeTransport::TransitionFrames; ++Frame) {
		Pipe.Update(Player);
	}

	assert(Pipe.Phase() == PipeTransportPhase::Idle);
	assert(!Pipe.IsActive());
	assert(Pipe.FadeAlpha() == 0);
	assert(NearlyEqual(Player.Body().Position.X, 300.0f));
	assert(NearlyEqual(Player.Body().Position.Y, 160.0f));
}


void TestPipeTileDefinitionsAreSolid() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/pipe-test/tiles.csv");
	assert(Loaded.IsSuccess());
	for (int Id = 61; Id <= 68; ++Id) {
		const TileDefinition* Pipe = Loaded.Value().Find(Id);
		assert(Pipe != nullptr);
		assert(Pipe->Collision == CollisionShape::Solid);
	}
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

void TestStagePropertyValuesKeepTypes() {
	StagePropertyValue Integer = StagePropertyValue::Integer(7);
	StagePropertyValue Float = StagePropertyValue::Float(2.5f);
	StagePropertyValue Boolean = StagePropertyValue::Boolean(true);
	StagePropertyValue String = StagePropertyValue::String("left");
	StagePropertyValue Vector = StagePropertyValue::Vector2({12.0f, 34.0f});

	int IntValue = 0;
	float FloatValue = 0.0f;
	bool BoolValue = false;
	std::string StringValue;
	WorldPosition VectorValue;

	assert(Integer.TryGetInteger(IntValue));
	assert(IntValue == 7);
	// JSON serializerが2.0を2へ正規化してもsemantic floatとして読める。
	assert(Integer.TryGetFloat(FloatValue));
	assert(NearlyEqual(FloatValue, 7.0f));

	assert(Float.TryGetFloat(FloatValue));
	assert(NearlyEqual(FloatValue, 2.5f));

	assert(Boolean.TryGetBoolean(BoolValue));
	assert(BoolValue);

	assert(String.TryGetString(StringValue));
	assert(StringValue == "left");

	assert(Vector.TryGetVector2(VectorValue));
	assert(NearlyEqual(VectorValue.X, 12.0f));
	assert(NearlyEqual(VectorValue.Y, 34.0f));
}

void TestNativeGoalUsesCentral16x32TouchBounds() {
	CharacterBody Body;
	Body.Position = {164.0f, 128.0f};
	CharacterController Player(Body);

	const StageRegionGeometry Goal =
		StageRegionGeometry::Rectangle(
			{192.0f, 128.0f}, 32.0f, 32.0f);

	// 見た目32x32なら右端がGoalへ4px食い込む位置だが、
	// Goal判定に使う中央16x32はまだ届いていない。
	assert(Body.Position.X + Body.Width > 192.0f);
	CharacterTouchBounds Touch = Player.TouchBounds();
	assert(NearlyEqual(Touch.Right - Touch.Left, 16.0f));
	assert(NearlyEqual(Touch.Bottom - Touch.Top, 32.0f));
	assert(!Goal.IntersectsRectangle(
		{Touch.Left, Touch.Top},
		{Touch.Right - Touch.Left, Touch.Bottom - Touch.Top}));

	// 中央16px判定がGoalへ入ったら成立する。
	Player.Reposition({177.0f, 128.0f});
	Touch = Player.TouchBounds();
	assert(Goal.IntersectsRectangle(
		{Touch.Left, Touch.Top},
		{Touch.Right - Touch.Left, Touch.Bottom - Touch.Top}));
}

void TestStageRegionGeometryIntersection() {
	const StageRegionGeometry Rectangle =
		StageRegionGeometry::Rectangle({100.0f, 50.0f}, 32.0f, 64.0f);
	assert(Rectangle.IntersectsRectangle(
		{90.0f, 60.0f}, {20.0f, 20.0f}));
	assert(Rectangle.IntersectsRectangle(
		{100.0f, 50.0f}, {32.0f, 64.0f}));
	// 半開矩形なので右端/下端で触れるだけでは発火しない。
	assert(!Rectangle.IntersectsRectangle(
		{132.0f, 60.0f}, {16.0f, 16.0f}));
	assert(!Rectangle.IntersectsRectangle(
		{110.0f, 114.0f}, {16.0f, 16.0f}));

	const StageRegionGeometry Point =
		StageRegionGeometry::Point({128.0f, 96.0f});
	assert(Point.IntersectsRectangle(
		{120.0f, 88.0f}, {16.0f, 16.0f}));
	assert(!Point.IntersectsRectangle(
		{128.0f, 96.0f}, {0.0f, 32.0f}));
	assert(!Point.IntersectsRectangle(
		{112.0f, 96.0f}, {16.0f, 16.0f}));
}

void TestNativeStageDataSupportsOverlappingContent() {
	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Metadata.ZOrder = 0;
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.Map = MakeMap({{1, 1}, {1, 1}});

	TileLayer BackDecoration;
	BackDecoration.Metadata.Id = "decoration-back";
	BackDecoration.Metadata.Name = "Back Decoration";
	BackDecoration.Metadata.ZOrder = -10;
	BackDecoration.Role = TileLayerRole::Visual;
	BackDecoration.Map = MakeMap({{2, 0}, {0, 0}});

	TileLayer FrontDecoration;
	FrontDecoration.Metadata.Id = "decoration-front";
	FrontDecoration.Metadata.Name = "Front Decoration";
	FrontDecoration.Metadata.ZOrder = 10;
	FrontDecoration.Role = TileLayerRole::Visual;
	FrontDecoration.Map = MakeMap({{3, 0}, {0, 0}});

	ObjectSpawn Enemy;
	Enemy.Id = "enemy-1";
	Enemy.TypeId = "WalkingEnemy";
	Enemy.Position = {32.0f, 32.0f};
	Enemy.Properties["direction"] = StagePropertyValue::String("left");

	ObjectSpawn Lift;
	Lift.Id = "lift-1";
	Lift.TypeId = "HorizontalLift";
	Lift.Position = {32.0f, 32.0f};
	Lift.Properties["range"] = StagePropertyValue::Float(192.0f);
	Lift.Properties["speed"] = StagePropertyValue::Float(2.0f);

	ObjectLayer Objects;
	Objects.Metadata.Id = "objects";
	Objects.Metadata.Name = "Objects";
	Objects.Metadata.ZOrder = 5;
	Objects.Objects.push_back(Enemy);
	Objects.Objects.push_back(Lift);

	StageRegion CameraRegion;
	CameraRegion.Id = "camera-1";
	CameraRegion.TypeId = "CameraTrigger";
	CameraRegion.Geometry = StageRegionGeometry::Rectangle(
		{32.0f, 32.0f}, 64.0f, 32.0f);

	RegionLayer Events;
	Events.Metadata.Id = "events";
	Events.Metadata.Name = "Events";
	Events.Metadata.ZOrder = 20;
	Events.Regions.push_back(CameraRegion);

	StageTransition Pipe;
	Pipe.Id = "pipe-1";
	Pipe.TypeId = "Pipe";
	Pipe.Entry = StageRegionGeometry::Point({32.0f, 32.0f});
	Pipe.TargetAreaId = "main";
	Pipe.ExitPosition = {0.0f, 32.0f};
	Pipe.EnterDirection = StageDirection::Down;
	Pipe.ExitDirection = StageDirection::Up;

	StageArea Area;
	Area.Id = "main";
	Area.Width = 2;
	Area.Height = 2;
	Area.TileLayers = {BackDecoration, Terrain, FrontDecoration};
	Area.ObjectLayers = {Objects};
	Area.RegionLayers = {Events};
	Area.Transitions = {Pipe};

	StageData Data;
	Data.Id = "overlap-test";
	Data.StartAreaId = "main";
	Data.Areas = {Area};

	Result<bool> Validation = ValidateStageData(Data);
	assert(Validation.IsSuccess());

	const StageArea* LoadedArea = Data.FindArea("main");
	assert(LoadedArea != nullptr);
	assert(LoadedArea->TerrainLayer() != nullptr);
	assert(LoadedArea->FindTileLayer("decoration-back") != nullptr);
	assert(LoadedArea->FindTileLayer("decoration-front") != nullptr);

	const ObjectLayer* LoadedObjects = LoadedArea->FindObjectLayer("objects");
	assert(LoadedObjects != nullptr);
	assert(LoadedObjects->Objects.size() == 2);
	assert(NearlyEqual(
		LoadedObjects->Objects[0].Position.X,
		LoadedObjects->Objects[1].Position.X));
	assert(NearlyEqual(
		LoadedObjects->Objects[0].Position.Y,
		LoadedObjects->Objects[1].Position.Y));

	const RegionLayer* LoadedEvents = LoadedArea->FindRegionLayer("events");
	assert(LoadedEvents != nullptr);
	assert(LoadedEvents->Regions.size() == 1);
	assert(LoadedArea->Transitions.size() == 1);
}

void TestNativeStageDataValidationRejectsAmbiguousStructure() {
	TileLayer TerrainA;
	TerrainA.Metadata.Id = "terrain-a";
	TerrainA.Metadata.Name = "Terrain A";
	TerrainA.Role = TileLayerRole::Terrain;
	TerrainA.Map = MakeMap({{1}});

	TileLayer TerrainB = TerrainA;
	TerrainB.Metadata.Id = "terrain-b";
	TerrainB.Metadata.Name = "Terrain B";

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {TerrainA, TerrainB};

	StageData Data;
	Data.Id = "invalid";
	Data.StartAreaId = "main";
	Data.Areas = {Area};

	assert(ValidateStageData(Data).IsFailure());

	Area.TileLayers = {TerrainA};
	ObjectSpawn Enemy;
	Enemy.Id = "shared-id";
	Enemy.TypeId = "WalkingEnemy";

	ObjectLayer Objects;
	Objects.Metadata.Id = "objects";
	Objects.Metadata.Name = "Objects";
	Objects.Objects.push_back(Enemy);

	StageRegion Region;
	Region.Id = "shared-id";
	Region.TypeId = "Goal";
	Region.Geometry = StageRegionGeometry::Point({0.0f, 0.0f});

	RegionLayer Events;
	Events.Metadata.Id = "events";
	Events.Metadata.Name = "Events";
	Events.Regions.push_back(Region);

	Area.ObjectLayers = {Objects};
	Area.RegionLayers = {Events};
	Data.Areas = {Area};

	assert(ValidateStageData(Data).IsFailure());
}

void TestNativeStageDataAllowsExternalTransitions() {
	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.Map = MakeMap({{1}});

	StageTransition Exit;
	Exit.Id = "exit-to-stage-2";
	Exit.TypeId = "Door";
	Exit.Entry = StageRegionGeometry::Rectangle(
		{0.0f, 0.0f}, 32.0f, 32.0f);
	Exit.TargetStageId = "stage-2";
	Exit.TargetAreaId = "entrance";
	Exit.ExitPosition = {64.0f, 96.0f};

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {Terrain};
	Area.Transitions = {Exit};

	StageData Data;
	Data.Id = "stage-1";
	Data.StartAreaId = "main";
	Data.Areas = {Area};

	assert(ValidateStageData(Data).IsSuccess());
}

void TestNativeStageDataLoaderLoadsJsonAndCsv() {
	Result<StageData> Loaded =
		NativeStageDataLoader::Load("dat/stage/native-test/stage.json");
	assert(Loaded.IsSuccess());

	const StageData& Data = Loaded.Value();
	assert(Data.Id == "native-test");
	assert(Data.Mode == GameMode::Action);
	assert(Data.StartAreaId == "main");
	assert(Data.TileSets.size() == 1);
	const TileSetDefinition* TileSet = Data.FindTileSet("native-test");
	assert(TileSet != nullptr);
	assert(TileSet->ImageFile.find("tiles.png") != std::string::npos);
	assert(TileSet->TileWidth == 32);
	assert(TileSet->TileHeight == 32);
	assert(TileSet->Columns == 5);
	assert(TileSet->Rows == 1);
	assert(TileSet->TileCount() == 5);
	assert(TileSet->EmptyTileId == 0);
	assert(TileSet->Transparent);
	assert(TileSet->TerrainTiles.size() == 14);
	const TileDefinition* EmptyTerrain = TileSet->FindTerrainTile(0);
	const TileDefinition* SolidTerrain = TileSet->FindTerrainTile(2);
	const TileDefinition* CoinTerrain = TileSet->FindTerrainTile(4);
	const TileDefinition* QuestionTerrain = TileSet->FindTerrainTile(5);
	const TileDefinition* BrickTerrain = TileSet->FindTerrainTile(6);
	const TileDefinition* SwitchTerrain = TileSet->FindTerrainTile(8);
	const TileDefinition* SwitchOnTerrain = TileSet->FindTerrainTile(9);
	const TileDefinition* SwitchOffTerrain = TileSet->FindTerrainTile(10);
	const TileDefinition* PlayerDamageTerrain = TileSet->FindTerrainTile(11);
	const TileDefinition* EnemyDamageTerrain = TileSet->FindTerrainTile(12);
	const TileDefinition* BothDamageTerrain = TileSet->FindTerrainTile(13);
	const TileDefinition* PlayerDeathTerrain = TileSet->FindTerrainTile(14);
	const TileDefinition* EnemyDeathTerrain = TileSet->FindTerrainTile(15);
	const TileDefinition* BothDeathTerrain = TileSet->FindTerrainTile(16);

	assert(PlayerDamageTerrain != nullptr);
	assert(EnemyDamageTerrain != nullptr);
	assert(BothDamageTerrain != nullptr);
	assert(PlayerDeathTerrain != nullptr);
	assert(EnemyDeathTerrain != nullptr);
	assert(BothDeathTerrain != nullptr);

	assert(PlayerDamageTerrain->Rules[0].Action == TileAction::Damage);
	assert(PlayerDamageTerrain->Rules[0].Target == TileTarget::Player);
	assert(EnemyDamageTerrain->Rules[0].Action == TileAction::Damage);
	assert(EnemyDamageTerrain->Rules[0].Target == TileTarget::Enemy);
	assert(BothDamageTerrain->Rules[0].Action == TileAction::Damage);
	assert(BothDamageTerrain->Rules[0].Target == TileTarget::Both);

	assert(PlayerDeathTerrain->Rules[0].Action == TileAction::InstantDeath);
	assert(PlayerDeathTerrain->Rules[0].Target == TileTarget::Player);
	assert(EnemyDeathTerrain->Rules[0].Action == TileAction::InstantDeath);
	assert(EnemyDeathTerrain->Rules[0].Target == TileTarget::Enemy);
	assert(BothDeathTerrain->Rules[0].Action == TileAction::InstantDeath);
	assert(BothDeathTerrain->Rules[0].Target == TileTarget::Both);
	assert(EmptyTerrain != nullptr);
	assert(SolidTerrain != nullptr);
	assert(CoinTerrain != nullptr);
	assert(QuestionTerrain != nullptr);
	assert(BrickTerrain != nullptr);
	assert(SwitchTerrain != nullptr);
	assert(SwitchOnTerrain != nullptr);
	assert(SwitchOffTerrain != nullptr);
	assert(EmptyTerrain->Collision == CollisionShape::None);
	assert(EmptyTerrain->ImageIndex == 0);
	assert(SolidTerrain->Collision == CollisionShape::Solid);
	assert(SolidTerrain->ImageIndex == 2);
	assert(CoinTerrain->Collision == CollisionShape::None);
	assert(CoinTerrain->ImageIndex == 4);
	assert(CoinTerrain->Rules.size() == 3);
	assert(CoinTerrain->Rules[0].Trigger == TileTrigger::Touch);
	assert(CoinTerrain->Rules[0].Action == TileAction::AddCoin);
	assert(CoinTerrain->Rules[0].Value == 1);
	assert(CoinTerrain->Rules[0].Target == TileTarget::Player);
	assert(CoinTerrain->Rules[1].Action == TileAction::AddScore);
	assert(CoinTerrain->Rules[1].Value == 100);
	assert(CoinTerrain->Rules[2].Action == TileAction::ReplaceTile);
	assert(CoinTerrain->Rules[2].Value == 0);
	assert(QuestionTerrain->Rules.size() == 2);
	assert(QuestionTerrain->Rules[0].Trigger == TileTrigger::HitFromBelow);
	assert(QuestionTerrain->Rules[0].Action == TileAction::SpawnItem);
	assert(QuestionTerrain->Rules[0].Value == static_cast<int>(ItemKind::Healing));
	assert(QuestionTerrain->Rules[1].Action == TileAction::ReplaceTile);
	assert(QuestionTerrain->Rules[1].Value == 2);
	assert(BrickTerrain->Rules.size() == 1);
	assert(BrickTerrain->Rules[0].Action == TileAction::HitBrick);
	assert(BrickTerrain->Rules[0].Value == BrickSystem::Version1RequiredHealth);
	assert(SwitchTerrain->Rules.size() == 1);
	assert(SwitchTerrain->Rules[0].Action == TileAction::ToggleSwitch);
	assert(SwitchTerrain->Rules[0].Value == 0);
	assert(SwitchOnTerrain->SwitchChannel == 0);
	assert(SwitchOnTerrain->SwitchOnTileId == 9);
	assert(SwitchOnTerrain->SwitchOffTileId == 10);
	assert(SwitchOnTerrain->Collision == CollisionShape::Solid);
	assert(SwitchOffTerrain->SwitchChannel == 0);
	assert(SwitchOffTerrain->SwitchOnTileId == 9);
	assert(SwitchOffTerrain->SwitchOffTileId == 10);
	assert(SwitchOffTerrain->Collision == CollisionShape::None);
	Result<TileCatalog> NativeCatalog = TileSet->BuildTerrainCatalog();
	assert(NativeCatalog.IsSuccess());
	assert(NativeCatalog.Value().Find(2) != nullptr);
	assert(NativeCatalog.Value().Find(2)->Collision == CollisionShape::Solid);
	assert(Data.Areas.size() == 2);

	const StageArea* Area = Data.FindArea("main");
	assert(Area != nullptr);
	assert(Area->Width == 24);
	assert(Area->Height == 18);
	assert(Area->TileWidth == 32);
	assert(Area->TileHeight == 32);
	assert(Area->Settings.TimeLimitSeconds == 300);
	assert(Area->Settings.BgmId == "test-bgm");
	assert(Area->Settings.BackgroundId == "test-sky");

	assert(Area->TileLayers.size() == 3);
	const TileLayer* Background = Area->FindTileLayer("background");
	const TileLayer* Terrain = Area->FindTileLayer("terrain");
	const TileLayer* Foreground = Area->FindTileLayer("foreground");
	assert(Background != nullptr);
	assert(Terrain != nullptr);
	assert(Foreground != nullptr);
	assert(Background->Role == TileLayerRole::Visual);
	assert(Terrain->Role == TileLayerRole::Terrain);
	assert(Foreground->Role == TileLayerRole::Visual);
	assert(Background->TileSetId == "native-test");
	assert(Terrain->TileSetId == "native-test");
	assert(Foreground->TileSetId == "native-test");
	assert(Background->Metadata.ZOrder == -10);
	assert(Terrain->Metadata.ZOrder == 0);
	assert(Foreground->Metadata.ZOrder == 20);
	assert(*Background->Map.TryGet({0, 0}) == 1);
	assert(*Terrain->Map.TryGet({1, 3}) == 5);
	assert(*Terrain->Map.TryGet({2, 3}) == 6);
	assert(*Terrain->Map.TryGet({4, 3}) == 8);
	assert(*Terrain->Map.TryGet({2, 4}) == 4);
	assert(*Terrain->Map.TryGet({6, 4}) == 9);
	assert(*Terrain->Map.TryGet({23, 11}) == 2);
	// 踏みつけ確認のため、旧cliff fixtureの上段ブロックは撤去済み。
	for (int Column = 0; Column < 8; ++Column) {
		assert(*Terrain->Map.TryGet({Column, 2}) == 0);
	}
	assert(*Foreground->Map.TryGet({7, 4}) == 3);

	const ObjectLayer* Objects = Area->FindObjectLayer("objects");
	assert(Objects != nullptr);
	assert(Objects->Objects.size() == 10);
	assert(Objects->Objects[0].TypeId == "PlayerSpawn");
	assert(NearlyEqual(Objects->Objects[0].Position.X, 32.0f));
	assert(NearlyEqual(Objects->Objects[0].Position.Y, 128.0f));

	assert(Objects->Objects[1].Id == "enemy-cliff-turn");
	assert(Objects->Objects[1].TypeId == "WalkingEnemy");
	assert(NearlyEqual(Objects->Objects[1].Position.X, 48.0f));
	assert(NearlyEqual(Objects->Objects[1].Position.Y, 32.0f));

	assert(Objects->Objects[2].Id == "enemy-cliff-fall");
	assert(Objects->Objects[2].TypeId == "WalkingEnemy");
	assert(NearlyEqual(Objects->Objects[2].Position.X, 160.0f));
	assert(NearlyEqual(Objects->Objects[2].Position.Y, 32.0f));

	assert(Objects->Objects[3].Id == "enemy-meet-left");
	assert(Objects->Objects[3].TypeId == "WalkingEnemy");
	assert(NearlyEqual(Objects->Objects[3].Position.X, 80.0f));
	assert(NearlyEqual(Objects->Objects[3].Position.Y, 128.0f));

	assert(Objects->Objects[4].Id == "enemy-meet-right");
	assert(Objects->Objects[4].TypeId == "WalkingEnemy");
	assert(NearlyEqual(Objects->Objects[4].Position.X, 144.0f));
	assert(NearlyEqual(Objects->Objects[4].Position.Y, 128.0f));

	assert(Objects->Objects[5].TypeId == "HorizontalLift");
	assert(NearlyEqual(Objects->Objects[5].Position.X, 96.0f));
	assert(NearlyEqual(Objects->Objects[5].Position.Y, 96.0f));

	std::string Direction;
	int Variant = 0;
	float Speed = 0.0f;
	bool Aggressive = true;

	assert(Objects->Objects[1].Properties.at("direction").TryGetString(Direction));
	assert(Direction == "right");
	assert(Objects->Objects[1].Properties.at("variant").TryGetInteger(Variant));
	assert(Variant == 2);
	assert(Objects->Objects[1].Properties.at("speed").TryGetFloat(Speed));
	assert(NearlyEqual(Speed, 2.0f));

	assert(Objects->Objects[2].Properties.at("direction").TryGetString(Direction));
	assert(Direction == "right");
	assert(Objects->Objects[2].Properties.at("variant").TryGetInteger(Variant));
	assert(Variant == 1);
	assert(Objects->Objects[2].Properties.at("speed").TryGetFloat(Speed));
	assert(NearlyEqual(Speed, 2.0f));
	assert(Objects->Objects[2].Properties.at("aggressive").TryGetBoolean(Aggressive));
	assert(!Aggressive);

	float LiftRange = 0.0f;
	float LiftSpeed = 0.0f;
	assert(Objects->Objects[5].Properties.at("range").TryGetFloat(LiftRange));
	assert(NearlyEqual(LiftRange, 192.0f));
	assert(Objects->Objects[5].Properties.at("speed").TryGetFloat(LiftSpeed));
	assert(NearlyEqual(LiftSpeed, 2.0f));

	WorldPosition PathDelta;
	assert(Objects->Objects[5].Properties.at("pathDelta").TryGetVector2(PathDelta));
	assert(NearlyEqual(PathDelta.X, 192.0f));
	assert(NearlyEqual(PathDelta.Y, 0.0f));

	assert(Area->RegionLayers.empty());

	assert(Area->Transitions.size() == 1);
	const StageTransition& Pipe = Area->Transitions[0];
	assert(Pipe.Id == "pipe-main-sub");
	assert(Pipe.TargetStageId.empty());
	assert(Pipe.TargetAreaId == "sub");
	assert(Pipe.Entry.Shape == StageRegionShape::Point);
	assert(NearlyEqual(Pipe.Entry.Position.X, 128.0f));
	assert(NearlyEqual(Pipe.Entry.Position.Y, 128.0f));
	assert(Pipe.EnterDirection == StageDirection::Down);
	assert(Pipe.ExitDirection == StageDirection::Up);
	assert(NearlyEqual(Pipe.ExitPosition.X, 32.0f));
	assert(NearlyEqual(Pipe.ExitPosition.Y, 128.0f));

	const StageArea* Sub = Data.FindArea("sub");
	assert(Sub != nullptr);
	assert(Sub->Width == 8);
	assert(Sub->Height == 6);
	assert(Sub->TerrainLayer() != nullptr);
	assert(*Sub->TerrainLayer()->Map.TryGet({0, 5}) == 2);
	assert(Sub->ObjectLayers.empty());
	assert(Sub->Transitions.empty());

	const RegionLayer* Events = Sub->FindRegionLayer("events");
	assert(Events != nullptr);
	assert(Events->Regions.size() == 1);
	const StageRegion& Goal = Events->Regions[0];
	assert(Goal.Id == "goal-sub");
	assert(Goal.TypeId == "Goal");
	assert(Goal.Geometry.Shape == StageRegionShape::Rectangle);
	assert(NearlyEqual(Goal.Geometry.Position.X, 192.0f));
	assert(NearlyEqual(Goal.Geometry.Position.Y, 128.0f));
	assert(NearlyEqual(Goal.Geometry.Size.X, 32.0f));
	assert(NearlyEqual(Goal.Geometry.Size.Y, 32.0f));
	std::string GoalKind;
	assert(Goal.Properties.at("goalKind").TryGetString(GoalKind));
	assert(GoalKind == "normal");
}

void TestNativeStageCharacterControllerUsesTerrainSemantics() {
	Result<StageData> Loaded =
		NativeStageDataLoader::Load("dat/stage/native-test/stage.json");
	assert(Loaded.IsSuccess());

	const StageData& Data = Loaded.Value();
	const StageArea* Area = Data.FindArea(Data.StartAreaId);
	assert(Area != nullptr);
	const TileLayer* Terrain = Area->TerrainLayer();
	assert(Terrain != nullptr);
	const TileSetDefinition* TileSet = Data.FindTileSet(Terrain->TileSetId);
	assert(TileSet != nullptr);

	Result<TileCatalog> Catalog = TileSet->BuildTerrainCatalog();
	assert(Catalog.IsSuccess());
	assert(Catalog.Value().Find(2) != nullptr);
	assert(Catalog.Value().Find(2)->Collision == CollisionShape::Solid);

	const ObjectSpawn* Spawn = nullptr;
	for (const ObjectLayer& Layer : Area->ObjectLayers) {
		for (const ObjectSpawn& Object : Layer.Objects) {
			if (Object.TypeId == "PlayerSpawn") {
				assert(Spawn == nullptr);
				Spawn = &Object;
			}
		}
	}
	assert(Spawn != nullptr);

	CharacterBody Body;
	Body.Position = Spawn->Position;
	Body.Grounded = true;
	CharacterController Player(Body);

	for (int Frame = 0; Frame < 4; ++Frame) {
		Player.Step(1.0f, false, Terrain->Map, Catalog.Value());
	}
	assert(Player.Body().Position.X > Spawn->Position.X);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 128.0f));

	// #37 fixtureではPlayerSpawnの真上に? block(id=5)がある。
	// Spawn位置でZを押すと上昇するのではなく、正しくHitFromBelowになる。
	CharacterBody QuestionBody;
	QuestionBody.Position = Spawn->Position;
	QuestionBody.Grounded = true;
	CharacterController QuestionPlayer(QuestionBody);
	QuestionPlayer.Step(0.0f, true, Terrain->Map, Catalog.Value());

	bool HitQuestionFromBelow = false;
	for (const TileInteraction& Interaction : QuestionPlayer.Interactions()) {
		if (Interaction.Trigger == TileTrigger::HitFromBelow &&
			Interaction.Position.Column == 1 &&
			Interaction.Position.Row == 3 &&
			Interaction.TileId == 5) {
			HitQuestionFromBelow = true;
			break;
		}
	}
	assert(HitQuestionFromBelow);

	// CharacterController自体のジャンプ確認はgameplay fixtureから分離する。
	// stage.json側の配置変更でphysics smoke testの意味を変えない。
	TileMap JumpMap = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{2, 2, 2}
	});
	CharacterBody JumpBody;
	JumpBody.Position = {32.0f, 32.0f};
	JumpBody.Grounded = true;
	CharacterController JumpPlayer(JumpBody);
	JumpPlayer.Step(0.0f, true, JumpMap, Catalog.Value());
	assert(JumpPlayer.Body().Position.Y < 32.0f);
	assert(!JumpPlayer.Body().Grounded);
}

void TestNativeStageTileRulesApplyFromJson() {
	Result<StageData> Loaded =
		NativeStageDataLoader::Load("dat/stage/native-test/stage.json");
	assert(Loaded.IsSuccess());

	StageData Data = std::move(Loaded.Value());
	StageArea* Area = Data.FindArea(Data.StartAreaId);
	assert(Area != nullptr);
	TileLayer* Terrain = Area->TerrainLayer();
	assert(Terrain != nullptr);
	const TileSetDefinition* TileSet = Data.FindTileSet(Terrain->TileSetId);
	assert(TileSet != nullptr);

	Result<TileCatalog> Catalog = TileSet->BuildTerrainCatalog();
	assert(Catalog.IsSuccess());

	TileRuntimeMap Runtime(Terrain->Map);
	TileInteraction Touch;
	Touch.Trigger = TileTrigger::Touch;
	Touch.Position = {2, 4};
	Touch.TileId = 4;
	Touch.Actor = TileActor::Player;

	const TileBehaviorResult Result =
		TileBehaviorSystem::Apply(
			Touch, Terrain->Map, Catalog.Value(), Runtime);
	assert(Result.Handled);
	assert(Result.Effects.size() == 2);
	assert(Result.Effects[0].Type == TileEffectType::AddCoin);
	assert(Result.Effects[0].Value == 1);
	assert(Result.Effects[1].Type == TileEffectType::AddScore);
	assert(Result.Effects[1].Value == 100);
	assert(*Terrain->Map.TryGet({2, 4}) == 0);
}

void TestNativeStageGameplayAdaptersFromJson() {
	Result<StageData> Loaded =
		NativeStageDataLoader::Load("dat/stage/native-test/stage.json");
	assert(Loaded.IsSuccess());

	StageData Data = std::move(Loaded.Value());
	StageArea* Area = Data.FindArea("main");
	assert(Area != nullptr);
	TileLayer* Terrain = Area->TerrainLayer();
	assert(Terrain != nullptr);
	const TileSetDefinition* TileSet = Data.FindTileSet(Terrain->TileSetId);
	assert(TileSet != nullptr);

	Result<TileCatalog> CatalogResult = TileSet->BuildTerrainCatalog();
	assert(CatalogResult.IsSuccess());
	TileCatalog Catalog = std::move(CatalogResult.Value());
	TileRuntimeMap Runtime(Terrain->Map);

	// ? block -> Healing SpawnItem + Used blockへの置換。
	TileInteraction QuestionHit;
	QuestionHit.Trigger = TileTrigger::HitFromBelow;
	QuestionHit.Position = {1, 3};
	QuestionHit.TileId = 5;
	QuestionHit.Actor = TileActor::Player;
	const TileBehaviorResult QuestionResult =
		TileBehaviorSystem::Apply(
			QuestionHit, Terrain->Map, Catalog, Runtime);
	assert(QuestionResult.Handled);
	assert(QuestionResult.Effects.size() == 1);
	assert(QuestionResult.Effects[0].Type == TileEffectType::SpawnItem);
	assert(QuestionResult.Effects[0].Value ==
		static_cast<int>(ItemKind::Healing));
	assert(*Terrain->Map.TryGet({1, 3}) == 2);

	ItemSystem Items;
	Items.ConsumeTileEffects(
		QuestionResult.Effects,
		Terrain->Map.TileWidth(),
		Terrain->Map.TileHeight());
	assert(Items.Items().size() == 1);
	assert(Items.Items()[0].Kind == ItemKind::Healing);

	std::vector<TileEffect> ItemEffects;
	for (int Frame = 0; Frame < 64 && ItemEffects.empty(); ++Frame) {
		ItemEffects = Items.Update();
	}
	assert(ItemEffects.size() == 2);
	assert(ItemEffects[0].Type == TileEffectType::AddHealth);
	assert(ItemEffects[0].Value == 1);
	assert(ItemEffects[1].Type == TileEffectType::AddScore);
	assert(ItemEffects[1].Value == 1000);

	// Brick: HP4ならbump、HP5ならbreak。
	TileInteraction BrickHit;
	BrickHit.Trigger = TileTrigger::HitFromBelow;
	BrickHit.Position = {2, 3};
	BrickHit.TileId = 6;
	BrickHit.Actor = TileActor::Player;
	const TileBehaviorResult BrickResult =
		TileBehaviorSystem::Apply(
			BrickHit, Terrain->Map, Catalog, Runtime);
	assert(BrickResult.Effects.size() == 1);
	assert(BrickResult.Effects[0].Type == TileEffectType::BrickHit);

	BrickSystem Bricks;
	GameStateSnapshot State;
	State.Health = 4;
	Bricks.ConsumeTileEffects(BrickResult.Effects, State);
	assert(Bricks.TryGet({2, 3}) != nullptr);
	assert(Bricks.TryGet({2, 3})->Phase == BrickPhase::Bumping);
	for (int Frame = 0; Frame < BrickSystem::Version1BumpFrames; ++Frame) {
		Bricks.Update(Terrain->Map, 32, 32, 9999.0f);
	}
	assert(*Terrain->Map.TryGet({2, 3}) == 6);

	State.Health = 5;
	Bricks.ConsumeTileEffects(BrickResult.Effects, State);
	assert(Bricks.TryGet({2, 3}) != nullptr);
	assert(Bricks.TryGet({2, 3})->Phase == BrickPhase::Breaking);
	std::vector<TileEffect> BrickEffects;
	for (int Frame = 0;
		Frame < BrickSystem::Version1BreakFrames && BrickEffects.empty();
		++Frame) {
		BrickEffects = Bricks.Update(
			Terrain->Map, 32, 32, 9999.0f);
	}
	assert(*Terrain->Map.TryGet({2, 3}) == 0);
	assert(BrickEffects.size() == 2);
	assert(BrickEffects[0].Type == TileEffectType::AddScore);
	assert(BrickEffects[1].Type == TileEffectType::TileBroken);

	// Switch button -> channel 0 OFF -> bound block 9 -> 10。
	TileInteraction SwitchHit;
	SwitchHit.Trigger = TileTrigger::HitFromBelow;
	SwitchHit.Position = {4, 3};
	SwitchHit.TileId = 8;
	SwitchHit.Actor = TileActor::Player;
	const TileBehaviorResult SwitchResult =
		TileBehaviorSystem::Apply(
			SwitchHit, Terrain->Map, Catalog, Runtime);
	assert(SwitchResult.Effects.size() == 1);
	assert(SwitchResult.Effects[0].Type == TileEffectType::ToggleSwitch);
	assert(SwitchResult.Effects[0].Value == 0);

	WorldState World;
	World.Reset(1, true);
	assert(World.GetSwitch(0));
	const WorldStateUpdate WorldUpdate =
		World.ApplyEffects(
			SwitchResult.Effects, Terrain->Map, Catalog);
	assert(!World.GetSwitch(0));
	assert(*Terrain->Map.TryGet({6, 4}) == 10);
	assert(WorldUpdate.ChangedTiles.size() == 1);
}

void TestNativeStageDataValidationRejectsInvalidSwitchBinding() {
	TileSetDefinition TileSet;
	TileSet.Id = "switch";
	TileSet.ImageFile = "dummy.png";
	TileSet.Columns = 1;
	TileSet.Rows = 1;

	TileDefinition Empty;
	Empty.Id = 0;
	Empty.ImageIndex = 0;
	TileSet.TerrainTiles.push_back(Empty);

	TileDefinition Bound;
	Bound.Id = 1;
	Bound.ImageIndex = 0;
	Bound.SwitchChannel = 0;
	Bound.SwitchOnTileId = 1;
	Bound.SwitchOffTileId = 99;
	TileSet.TerrainTiles.push_back(Bound);

	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.TileSetId = "switch";
	Terrain.Map = MakeMap({{1}});

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {Terrain};

	StageData Data;
	Data.Id = "bad-switch";
	Data.StartAreaId = "main";
	Data.TileSets = {TileSet};
	Data.Areas = {Area};

	Result<bool> Validation = ValidateStageData(Data);
	assert(Validation.IsFailure());
	assert(
		Validation.Error().find("undefined tile id") != std::string::npos);
}

void TestNativeObjectRuntimeBuildsTypeSpecificHitBounds() {
	Result<StageData> Loaded =
		NativeStageDataLoader::Load("dat/stage/native-test/stage.json");
	assert(Loaded.IsSuccess());

	const StageArea* Area = Loaded.Value().FindArea("main");
	assert(Area != nullptr);

	NativeObjectSystem Objects;
	Result<bool> Reset = Objects.Reset(*Area);
	assert(Reset.IsSuccess());
	assert(Objects.Objects().size() == 9);

	const NativeObjectRuntime* TurnEnemy =
		Objects.Find("enemy-cliff-turn");
	const NativeObjectRuntime* FallEnemy =
		Objects.Find("enemy-cliff-fall");
	const NativeObjectRuntime* Lift = Objects.Find("lift-1");
	const NativeObjectRuntime* Carrot = Objects.Find("carrot-v1");
	const NativeObjectRuntime* BallFall = Objects.Find("ball-slime-fall");
	const NativeObjectRuntime* BallTurn = Objects.Find("ball-slime-turn");
	assert(TurnEnemy != nullptr);
	assert(FallEnemy != nullptr);
	assert(Lift != nullptr);
	assert(Carrot != nullptr);
	assert(BallFall != nullptr);
	assert(BallTurn != nullptr);

	assert(FallEnemy->TypeId == "WalkingEnemy");
	assert(NearlyEqual(FallEnemy->Position.X, 160.0f));
	assert(NearlyEqual(FallEnemy->Position.Y, 32.0f));
	assert(NearlyEqual(FallEnemy->HitboxOffset.X, 8.0f));
	assert(NearlyEqual(FallEnemy->HitboxOffset.Y, 1.0f));
	assert(NearlyEqual(FallEnemy->HitboxSize.X, 16.0f));
	assert(NearlyEqual(FallEnemy->HitboxSize.Y, 31.0f));
	assert(FallEnemy->ContactDamage == 1);
	assert(FallEnemy->Direction == 1);
	assert(FallEnemy->Variant == 1);
	assert(NearlyEqual(FallEnemy->MoveSpeed, 2.0f));
	assert(NearlyEqual(FallEnemy->Gravity, 0.5f));
	assert(NearlyEqual(FallEnemy->MaxFallSpeed, 12.0f));

	const ObjectHitBounds EnemyBounds = FallEnemy->HitBounds();
	assert(NearlyEqual(EnemyBounds.Position.X, 168.0f));
	assert(NearlyEqual(EnemyBounds.Position.Y, 33.0f));
	assert(NearlyEqual(EnemyBounds.Size.X, 16.0f));
	assert(NearlyEqual(EnemyBounds.Size.Y, 31.0f));

	assert(TurnEnemy->TypeId == "WalkingEnemy");
	assert(TurnEnemy->Direction == 1);
	assert(TurnEnemy->Variant == 2);
	assert(NearlyEqual(TurnEnemy->Position.X, 48.0f));
	assert(NearlyEqual(TurnEnemy->Position.Y, 32.0f));

	assert(Lift->TypeId == "HorizontalLift");
	assert(NearlyEqual(Lift->HitboxOffset.X, -6.0f));
	assert(NearlyEqual(Lift->HitboxOffset.Y, 11.0f));
	assert(NearlyEqual(Lift->HitboxSize.X, 44.0f));
	assert(NearlyEqual(Lift->HitboxSize.Y, 10.0f));
	assert(Lift->ContactDamage == 0);

	assert(Carrot->TypeId == "CarrotMan");
	assert(!Carrot->ContactEnabled);
	assert(!Carrot->Stompable);
	assert(Carrot->BehaviorState == 0);

	assert(BallFall->TypeId == "BallSlime");
	assert(BallFall->Variant == 1);
	assert(BallFall->Direction == -1);
	assert(NearlyEqual(BallFall->HitboxOffset.X, 2.0f));
	assert(NearlyEqual(BallFall->HitboxOffset.Y, 1.0f));
	assert(NearlyEqual(BallFall->HitboxSize.X, 28.0f));
	assert(NearlyEqual(BallFall->HitboxSize.Y, 31.0f));
	assert(BallFall->BehaviorState == 0);
	assert(BallFall->Stompable);
	assert(BallFall->ContactDamage == 1);

	assert(BallTurn->TypeId == "BallSlime");
	assert(BallTurn->Variant == 2);
}

void TestNativeObjectRuntimeUsesPlayerCentralTouchBounds() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn Enemy;
	Enemy.Id = "enemy";
	Enemy.TypeId = "WalkingEnemy";
	Enemy.Position = {100.0f, 100.0f};
	Layer.Objects.push_back(Enemy);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	CharacterBody Body;
	Body.Position = {68.0f, 100.0f};
	CharacterController Player(Body);
	CharacterTouchBounds Touch = Player.TouchBounds();

	// 見た目32pxの右端はenemy spriteへ触れているが、
	// center 16x32はenemy hitboxへ届いていない。
	assert(Body.Position.X + Body.Width == 100.0f);
	std::vector<NativeObjectContact> Contacts =
		Objects.FindContacts(
			{Touch.Left, Touch.Top},
			{Touch.Right - Touch.Left, Touch.Bottom - Touch.Top});
	assert(Contacts.empty());

	Player.Reposition({86.0f, 100.0f});
	Touch = Player.TouchBounds();
	Contacts = Objects.FindContacts(
		{Touch.Left, Touch.Top},
		{Touch.Right - Touch.Left, Touch.Bottom - Touch.Top});
	assert(Contacts.size() == 1);
	assert(Contacts[0].ObjectId == "enemy");
	assert(Contacts[0].TypeId == "WalkingEnemy");
	assert(Contacts[0].ContactDamage == 1);
}


void TestNativeWalkingEnemyClassifiesStompSeparatelyFromDamage() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	ObjectSpawn Enemy;
	Enemy.Id = "enemy";
	Enemy.TypeId = "WalkingEnemy";
	Enemy.Position = {100.0f, 100.0f};
	Layer.Objects.push_back(Enemy);
	Area.ObjectLayers.push_back(Layer);
	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const std::vector<NativeObjectContact> Stomp = Objects.FindContacts(
		{108.0f, 70.0f}, {16.0f, 32.0f}, 4.0f);
	assert(Stomp.size() == 1);
	assert(Stomp[0].Kind == NativeObjectContactKind::Stomp);
	assert(Stomp[0].ContactDamage == 0);

	const std::vector<NativeObjectContact> Rising = Objects.FindContacts(
		{108.0f, 70.0f}, {16.0f, 32.0f}, -4.0f);
	assert(Rising.size() == 1);
	assert(Rising[0].Kind == NativeObjectContactKind::Touch);
	assert(Rising[0].ContactDamage == 1);

	const std::vector<NativeObjectContact> Side = Objects.FindContacts(
		{96.0f, 100.0f}, {16.0f, 32.0f}, 4.0f);
	assert(Side.size() == 1);
	assert(Side[0].Kind == NativeObjectContactKind::Touch);
	assert(Side[0].ContactDamage == 1);

	assert(Objects.Deactivate("enemy"));
	assert(!Objects.Deactivate("enemy"));
	assert(Objects.FindContacts({108.0f, 70.0f}, {16.0f, 32.0f}, 4.0f).empty());
}

void TestNativeWalkingEnemyLifecycleUsesCameraAndKeepsDefeatedState() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	ObjectSpawn EnemySpawn;
	EnemySpawn.Id = "walker";
	EnemySpawn.TypeId = "WalkingEnemy";
	EnemySpawn.Position = {640.0f, 100.0f};
	EnemySpawn.Properties["direction"] =
		StagePropertyValue::String("right");
	EnemySpawn.Properties["variant"] =
		StagePropertyValue::Integer(1);
	EnemySpawn.Properties["speed"] =
		StagePropertyValue::Float(2.0f);
	Layer.Objects.push_back(EnemySpawn);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const WorldPosition ViewSize = {512.0f, 320.0f};

	// 初期Cameraから十分離れたEnemyはDormantへ入り、spawn位置へ戻る。
	Objects.UpdateLifecycle({0.0f, 0.0f}, ViewSize);
	const NativeObjectRuntime* Enemy = Objects.Find("walker");
	assert(Enemy != nullptr);
	assert(Enemy->LifeState == ObjectLifeState::Dormant);
	assert(!Enemy->Active);
	assert(Enemy->RespawnArmed);
	assert(NearlyEqual(Enemy->Position.X, 640.0f));
	assert(Enemy->Direction == 1);

	// spawn地点がActivation範囲へ入るとActive化する。
	Objects.UpdateLifecycle({200.0f, 0.0f}, ViewSize);
	Enemy = Objects.Find("walker");
	assert(Enemy->LifeState == ObjectLifeState::Active);
	assert(Enemy->Active);
	assert(!Enemy->RespawnArmed);

	// Active中に現在位置がCameraから十分離れるとDormantへ戻る。
	NativeObjectRuntime* MutableEnemy = Objects.Find("walker");
	assert(MutableEnemy != nullptr);
	MutableEnemy->Position = {900.0f, 100.0f};
	MutableEnemy->Direction = -1;
	Objects.UpdateLifecycle({200.0f, 0.0f}, ViewSize);
	Enemy = Objects.Find("walker");
	assert(Enemy->LifeState == ObjectLifeState::Dormant);
	assert(!Enemy->Active);
	assert(NearlyEqual(Enemy->Position.X, 640.0f));
	assert(Enemy->Direction == 1);

	// spawn地点がまだCamera付近にある間は即respawnしない。
	Objects.UpdateLifecycle({200.0f, 0.0f}, ViewSize);
	assert(Objects.Find("walker")->LifeState == ObjectLifeState::Dormant);

	// 一度spawn地点を画面外へ出してから戻すと再度Active化する。
	Objects.UpdateLifecycle({0.0f, 0.0f}, ViewSize);
	assert(Objects.Find("walker")->RespawnArmed);
	Objects.UpdateLifecycle({200.0f, 0.0f}, ViewSize);
	assert(Objects.Find("walker")->LifeState == ObjectLifeState::Active);

	// 倒されたEnemyはCameraを往復しても復活しない。
	assert(Objects.Deactivate("walker"));
	assert(Objects.Find("walker")->LifeState == ObjectLifeState::Defeated);
	Objects.UpdateLifecycle({0.0f, 0.0f}, ViewSize);
	Objects.UpdateLifecycle({200.0f, 0.0f}, ViewSize);
	assert(Objects.Find("walker")->LifeState == ObjectLifeState::Defeated);
	assert(!Objects.Find("walker")->Active);
}

void TestNativeCarrotManWaitsEmergesAndStartsWalking() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1, 1, 1, 1}
	});
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());
	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn Carrot;
	Carrot.Id = "carrot";
	Carrot.TypeId = "CarrotMan";
	Carrot.Position = {64.0f, 32.0f};
	Layer.Objects.push_back(Carrot);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const NativeObjectRuntime* Enemy = Objects.Find("carrot");
	assert(Enemy != nullptr);
	assert(Enemy->BehaviorState == 0);
	assert(!Enemy->ContactEnabled);
	assert(!Enemy->Stompable);

	// 96pxより遠い間は待機timerを進めない。
	for (int Frame = 0; Frame < 40; ++Frame) {
		Objects.Update(Map, Catalog, {200.0f, 32.0f});
	}
	Enemy = Objects.Find("carrot");
	assert(Enemy->BehaviorState == 0);
	assert(Enemy->BehaviorTimer == 0);
	assert(NearlyEqual(Enemy->Position.Y, 32.0f));

	// V1同様、3tile以内に31frameいると上へ飛び出す。
	for (int Frame = 0; Frame < 31; ++Frame) {
		Objects.Update(Map, Catalog, {128.0f, 32.0f});
	}
	Enemy = Objects.Find("carrot");
	assert(Enemy->BehaviorState == 1);
	assert(Enemy->ContactEnabled);
	assert(Enemy->Stompable);
	assert(NearlyEqual(Enemy->Velocity.Y, -10.0f));

	// 飛び出し中は上昇し、着地後はPlayer側を向いて通常歩行へ移る。
	Objects.Update(Map, Catalog, {128.0f, 32.0f});
	assert(Objects.Find("carrot")->Position.Y < 32.0f);

	for (int Frame = 0;
		Frame < 120 && Objects.Find("carrot")->BehaviorState != 2;
		++Frame) {
		Objects.Update(Map, Catalog, {128.0f, 32.0f});
	}

	Enemy = Objects.Find("carrot");
	assert(Enemy->BehaviorState == 2);
	assert(Enemy->Grounded);
	assert(Enemy->Direction == 1);

	const float BeforeX = Enemy->Position.X;
	Objects.Update(Map, Catalog, {128.0f, 32.0f});
	assert(Objects.Find("carrot")->Position.X > BeforeX);

	// Dormant resetでは再び地中待機へ戻る。
	NativeObjectRuntime* Mutable = Objects.Find("carrot");
	assert(Mutable != nullptr);
	Mutable->Position = {700.0f, 32.0f};
	Objects.UpdateLifecycle({0.0f, 0.0f}, {512.0f, 320.0f});
	Enemy = Objects.Find("carrot");
	assert(Enemy->LifeState == ObjectLifeState::Dormant);
	assert(Enemy->BehaviorState == 0);
	assert(!Enemy->ContactEnabled);
	assert(!Enemy->Stompable);
}

void TestNativeBallSlimeTransitionsWalkingShellKickAndRecovery() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1, 1, 1, 1}
	});
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());
	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	ObjectSpawn Spawn;
	Spawn.Id = "ball";
	Spawn.TypeId = "BallSlime";
	Spawn.Position = {64.0f, 32.0f};
	Spawn.Properties["direction"] =
		StagePropertyValue::String("right");
	Spawn.Properties["variant"] =
		StagePropertyValue::Integer(1);
	Layer.Objects.push_back(Spawn);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const NativeObjectRuntime* Ball = Objects.Find("ball");
	assert(Ball != nullptr);
	assert(Ball->BehaviorState == 0);
	assert(Ball->Stompable);
	assert(Ball->ContactDamage == 1);

	// 通常状態を踏むと倒れず、停止したShell状態へ移る。
	assert(Objects.HandleStomp("ball"));
	Ball = Objects.Find("ball");
	assert(Ball->Active);
	assert(Ball->LifeState == ObjectLifeState::Active);
	assert(Ball->BehaviorState == 1);
	assert(!Ball->Stompable);
	assert(Ball->ContactDamage == 0);
	assert(NearlyEqual(Ball->Velocity.X, 0.0f));

	// Shellへ横から触ると、Playerから離れる左向きへKickされる。
	// 踏みつけ直後の誤KickはSandbox側のHSP式座標補正で防ぐ。
	assert(Objects.HandlePlayerTouch("ball", 120.0f));
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 2);
	assert(Ball->Direction == -1);
	assert(NearlyEqual(Ball->MoveSpeed, 8.0f));
	assert(Ball->ContactDamage == 0);

	for (int Frame = 0; Frame < 29; ++Frame) {
		Objects.Update(Map, Catalog);
	}
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 2);
	assert(Ball->BehaviorTimer == 29);
	assert(!Ball->Stompable);
	assert(Ball->ContactDamage == 0);

	Objects.Update(Map, Catalog);
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorTimer == 30);
	assert(Ball->Stompable);
	assert(Ball->ContactDamage == 1);

	// 高速甲羅を踏むと再び停止Shellへ戻る。
	assert(Objects.HandleStomp("ball"));
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 1);
	assert(Ball->BehaviorTimer == 0);
	assert(Ball->ContactDamage == 0);

	// 5秒で復活予告の小ジャンプ、7秒で通常歩行へ戻る。
	for (int Frame = 0; Frame < 300; ++Frame) {
		Objects.Update(Map, Catalog);
	}
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 1);
	assert(Ball->BehaviorTimer == 300);
	assert(Ball->Stompable);
	assert(Ball->Velocity.Y < 0.0f);

	for (int Frame = 0; Frame < 120; ++Frame) {
		Objects.Update(Map, Catalog);
	}
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 0);
	assert(Ball->BehaviorTimer == 0);
	assert(Ball->Stompable);
	assert(Ball->ContactDamage == 1);
	assert(NearlyEqual(Ball->MoveSpeed, 2.0f));
}

void TestNativeBallSlimeVariant2TurnsAtCliffOnlyWhileWalking() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 0, 0},
		{0, 0, 0, 0, 0}
	});
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());
	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	ObjectSpawn Spawn;
	Spawn.Id = "ball";
	Spawn.TypeId = "BallSlime";
	Spawn.Position = {64.0f, 32.0f};
	Spawn.Properties["direction"] =
		StagePropertyValue::String("right");
	Spawn.Properties["variant"] =
		StagePropertyValue::Integer(2);
	Layer.Objects.push_back(Spawn);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());
	for (int Frame = 0; Frame < 10; ++Frame) {
		Objects.Update(Map, Catalog);
	}

	const NativeObjectRuntime* Ball = Objects.Find("ball");
	assert(Ball != nullptr);
	assert(Ball->BehaviorState == 0);
	assert(Ball->Grounded);
	assert(Ball->Direction == -1);

	// Kick後はvariant=2でも崖回避を行わず、甲羅として落下できる。
	assert(Objects.HandleStomp("ball"));
	assert(Objects.HandlePlayerTouch("ball", 0.0f));
	for (int Frame = 0; Frame < 10; ++Frame) {
		Objects.Update(Map, Catalog);
	}
	Ball = Objects.Find("ball");
	assert(Ball->BehaviorState == 2);
	assert(!Ball->Grounded);
	assert(Ball->Position.Y > 32.0f);
}

void TestNativeKickedBallSlimeDefeatsOtherEnemyAndLifecycleResetsShell() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1, 1}
	});
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());
	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn BallSpawn;
	BallSpawn.Id = "ball";
	BallSpawn.TypeId = "BallSlime";
	BallSpawn.Position = {32.0f, 32.0f};
	BallSpawn.Properties["direction"] =
		StagePropertyValue::String("right");
	Layer.Objects.push_back(BallSpawn);

	ObjectSpawn Walker;
	Walker.Id = "walker";
	Walker.TypeId = "WalkingEnemy";
	Walker.Position = {60.0f, 32.0f};
	Walker.Properties["direction"] =
		StagePropertyValue::String("left");
	Layer.Objects.push_back(Walker);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());
	assert(Objects.HandleStomp("ball"));
	assert(Objects.HandlePlayerTouch("ball", 0.0f));

	Objects.Update(Map, Catalog);
	const NativeObjectRuntime* WalkerRuntime = Objects.Find("walker");
	assert(WalkerRuntime != nullptr);
	assert(!WalkerRuntime->Active);
	assert(WalkerRuntime->LifeState == ObjectLifeState::Defeated);

	// Camera外へ出たBallSlimeはspawnへ戻り、通常歩行状態へresetされる。
	NativeObjectRuntime* Ball = Objects.Find("ball");
	assert(Ball != nullptr);
	Ball->Position = {700.0f, 32.0f};
	Objects.UpdateLifecycle({0.0f, 0.0f}, {512.0f, 320.0f});
	Ball = Objects.Find("ball");
	assert(Ball->LifeState == ObjectLifeState::Dormant);
	assert(Ball->BehaviorState == 0);
	assert(Ball->BehaviorTimer == 0);
	assert(Ball->ContactDamage == 1);
	assert(Ball->Stompable);
}

void TestStompRepositionMatchesHspOnePixelSeparation() {
	CharacterBody Body;
	Body.Position = {100.0f, 73.0f};
	Body.Velocity = {2.0f, 4.0f};
	CharacterController Player(Body);

	NativeObjectRuntime Enemy;
	Enemy.Position = {100.0f, 100.0f};
	Enemy.HitboxOffset = {8.0f, 1.0f};
	Enemy.HitboxSize = {16.0f, 31.0f};

	const CharacterBody Before = Player.Body();
	Player.Reposition({
		Before.Position.X,
		Enemy.Position.Y - Before.Height
	});
	Player.SetVelocity({Before.Velocity.X, -6.0f});

	const CharacterBody& After = Player.Body();
	assert(NearlyEqual(After.Position.Y, 68.0f));
	assert(NearlyEqual(
		After.Position.Y + After.Height,
		Enemy.Position.Y));
	assert(After.Position.Y + After.Height <
		Enemy.HitBounds().Position.Y);
	assert(NearlyEqual(After.Velocity.X, 2.0f));
	assert(NearlyEqual(After.Velocity.Y, -6.0f));
}

void TestCharacterSetVelocityKeepsInternalVelocityInSync() {
	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	CharacterMotion Motion;
	Motion.Gravity = 0.0f;
	Motion.MoveSpeed = 0.0f;
	CharacterController Player(Body, Motion);
	Player.SetVelocity({0.0f, -6.0f});
	assert(NearlyEqual(Player.Body().Velocity.Y, -6.0f));
	TileMap Map = MakeMap({{0, 0, 0}, {0, 0, 0}, {0, 0, 0}});
	TileCatalog Catalog;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());
	Player.StepWithoutInput(Map, Catalog);
	assert(Player.Body().Position.Y < 32.0f);
	assert(NearlyEqual(Player.Body().Velocity.Y, -6.0f));
}

void TestNativeObjectContactComposesWithDamageReaction() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn Enemy;
	Enemy.Id = "enemy";
	Enemy.TypeId = "WalkingEnemy";
	Enemy.Position = {100.0f, 100.0f};
	Layer.Objects.push_back(Enemy);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const NativeObjectRuntime* Runtime = Objects.Find("enemy");
	assert(Runtime != nullptr);
	const ObjectHitBounds Bounds = Runtime->HitBounds();
	const float SourceCenterX =
		Bounds.Position.X + Bounds.Size.X * 0.5f;
	assert(NearlyEqual(SourceCenterX, 116.0f));

	CharacterBody LeftBody;
	LeftBody.Position = {86.0f, 100.0f};
	CharacterController LeftPlayer(LeftBody);
	CharacterTouchBounds LeftTouch = LeftPlayer.TouchBounds();
	const std::vector<NativeObjectContact> LeftContacts =
		Objects.FindContacts(
			{LeftTouch.Left, LeftTouch.Top},
			{LeftTouch.Right - LeftTouch.Left,
			 LeftTouch.Bottom - LeftTouch.Top});
	assert(LeftContacts.size() == 1);
	assert(LeftContacts[0].ContactDamage == 1);

	const float LeftPlayerCenterX =
		LeftPlayer.Body().Position.X +
		LeftPlayer.Body().Width * 0.5f;
	const int LeftDirection =
		DamageReactionState::DirectionAwayFromSource(
			LeftPlayerCenterX, SourceCenterX, 1);
	assert(LeftDirection == -1);

	DamageReactionState Reaction;
	assert(Reaction.Begin(LeftDirection));
	// Damage中は同じ/別の危険源から再度Beginされても無効。
	assert(!Reaction.Begin(1));
	assert(Reaction.AdvanceFrame() == -1.0f);

	while (Reaction.Active()) {
		Reaction.AdvanceFrame();
	}
	assert(!Reaction.Active());

	CharacterBody RightBody;
	RightBody.Position = {114.0f, 100.0f};
	CharacterController RightPlayer(RightBody);
	CharacterTouchBounds RightTouch = RightPlayer.TouchBounds();
	const std::vector<NativeObjectContact> RightContacts =
		Objects.FindContacts(
			{RightTouch.Left, RightTouch.Top},
			{RightTouch.Right - RightTouch.Left,
			 RightTouch.Bottom - RightTouch.Top});
	assert(RightContacts.size() == 1);

	const float RightPlayerCenterX =
		RightPlayer.Body().Position.X +
		RightPlayer.Body().Width * 0.5f;
	const int RightDirection =
		DamageReactionState::DirectionAwayFromSource(
			RightPlayerCenterX, SourceCenterX, -1);
	assert(RightDirection == 1);
	assert(Reaction.Begin(RightDirection));
	assert(Reaction.AdvanceFrame() == 1.0f);
}

TileCatalog MakeNativeObjectMovementCatalog() {
	TileCatalog Catalog;

	TileDefinition Empty;
	Empty.Id = 0;
	Empty.Collision = CollisionShape::None;
	assert(Catalog.Register(Empty).IsSuccess());

	TileDefinition Solid;
	Solid.Id = 1;
	Solid.Collision = CollisionShape::Solid;
	assert(Catalog.Register(Solid).IsSuccess());

	TileDefinition OneWay;
	OneWay.Id = 2;
	OneWay.Collision = CollisionShape::OneWay;
	assert(Catalog.Register(OneWay).IsSuccess());

	TileDefinition DropThrough;
	DropThrough.Id = 3;
	DropThrough.Collision = CollisionShape::DropThroughOneWay;
	assert(Catalog.Register(DropThrough).IsSuccess());

	return Catalog;
}

ObjectSpawn MakeWalkingEnemySpawn(
	const std::string& Id,
	WorldPosition Position,
	const char* Direction,
	int Variant) {
	ObjectSpawn Enemy;
	Enemy.Id = Id;
	Enemy.TypeId = "WalkingEnemy";
	Enemy.Position = Position;
	Enemy.Properties["direction"] =
		StagePropertyValue::String(Direction);
	Enemy.Properties["variant"] =
		StagePropertyValue::Integer(Variant);
	Enemy.Properties["speed"] =
		StagePropertyValue::Float(2.0f);
	return Enemy;
}

void TestNativeWalkingEnemyMovesAndTurnsAtWall() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 0, 0, 1, 0},
		{1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeNativeObjectMovementCatalog();

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"walker", {64.0f, 32.0f}, "right", 1));
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	for (int Frame = 0; Frame < 5; ++Frame) {
		Objects.Update(Map, Catalog);
	}

	const NativeObjectRuntime* Enemy = Objects.Find("walker");
	assert(Enemy != nullptr);
	assert(NearlyEqual(Enemy->Position.X, 72.0f));
	assert(NearlyEqual(Enemy->Position.Y, 32.0f));
	assert(Enemy->Direction == -1);
	assert(Enemy->Grounded);
	assert(NearlyEqual(Enemy->Velocity.Y, 0.0f));

	Objects.Update(Map, Catalog);
	Enemy = Objects.Find("walker");
	assert(NearlyEqual(Enemy->Position.X, 70.0f));
	assert(Enemy->Direction == -1);
}

void TestNativeWalkingEnemiesTurnWhenTheyMeet() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1, 1}
	});
	TileCatalog Catalog = MakeNativeObjectMovementCatalog();

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"left", {48.0f, 32.0f}, "right", 1));
	Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"right", {80.0f, 32.0f}, "left", 1));
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	for (int Frame = 0; Frame < 5; ++Frame) {
		Objects.Update(Map, Catalog);
	}

	const NativeObjectRuntime* Left = Objects.Find("left");
	const NativeObjectRuntime* Right = Objects.Find("right");
	assert(Left != nullptr);
	assert(Right != nullptr);
	assert(Left->Direction == -1);
	assert(Right->Direction == 1);
	assert(Left->Velocity.X < 0.0f);
	assert(Right->Velocity.X > 0.0f);
	assert(!Left->HitBounds().Intersects(
		Right->HitBounds().Position,
		Right->HitBounds().Size));

	Objects.Update(Map, Catalog);
	Left = Objects.Find("left");
	Right = Objects.Find("right");
	assert(Left->Position.X < Right->Position.X);
	assert(Left->Direction == -1);
	assert(Right->Direction == 1);
}

void TestNativeWalkingEnemyVariantsDifferAtCliff() {
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 0, 0},
		{0, 0, 0, 0, 0}
	});
	TileCatalog Catalog = MakeNativeObjectMovementCatalog();

	StageArea Variant1Area;
	ObjectLayer Variant1Layer;
	Variant1Layer.Metadata.Id = "objects";
	Variant1Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"walker-1", {64.0f, 32.0f}, "right", 1));
	Variant1Area.ObjectLayers.push_back(Variant1Layer);

	NativeObjectSystem Variant1;
	assert(Variant1.Reset(Variant1Area).IsSuccess());
	for (int Frame = 0; Frame < 10; ++Frame) {
		Variant1.Update(Map, Catalog);
	}
	const NativeObjectRuntime* Walker1 = Variant1.Find("walker-1");
	assert(Walker1 != nullptr);
	assert(Walker1->Position.Y > 32.0f);
	assert(!Walker1->Grounded);
	assert(Walker1->Direction == 1);

	StageArea Variant2Area;
	ObjectLayer Variant2Layer;
	Variant2Layer.Metadata.Id = "objects";
	Variant2Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"walker-2", {64.0f, 32.0f}, "right", 2));
	Variant2Area.ObjectLayers.push_back(Variant2Layer);

	NativeObjectSystem Variant2;
	assert(Variant2.Reset(Variant2Area).IsSuccess());
	for (int Frame = 0; Frame < 10; ++Frame) {
		Variant2.Update(Map, Catalog);
	}
	const NativeObjectRuntime* Walker2 = Variant2.Find("walker-2");
	assert(Walker2 != nullptr);
	assert(NearlyEqual(Walker2->Position.Y, 32.0f));
	assert(Walker2->Grounded);
	assert(Walker2->Direction == -1);
	assert(Walker2->Position.X < 72.0f);
}

void TestNativeWalkingEnemyStandsOnOneWayFloors() {
	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{2, 2, 2}
	});
	TileCatalog Catalog = MakeNativeObjectMovementCatalog();

	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";
	Layer.Objects.push_back(
		MakeWalkingEnemySpawn(
			"walker", {32.0f, 16.0f}, "right", 1));
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());
	for (int Frame = 0; Frame < 20; ++Frame) {
		Objects.Update(Map, Catalog);
	}

	const NativeObjectRuntime* Enemy = Objects.Find("walker");
	assert(Enemy != nullptr);
	assert(Enemy->Grounded);
	assert(NearlyEqual(Enemy->Position.Y, 32.0f));
}

void TestNativeObjectRuntimePropertiesOverrideHitbox() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn Custom;
	Custom.Id = "custom";
	Custom.TypeId = "UnknownObject";
	Custom.Position = {10.0f, 20.0f};
	Custom.Properties["hitboxOffset"] =
		StagePropertyValue::Vector2({3.0f, 4.0f});
	Custom.Properties["hitboxSize"] =
		StagePropertyValue::Vector2({12.0f, 18.0f});
	Custom.Properties["contactDamage"] =
		StagePropertyValue::Integer(2);
	Layer.Objects.push_back(Custom);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	assert(Objects.Reset(Area).IsSuccess());

	const NativeObjectRuntime* Runtime = Objects.Find("custom");
	assert(Runtime != nullptr);
	assert(NearlyEqual(Runtime->HitboxOffset.X, 3.0f));
	assert(NearlyEqual(Runtime->HitboxOffset.Y, 4.0f));
	assert(NearlyEqual(Runtime->HitboxSize.X, 12.0f));
	assert(NearlyEqual(Runtime->HitboxSize.Y, 18.0f));
	assert(Runtime->ContactDamage == 2);

	const ObjectHitBounds Bounds = Runtime->HitBounds();
	assert(NearlyEqual(Bounds.Position.X, 13.0f));
	assert(NearlyEqual(Bounds.Position.Y, 24.0f));
}

void TestNativeObjectRuntimeRejectsInvalidHitbox() {
	StageArea Area;
	ObjectLayer Layer;
	Layer.Metadata.Id = "objects";

	ObjectSpawn Invalid;
	Invalid.Id = "invalid";
	Invalid.TypeId = "WalkingEnemy";
	Invalid.Properties["hitboxSize"] =
		StagePropertyValue::Vector2({0.0f, 32.0f});
	Layer.Objects.push_back(Invalid);
	Area.ObjectLayers.push_back(Layer);

	NativeObjectSystem Objects;
	Result<bool> Reset = Objects.Reset(Area);
	assert(Reset.IsFailure());
	assert(
		Reset.Error().find("hitboxSize must be positive") !=
		std::string::npos);
}

void TestNativeStageDataValidationRejectsUndefinedRuleReplacement() {
	TileSetDefinition TileSet;
	TileSet.Id = "rules";
	TileSet.ImageFile = "dummy.png";
	TileSet.Columns = 1;
	TileSet.Rows = 1;

	TileDefinition Empty;
	Empty.Id = 0;
	Empty.ImageIndex = 0;
	TileSet.TerrainTiles.push_back(Empty);

	TileDefinition Replacer;
	Replacer.Id = 1;
	Replacer.ImageIndex = 0;
	TileRule Rule;
	Rule.Trigger = TileTrigger::Touch;
	Rule.Action = TileAction::ReplaceTile;
	Rule.Value = 99;
	Replacer.Rules.push_back(Rule);
	TileSet.TerrainTiles.push_back(Replacer);

	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.TileSetId = "rules";
	Terrain.Map = MakeMap({{0}});

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {Terrain};

	StageData Data;
	Data.Id = "bad-rule-target";
	Data.StartAreaId = "main";
	Data.TileSets = {TileSet};
	Data.Areas = {Area};

	Result<bool> Validation = ValidateStageData(Data);
	assert(Validation.IsFailure());
	assert(
		Validation.Error().find("undefined replacement id 99") !=
		std::string::npos);
}

void TestNativeStageDataValidationRejectsUndefinedTerrainTile() {
	TileSetDefinition TileSet;
	TileSet.Id = "test";
	TileSet.ImageFile = "dummy.png";
	TileSet.Columns = 1;
	TileSet.Rows = 1;
	TileDefinition Empty;
	Empty.Id = 0;
	Empty.ImageIndex = 0;
	TileSet.TerrainTiles.push_back(Empty);

	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.TileSetId = "test";
	Terrain.Map = MakeMap({{9}});

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {Terrain};

	StageData Data;
	Data.Id = "undefined-terrain";
	Data.StartAreaId = "main";
	Data.TileSets = {TileSet};
	Data.Areas = {Area};

	Result<bool> Validation = ValidateStageData(Data);
	assert(Validation.IsFailure());
	assert(Validation.Error().find("undefined tile id 9") != std::string::npos);
}

void TestNativeStageDataValidationRejectsUnknownTileSet() {
	TileLayer Terrain;
	Terrain.Metadata.Id = "terrain";
	Terrain.Metadata.Name = "Terrain";
	Terrain.Role = TileLayerRole::Terrain;
	Terrain.TileSetId = "missing";
	Terrain.Map = MakeMap({{1}});

	StageArea Area;
	Area.Id = "main";
	Area.Width = 1;
	Area.Height = 1;
	Area.TileLayers = {Terrain};

	StageData Data;
	Data.Id = "unknown-tileset";
	Data.StartAreaId = "main";
	Data.Areas = {Area};

	Result<bool> Validation = ValidateStageData(Data);
	assert(Validation.IsFailure());
	assert(Validation.Error().find("unknown tile set") != std::string::npos);
}

void TestNativeStageDataLoaderRejectsUnsupportedVersion() {
	const std::string Json =
		"{\"formatVersion\":2,"
		"\"id\":\"future\","
		"\"startArea\":\"main\","
		"\"areas\":[]}";
	Result<StageData> Loaded = NativeStageDataLoader::Parse(Json);
	assert(Loaded.IsFailure());
	assert(Loaded.Error().find("formatVersion") != std::string::npos);
}

void TestNativeStageDataLoaderRejectsNestedProperties() {
	const std::string Json =
		"{\"formatVersion\":1,"
		"\"id\":\"bad-property\","
		"\"startArea\":\"main\","
		"\"properties\":{\"nested\":{\"value\":1}},"
		"\"areas\":[]}";
	Result<StageData> Loaded = NativeStageDataLoader::Parse(Json);
	assert(Loaded.IsFailure());
	assert(Loaded.Error().find("properties.nested") != std::string::npos);
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

	// 見た目32pxの右端が近づいただけではTouchしない。
	// 中央16pxの右端が次タイルへ入った時点で初めて反応する。
	Player.Step(1.0f, false, Map, Catalog);
	bool FoundTouch = false;
	for (const TileInteraction& Interaction : Player.Interactions()) {
		if (Interaction.Trigger == TileTrigger::Touch &&
			Interaction.Position.Column == 1 && Interaction.Position.Row == 0 &&
			Interaction.TileId == 20) {
			FoundTouch = true;
		}
	}
	assert(!FoundTouch);

	Player.Step(1.0f, false, Map, Catalog);
	Player.Step(1.0f, false, Map, Catalog);
	FoundTouch = false;
	for (const TileInteraction& Interaction : Player.Interactions()) {
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

void TestCentralTouchHitboxStillTouchesSolidHazardFromSide() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/hazard-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 73, 0},
		{1, 1, 1}
	});
	CharacterBody Body;
	Body.Position = {0.0f, 0.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	bool FoundDamageTouch = false;
	for (int Frame = 0; Frame < 10; ++Frame) {
		Player.Step(1.0f, false, Map, Loaded.Value());
		for (const TileInteraction& Interaction : Player.Interactions()) {
			if (Interaction.Trigger == TileTrigger::Touch &&
				Interaction.Position.Column == 1 &&
				Interaction.Position.Row == 0 &&
				Interaction.TileId == 73) {
				FoundDamageTouch = true;
				break;
			}
		}
	}

	assert(FoundDamageTouch);
	// Touch判定は中央16px幅なので、Solid壁へ衝突する数フレーム前から
	// 危険ブロックへ届く。Touchを検出しても移動を続け、最終的な壁位置も確認する。
	// Solid衝突は中央軸で止まり、見た目は半分ほどブロックへ重なる。
	assert(NearlyEqual(Player.Body().Position.X, 16.0f));
	const CharacterTouchBounds Bounds = Player.TouchBounds();
	assert(NearlyEqual(Bounds.Left, 24.0f));
	assert(NearlyEqual(Bounds.Right, 40.0f));
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

void TestCharacterJumpIsCancelledUnder2x1HighSideConnectedToBlock() {
	TileCatalog Catalog = MakeTerrainCatalog();
	TileMap Map = MakeMap({
		{0, 0, 0, 0, 0},
		{0, 4, 5, 1, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1, 1}
	});
	CharacterBody Body;
	// 高い端にブロックが接続された坂の右下から左ジャンプする。
	// 開始位置では頭上のSolidへ即座に当たるため、ジャンプ初速はそこで失われる。
	Body.Position = {84.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	bool ClearedTerrain = false;
	bool RoseAfterClearing = false;
	for (int Frame = 0; Frame < 40; ++Frame) {
		Player.Step(-1.0f, Frame == 0, Map, Catalog);
		const float Center = Player.Body().Position.X + 15.0f;
		if (Center >= 32.0f && Center < 128.0f) {
			// 坂とブロックの下面を通過しない。
			assert(Player.Body().Position.Y >= 64.0f);
		} else if (Center < 32.0f) {
			ClearedTerrain = true;
			if (Player.Body().Position.Y < 64.0f) RoseAfterClearing = true;
		}
		assert(Player.Body().Position.Y <= 64.0f);
	}
	assert(ClearedTerrain);
	// 天井衝突でVelocityYは0になる。一度抜けた後にジャンプ初速を復活させない。
	assert(!RoseAfterClearing);
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

void TestCharacterCanJumpOffLadder() {
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

	const float BeforeJumpY = Player.Body().Position.Y;
	CharacterInput Jump;
	Jump.JumpPressed = true;
	Player.Step(Jump, Map, Loaded.Value());

	assert(Player.Mode() == MovementMode::Normal);
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Velocity.Y, -9.0f));
	assert(Player.Body().Position.Y < BeforeJumpY);
}

void TestLadderJumpFollowsReversedGravity() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/interaction-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{1, 1, 1},
		{0, 55, 0},
		{0, 59, 0},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);
	CharacterInput Idle;

	// G^で逆重力になり、上のLadderタイル内で天井へ接地する。
	for (int Frame = 0; Frame < 50 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	if (Player.Gravity() != GravityDirection::Up) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	for (int Frame = 0; Frame < 50 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	assert(Player.Gravity() == GravityDirection::Up);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));

	CharacterInput Up;
	Up.Vertical = -1.0f;
	Player.Step(Up, Map, Loaded.Value());
	assert(Player.Mode() == MovementMode::Climbing);

	const float BeforeJumpY = Player.Body().Position.Y;
	CharacterInput Jump;
	Jump.JumpPressed = true;
	Player.Step(Jump, Map, Loaded.Value());

	assert(Player.Mode() == MovementMode::Normal);
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Velocity.Y, 9.0f));
	assert(Player.Body().Position.Y > BeforeJumpY);
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

void TestDropThroughOneWayDefinition() {
	Result<TerrainStageData> Loaded =
		TerrainStageLoader::Load("dat/stage/through-test/stage.ini");
	assert(Loaded.IsSuccess());
	assert(Loaded.Value().Map.Width() == 16);
	assert(Loaded.Value().Map.Height() == 9);
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.X, 128.0f));
	assert(NearlyEqual(Loaded.Value().PlayerSpawn.Y, 128.0f));

	const TileDefinition* Normal = Loaded.Value().Catalog.Find(61);
	const TileDefinition* Through = Loaded.Value().Catalog.Find(62);
	assert(Normal != nullptr && Through != nullptr);
	assert(Normal->Collision == CollisionShape::OneWay);
	assert(Through->Collision == CollisionShape::DropThroughOneWay);
	assert(CanvasMasaoTerrain::CodeFor(Through->Collision) ==
		CanvasMasaoTerrain::OneWayCode);
}

void TestCharacterLandsOnDropThroughPlatformNormally() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/through-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{0, 62, 0},
		{0, 0, 0},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 0.0f};
	Body.Grounded = false;
	CharacterController Player(Body);

	CharacterInput Idle;
	for (int Frame = 0; Frame < 40 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}

	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
}

void TestCharacterDropsThroughPlatformWithDown() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/through-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{0, 62, 0},
		{0, 0, 0},
		{1, 1, 1}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Down;
	Down.Vertical = 1.0f;

	// 最初の0.5px/frame相当は整数移動0px。次フレームで床面を跨ぐ。
	Player.Step(Down, Map, Loaded.Value());
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));

	Player.Step(Down, Map, Loaded.Value());
	assert(Player.Body().Position.Y > 32.0f);
	assert(!Player.Body().Grounded);

	CharacterInput Idle;
	for (int Frame = 0; Frame < 60 && !Player.Body().Grounded; ++Frame) {
		Player.Step(Idle, Map, Loaded.Value());
	}
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 96.0f));
}

void TestNormalOneWayDoesNotDropWithDown() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/through-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 0, 0},
		{0, 61, 0},
		{0, 0, 0}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 32.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Down;
	Down.Vertical = 1.0f;
	for (int Frame = 0; Frame < 8; ++Frame) {
		Player.Step(Down, Map, Loaded.Value());
	}

	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 32.0f));
}

void TestReverseGravityDropsThroughPlatformWithUp() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/through-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 62, 0},
		{0, 59, 0},
		{0, 0, 0}
	});

	CharacterBody Body;
	// row1のThrough下面(y=64)に逆向きで立つ。
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Up;
	Up.Vertical = -1.0f;

	Player.Step(Up, Map, Loaded.Value());
	assert(Player.Gravity() == GravityDirection::Up);
	assert(!Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 64.0f));

	Player.Step(Up, Map, Loaded.Value());
	assert(Player.Gravity() == GravityDirection::Up);
	assert(Player.Body().Position.Y < 64.0f);
	assert(!Player.Body().Grounded);
}

void TestReverseGravityNormalOneWayDoesNotDropWithUp() {
	Result<TileCatalog> Loaded =
		TerrainStageLoader::LoadCatalog("dat/stage/through-test/tiles.csv");
	assert(Loaded.IsSuccess());

	TileMap Map = MakeMap({
		{0, 0, 0},
		{0, 61, 0},
		{0, 59, 0},
		{0, 0, 0}
	});

	CharacterBody Body;
	Body.Position = {32.0f, 64.0f};
	Body.Grounded = true;
	CharacterController Player(Body);

	CharacterInput Up;
	Up.Vertical = -1.0f;
	for (int Frame = 0; Frame < 8; ++Frame) {
		Player.Step(Up, Map, Loaded.Value());
	}

	assert(Player.Gravity() == GravityDirection::Up);
	assert(Player.Body().Grounded);
	assert(NearlyEqual(Player.Body().Position.Y, 64.0f));
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

int main(int argc, char* argv[]) {
	if (argc >= 2 && std::string(argv[1]) == "--native-stage-loader") {
		TestStagePropertyValuesKeepTypes();
		TestNativeGoalUsesCentral16x32TouchBounds();
		TestStageRegionGeometryIntersection();
		TestNativeStageDataSupportsOverlappingContent();
		TestNativeStageDataValidationRejectsAmbiguousStructure();
		TestNativeStageDataAllowsExternalTransitions();
		TestNativeStageDataLoaderLoadsJsonAndCsv();
		TestNativeStageCharacterControllerUsesTerrainSemantics();
		TestNativeStageTileRulesApplyFromJson();
		TestNativeStageGameplayAdaptersFromJson();
		TestNativeObjectRuntimeBuildsTypeSpecificHitBounds();
		TestNativeObjectRuntimeUsesPlayerCentralTouchBounds();
		TestNativeObjectContactComposesWithDamageReaction();
		TestNativeWalkingEnemyClassifiesStompSeparatelyFromDamage();
		TestNativeBallSlimeTransitionsWalkingShellKickAndRecovery();
		TestNativeBallSlimeVariant2TurnsAtCliffOnlyWhileWalking();
		TestNativeKickedBallSlimeDefeatsOtherEnemyAndLifecycleResetsShell();
		TestStompRepositionMatchesHspOnePixelSeparation();
		TestCharacterSetVelocityKeepsInternalVelocityInSync();
		TestNativeWalkingEnemyMovesAndTurnsAtWall();
	TestNativeWalkingEnemiesTurnWhenTheyMeet();
		TestNativeWalkingEnemyVariantsDifferAtCliff();
		TestNativeWalkingEnemyStandsOnOneWayFloors();
		TestNativeObjectRuntimePropertiesOverrideHitbox();
		TestNativeObjectRuntimeRejectsInvalidHitbox();
		TestNativeStageDataValidationRejectsInvalidSwitchBinding();
		TestNativeStageDataValidationRejectsUndefinedRuleReplacement();
		TestNativeStageDataValidationRejectsUndefinedTerrainTile();
		TestNativeStageDataValidationRejectsUnknownTileSet();
		TestNativeStageDataLoaderRejectsUnsupportedVersion();
		TestNativeStageDataLoaderRejectsNestedProperties();
		std::cout << "Native stage data tests passed.\n";
		return 0;
	}

	TestAssetPaths();
	TestGridDataLoader();
	TestExternalTerrainStage();
	TestBrickDefinitionAndHitEffect();
	TestBrickWithLowHealthBumpsButDoesNotBreak();
	TestBrickWithFullHealthBreaksAfterVersion1Delay();
	TestBrickIgnoresRepeatedHitsWhileAnimating();
	TestHazardDefinitionsMatchVersion1Targets();
	TestHazardTargetsFilterPlayerAndEnemyActors();
	TestLegacyDamagingRuleStillTargetsPlayer();
	TestCharacterExposesActualTouchProbePoints();
	TestDamageKnockbackMovesAwayFromHazardCenter();
	TestDamageReactionMatchesVersion1SixteenFrames();
	TestDirectCollectibleDefinitionsMatchVersion1();
	TestDirectCollectiblesEmitVersion1RewardsAndDisappear();
	TestPlayerResourceRulesMatchVersion1Limits();
	TestGoalStageDefinitionsAndEffects();
	TestNormalAndSecretGoalProgressAreIndependent();
	TestStageCompletionEndsRunWithOneGoal();
	TestStageProgressTracksClearStatePerStage();
	TestStepWithoutInputStopsHorizontalAndSettlesVertically();
	TestExternalPipeStage();
	TestPipeDirectionInputMatching();
	TestPipeTransportRequiresDirectionAndAlignment();
	TestSidePipeRequiresGrounded();
	TestPipeTransportFadesBeforeEmergence();
	TestPipeTileDefinitionsAreSolid();
	TestTileMapBounds();
	TestGameModes();
	TestTileCatalog();
	TestSlopeSurfaces();
	TestStairSurfaces();
	TestSlopeSolidRegions();
	TestSlopeSideBlocks();
	TestSlopeGroundSnap();
	TestLayeredMap();
	TestNativeStageDataLoaderLoadsJsonAndCsv();
	TestNativeStageCharacterControllerUsesTerrainSemantics();
	TestNativeStageTileRulesApplyFromJson();
	TestNativeStageGameplayAdaptersFromJson();
	TestNativeObjectRuntimeBuildsTypeSpecificHitBounds();
	TestNativeObjectRuntimeUsesPlayerCentralTouchBounds();
	TestNativeObjectContactComposesWithDamageReaction();
	TestNativeWalkingEnemyClassifiesStompSeparatelyFromDamage();
	TestNativeWalkingEnemyLifecycleUsesCameraAndKeepsDefeatedState();
	TestNativeCarrotManWaitsEmergesAndStartsWalking();
	TestNativeBallSlimeTransitionsWalkingShellKickAndRecovery();
	TestNativeBallSlimeVariant2TurnsAtCliffOnlyWhileWalking();
	TestNativeKickedBallSlimeDefeatsOtherEnemyAndLifecycleResetsShell();
	TestStompRepositionMatchesHspOnePixelSeparation();
	TestCharacterSetVelocityKeepsInternalVelocityInSync();
	TestNativeWalkingEnemyMovesAndTurnsAtWall();
	TestNativeWalkingEnemiesTurnWhenTheyMeet();
	TestNativeWalkingEnemyVariantsDifferAtCliff();
	TestNativeWalkingEnemyStandsOnOneWayFloors();
	TestNativeObjectRuntimePropertiesOverrideHitbox();
	TestNativeObjectRuntimeRejectsInvalidHitbox();
	TestNativeStageDataValidationRejectsInvalidSwitchBinding();
	TestNativeStageDataValidationRejectsUndefinedRuleReplacement();
	TestNativeStageDataValidationRejectsUndefinedTerrainTile();
	TestNativeStageDataValidationRejectsUnknownTileSet();
	TestNativeStageDataLoaderRejectsUnsupportedVersion();
	TestNativeStageDataLoaderRejectsNestedProperties();
	TestStagePropertyValuesKeepTypes();
	TestNativeGoalUsesCentral16x32TouchBounds();
	TestStageRegionGeometryIntersection();
	TestNativeStageDataSupportsOverlappingContent();
	TestNativeStageDataValidationRejectsAmbiguousStructure();
	TestNativeStageDataAllowsExternalTransitions();
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
	TestCentralTouchHitboxStillTouchesSolidHazardFromSide();
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
	TestCharacterJumpIsCancelledUnder2x1HighSideConnectedToBlock();
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
	TestCharacterCanJumpOffLadder();
	TestLadderJumpFollowsReversedGravity();
	TestLadderEntryRulesMatchVersion1();
	TestLadderBuilderCreatesTilesUntilSolidCeiling();
	TestDropThroughOneWayDefinition();
	TestCharacterLandsOnDropThroughPlatformNormally();
	TestCharacterDropsThroughPlatformWithDown();
	TestNormalOneWayDoesNotDropWithDown();
	TestReverseGravityDropsThroughPlatformWithUp();
	TestReverseGravityNormalOneWayDoesNotDropWithUp();
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
