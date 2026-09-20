#pragma once

#include "Result.h"
#include "TileInteraction.h"
#include "ConditionalTerrain.h"

#include <vector>

namespace uchinoko {

enum class MovementRegion {
	None,
	Ladder,
	Water,
	GravityUp,
	GravityDown
};

enum class CollisionShape {
	None,
	Solid,
	OneWay,
	DropThroughOneWay,
	HitFromBelowOnly,
	SlopeUpRight,
	SlopeUpLeft,
	Stair2x1UpRightLow,
	Stair2x1UpRightHigh,
	Stair2x1UpLeftHigh,
	Stair2x1UpLeftLow,
	Stair1x2UpRightBottom,
	Stair1x2UpRightTop,
	Stair1x2UpLeftTop,
	Stair1x2UpLeftBottom
};

struct TileDefinition {
	int Id = 0;
	int ImageIndex = 0;
	CollisionShape Collision = CollisionShape::None;
	// 旧5列CSVとの互換用。Register時に Rules へ正規化される。
	bool Breakable = false;
	bool Damaging = false;
	std::vector<TileRule> Rules;

	// ON/OFF等の共有状態に応じて、Map上のIDを切り替えるための束縛。
	// -1 の場合は共有スイッチに依存しない。
	int SwitchChannel = -1;
	int SwitchOnTileId = -1;
	int SwitchOffTileId = -1;

	// 0より大きい場合、このスイッチチャネルを指定フレーム周期で自動反転する。
	// DisAppBlock1/2 のような時間制出現ブロック用。
	int AutoTogglePeriod = 0;

	// 外部ゲーム状態（Coins/Health/Lives/Score）で地形IDを切り替える。
	GameStateField ConditionField = GameStateField::None;
	ComparisonOperator ConditionOperator = ComparisonOperator::Equal;
	int ConditionThreshold = 0;
	int ConditionTrueTileId = -1;
	int ConditionFalseTileId = -1;

	// キャラクターの移動モードへ影響する領域。衝突形状とは独立して扱う。
	MovementRegion Movement = MovementRegion::None;
};

class TileCatalog {
public:
	Result<bool> Register(TileDefinition Definition);
	const TileDefinition* Find(int Id) const;

private:
	std::vector<TileDefinition> Definitions_;
};

} // namespace uchinoko
