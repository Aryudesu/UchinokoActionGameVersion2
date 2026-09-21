#pragma once

#include "ConditionalTerrain.h"
#include "Coordinates.h"
#include "TileInteraction.h"

#include <vector>

namespace uchinoko {

class TileMap;

enum class BrickPhase {
	Bumping,
	Breaking
};

struct ActiveBrick {
	TilePosition Position;
	int SourceTileId = 0;
	BrickPhase Phase = BrickPhase::Bumping;
	int Frame = 0;
};

struct BrickFragment {
	WorldPosition Position;
	WorldPosition Velocity;
};

class BrickSystem {
public:
	static constexpr int Version1RequiredHealth = 5;
	static constexpr int Version1BumpFrames = 9;
	static constexpr int Version1BreakFrames = 7;
	static constexpr int Version1BreakScore = 10;
	static constexpr int Version1FragmentCount = 5;

	void Reset();

	// TileBehaviorSystem が出した BrickHit を受け取り、
	// 現在HPに応じて壊れないアニメ / 破壊アニメを開始する。
	void ConsumeTileEffects(
		const std::vector<TileEffect>& Effects,
		const GameStateSnapshot& GameState);

	// V1 の Block::update() 相当。
	// 返り値には破壊完了時の AddScore / TileBroken を含む。
	std::vector<TileEffect> Update(
		TileMap& Map, int TileWidth, int TileHeight, float FragmentCullY);

	const ActiveBrick* TryGet(TilePosition Position) const;
	const std::vector<BrickFragment>& Fragments() const { return Fragments_; }

	// V1 の BlImg=60 系に対する相対フレーム。
	// Idleは0、Bumpingは5..9、Breakingは1..4。
	int VisualFrameOffset(TilePosition Position) const;

private:
	void SpawnFragments(TilePosition Position, int TileWidth, int TileHeight);
	void UpdateFragments(float FragmentCullY);

	std::vector<ActiveBrick> Active_;
	std::vector<BrickFragment> Fragments_;
};

} // namespace uchinoko
