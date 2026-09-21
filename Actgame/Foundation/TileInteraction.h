#pragma once

#include "Coordinates.h"

#include <vector>

namespace uchinoko {

class TileCatalog;
class TileMap;

// V1 の Touched/Hited/On/PushL/PushR を、継承ではなくデータとして表現する。
// CharacterController は「何にどう接触したか」だけを通知し、ゲーム効果は知らない。
enum class TileTrigger {
	Touch,
	HitFromBelow,
	StandOn,
	PushFromLeft,
	PushFromRight
};

// 1つのタイルへ複数ルールを合成できるため、CoinBlock/HealingBlock/... の
// 派生クラス増殖を避けられる。
enum class TileActor {
	Player,
	Enemy
};

enum class TileTarget {
	Player,
	Enemy,
	Both
};

enum class TileAction {
	None,
	ReplaceTile,
	BreakTile,
	AddCoin,
	AddHealth,
	AddLife,
	AddScore,
	Damage,
	InstantDeath,
	SpawnItem,
	IncrementCount,
	ToggleSwitch,
	Goal,
	HitBrick
};

enum class TileCountCondition {
	Any,
	LessThan,
	LessEqual,
	Equal,
	GreaterEqual,
	GreaterThan
};

struct TileRule {
	TileTrigger Trigger = TileTrigger::Touch;
	TileAction Action = TileAction::None;
	int Value = 0;
	bool Once = false;
	TileCountCondition CountCondition = TileCountCondition::Any;
	int CountValue = 0;
	TileTarget Target = TileTarget::Player;
};

struct TileInteraction {
	TileTrigger Trigger = TileTrigger::Touch;
	TilePosition Position;
	int TileId = 0;
	TileActor Actor = TileActor::Player;
};

// Foundation は SoundManager/GameData 等を直接呼ばず、外側へ意味のある結果を返す。
enum class TileEffectType {
	AddCoin,
	AddHealth,
	AddLife,
	AddScore,
	Damage,
	InstantDeath,
	SpawnItem,
	ToggleSwitch,
	Goal,
	BrickHit,
	TileBroken
};

struct TileEffect {
	TileEffectType Type = TileEffectType::AddScore;
	TilePosition Position;
	int Value = 0;
	int SourceTileId = 0;
	TileActor Actor = TileActor::Player;
};

struct TileRuntimeState {
	// 互換・デバッグ用に「何か一度きりのルールを消費したか」も保持する。
	bool Used = false;
	int Timer = 0;
	int Count = 0;
	std::vector<bool> ConsumedRules;
};

class TileRuntimeMap {
public:
	TileRuntimeMap() = default;
	explicit TileRuntimeMap(const TileMap& Map);

	void Reset(const TileMap& Map);
	TileRuntimeState* TryGet(TilePosition Position);
	const TileRuntimeState* TryGet(TilePosition Position) const;

private:
	std::vector<std::vector<TileRuntimeState>> States_;
};

struct TileBehaviorResult {
	bool Handled = false;
	std::vector<TileEffect> Effects;
};

class TileBehaviorSystem {
public:
	static TileBehaviorResult Apply(
		const TileInteraction& Interaction,
		TileMap& Map, const TileCatalog& Catalog, TileRuntimeMap& Runtime);

	static std::vector<TileEffect> ApplyAll(
		const std::vector<TileInteraction>& Interactions,
		TileMap& Map, const TileCatalog& Catalog, TileRuntimeMap& Runtime);
};

} // namespace uchinoko
