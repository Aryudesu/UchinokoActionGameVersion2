#include "BrickSystem.h"

#include "TileMap.h"

#include <algorithm>
#include <cstdlib>

namespace uchinoko {

namespace {

TileEffect MakeEffect(
	TileEffectType Type, const ActiveBrick& Brick, int Value) {
	TileEffect Effect;
	Effect.Type = Type;
	Effect.Position = Brick.Position;
	Effect.Value = Value;
	Effect.SourceTileId = Brick.SourceTileId;
	return Effect;
}

} // namespace

void BrickSystem::Reset() {
	Active_.clear();
	Fragments_.clear();
}

const ActiveBrick* BrickSystem::TryGet(TilePosition Position) const {
	for (std::size_t Index = 0; Index < Active_.size(); ++Index) {
		if (Active_[Index].Position.Column == Position.Column &&
			Active_[Index].Position.Row == Position.Row) {
			return &Active_[Index];
		}
	}
	return nullptr;
}

void BrickSystem::ConsumeTileEffects(
	const std::vector<TileEffect>& Effects,
	const GameStateSnapshot& GameState) {
	for (std::size_t Index = 0; Index < Effects.size(); ++Index) {
		if (Effects[Index].Type != TileEffectType::BrickHit) continue;
		if (TryGet(Effects[Index].Position) != nullptr) continue;

		ActiveBrick Brick;
		Brick.Position = Effects[Index].Position;
		Brick.SourceTileId = Effects[Index].SourceTileId;
		Brick.Frame = 0;
		Brick.Phase =
			GameState.Health >= Effects[Index].Value
				? BrickPhase::Breaking
				: BrickPhase::Bumping;
		Active_.push_back(Brick);
	}
}

void BrickSystem::UpdateFragments(float FragmentCullY) {
	for (std::size_t Index = 0; Index < Fragments_.size(); ++Index) {
		BrickFragment& Fragment = Fragments_[Index];
		// V1: GRAVITY=0.5 / Vmax.y=10.
		Fragment.Velocity.Y += 0.5f;
		if (Fragment.Velocity.Y > 10.0f) Fragment.Velocity.Y = 10.0f;
		Fragment.Position.X += Fragment.Velocity.X;
		Fragment.Position.Y += Fragment.Velocity.Y;
	}

	Fragments_.erase(
		std::remove_if(
			Fragments_.begin(), Fragments_.end(),
			[FragmentCullY](const BrickFragment& Fragment) {
				return Fragment.Position.Y > FragmentCullY;
			}),
		Fragments_.end());
}

void BrickSystem::SpawnFragments(
	TilePosition Position, int TileWidth, int TileHeight) {
	const float X = static_cast<float>(Position.Column * TileWidth);
	const float Y = static_cast<float>(Position.Row * TileHeight);

	for (int Index = 0; Index < Version1FragmentCount; ++Index) {
		BrickFragment Fragment;
		Fragment.Position = {X, Y};
		// V1 BlockFragment と同じ乱数幅。
		Fragment.Velocity.X = static_cast<float>(std::rand() % 11 - 5);
		Fragment.Velocity.Y = static_cast<float>(-(std::rand() % 15));
		Fragments_.push_back(Fragment);
	}
}

std::vector<TileEffect> BrickSystem::Update(
	TileMap& Map, int TileWidth, int TileHeight, float FragmentCullY) {
	std::vector<TileEffect> Effects;

	// V1では既存Effectのupdate後に新規破片が生成されるため、
	// 今フレーム生成した破片は次フレームから動かす。
	UpdateFragments(FragmentCullY);

	std::size_t Index = 0;
	while (Index < Active_.size()) {
		ActiveBrick& Brick = Active_[Index];
		++Brick.Frame;

		if (Brick.Phase == BrickPhase::Bumping) {
			if (Brick.Frame >= Version1BumpFrames) {
				Active_.erase(Active_.begin() + static_cast<std::ptrdiff_t>(Index));
				continue;
			}
			++Index;
			continue;
		}

		if (Brick.Frame < Version1BreakFrames) {
			++Index;
			continue;
		}

		int* Current = Map.TryGet(Brick.Position);
		if (Current != nullptr && *Current == Brick.SourceTileId) {
			*Current = 0;
			Effects.push_back(
				MakeEffect(TileEffectType::AddScore, Brick, Version1BreakScore));
			Effects.push_back(
				MakeEffect(TileEffectType::TileBroken, Brick, 0));
			SpawnFragments(Brick.Position, TileWidth, TileHeight);
		}

		Active_.erase(Active_.begin() + static_cast<std::ptrdiff_t>(Index));
	}

	return Effects;
}

int BrickSystem::VisualFrameOffset(TilePosition Position) const {
	const ActiveBrick* Brick = TryGet(Position);
	if (Brick == nullptr) return 0;

	const int DisplayFrame = std::max(0, Brick->Frame - 1);
	if (Brick->Phase == BrickPhase::Bumping) {
		// V1: 65,65,66,66,67,67,68,68,69
		return 5 + DisplayFrame / 2;
	}
	// V1: 61,61,62,62,63,63,64 の後に破壊。
	return 1 + DisplayFrame / 2;
}

} // namespace uchinoko
