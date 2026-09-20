#include "ItemSystem.h"

#include <algorithm>

namespace uchinoko {

void ItemSystem::Reset() {
	Items_.clear();
}

bool ItemSystem::TryParseKind(int Value, ItemKind& Kind) {
	switch (Value) {
	case static_cast<int>(ItemKind::Coin):
		Kind = ItemKind::Coin;
		return true;
	case static_cast<int>(ItemKind::Healing):
		Kind = ItemKind::Healing;
		return true;
	case static_cast<int>(ItemKind::OneUp):
		Kind = ItemKind::OneUp;
		return true;
	default:
		return false;
	}
}

bool ItemSystem::Spawn(
	ItemKind Kind, TilePosition Source, int TileWidth, int TileHeight) {
	if (TileWidth <= 0 || TileHeight <= 0) return false;

	SpawnedItem Item;
	Item.Kind = Kind;
	Item.SourceTile = Source;
	Item.Position.X = static_cast<float>(Source.Column * TileWidth);
	// V1: MakeObject(..., pos.x, pos.y - 32)
	Item.Position.Y = static_cast<float>((Source.Row - 1) * TileHeight);
	Item.VelocityY = -10.0f;
	Item.Active = true;
	Items_.push_back(Item);
	return true;
}

void ItemSystem::ConsumeTileEffects(
	const std::vector<TileEffect>& Effects, int TileWidth, int TileHeight) {
	for (std::size_t Index = 0; Index < Effects.size(); ++Index) {
		if (Effects[Index].Type != TileEffectType::SpawnItem) continue;
		ItemKind Kind;
		if (!TryParseKind(Effects[Index].Value, Kind)) continue;
		Spawn(Kind, Effects[Index].Position, TileWidth, TileHeight);
	}
}

void ItemSystem::AddRewardEffects(
	std::vector<TileEffect>& Effects, const SpawnedItem& Item) {
	TileEffect Effect;
	Effect.Position = Item.SourceTile;
	Effect.SourceTileId = 0;

	switch (Item.Kind) {
	case ItemKind::Coin:
		Effect.Type = TileEffectType::AddCoin;
		Effect.Value = 1;
		Effects.push_back(Effect);
		Effect.Type = TileEffectType::AddScore;
		Effect.Value = 100;
		Effects.push_back(Effect);
		break;
	case ItemKind::Healing:
		Effect.Type = TileEffectType::AddHealth;
		Effect.Value = 1;
		Effects.push_back(Effect);
		Effect.Type = TileEffectType::AddScore;
		Effect.Value = 1000;
		Effects.push_back(Effect);
		break;
	case ItemKind::OneUp:
		Effect.Type = TileEffectType::AddLife;
		Effect.Value = 1;
		Effects.push_back(Effect);
		break;
	}
}

std::vector<TileEffect> ItemSystem::Update(float Gravity) {
	std::vector<TileEffect> Effects;
	for (std::size_t Index = 0; Index < Items_.size(); ++Index) {
		SpawnedItem& Item = Items_[Index];
		if (!Item.Active) continue;

		Item.VelocityY += Gravity;
		Item.Position.Y += Item.VelocityY;

		// V1 の CoinItem / HealingItem / OneUpItem と同じ終了条件。
		if (Item.VelocityY >= 5.0f) {
			Item.Active = false;
			AddRewardEffects(Effects, Item);
		}
	}

	Items_.erase(
		std::remove_if(
			Items_.begin(), Items_.end(),
			[](const SpawnedItem& Item) { return !Item.Active; }),
		Items_.end());
	return Effects;
}

} // namespace uchinoko
