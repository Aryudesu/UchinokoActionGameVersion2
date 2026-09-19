#pragma once

#include "Coordinates.h"
#include "TileInteraction.h"

#include <vector>

namespace uchinoko {

// V1 の MakeObject(1/2/3) に対応する、Foundation 側の意味のある種類。
// CSV の SpawnItem value にはこの整数値を指定する。
enum class ItemKind {
	Coin = 1,
	Healing = 2,
	OneUp = 3
};

struct SpawnedItem {
	ItemKind Kind = ItemKind::Coin;
	TilePosition SourceTile;
	WorldPosition Position;
	float VelocityY = -10.0f;
	bool Active = true;
};

class ItemSystem {
public:
	void Reset();

	// TileBehaviorSystem が返した SpawnItem だけを受け取り、実体へ変換する。
	// SoundManager / PlayerManager 等には依存しない。
	void ConsumeTileEffects(
		const std::vector<TileEffect>& Effects,
		int TileWidth = 32, int TileHeight = 32);

	// V1 と同様、上へ飛び出して下降速度が一定値に達したら報酬へ変換する。
	// 返された TileEffect をゲーム側が Player / Score / SE 等へ接続する。
	std::vector<TileEffect> Update(float Gravity = 0.5f);

	const std::vector<SpawnedItem>& Items() const { return Items_; }

private:
	bool Spawn(ItemKind Kind, TilePosition Source, int TileWidth, int TileHeight);
	static bool TryParseKind(int Value, ItemKind& Kind);
	static void AddRewardEffects(
		std::vector<TileEffect>& Effects, const SpawnedItem& Item);

	std::vector<SpawnedItem> Items_;
};

} // namespace uchinoko
