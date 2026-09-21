#pragma once

#include <algorithm>

namespace uchinoko {

// V1 PlayerManager / GameData の資源更新ルール。
// TileEffectの意味とは分離し、どのSceneからでも再利用できる純粋な更新処理にする。
struct PlayerResourceRules {
	static constexpr int MaxHealth = 5;
	static constexpr int CoinsPerLife = 100;
	static constexpr int MaxLives = 999;

	static void AddHealth(int Value, int& Health) {
		Health = (std::min)(MaxHealth, Health + Value);
	}

	static void AddLife(int Value, int& Lives) {
		Lives = (std::min)(MaxLives, Lives + Value);
	}

	// V1 PlayerManager::PlusCoin() と同様、
	// 1回の加算で閾値を超えた場合に1度だけ100枚を消費して1UPする。
	// 戻り値は100コイン1UPが発生したか。
	static bool AddCoin(int Value, int& Coins, int& Lives) {
		Coins += Value;
		if (Coins < CoinsPerLife) return false;

		AddLife(1, Lives);
		Coins -= CoinsPerLife;
		return true;
	}
};

} // namespace uchinoko
