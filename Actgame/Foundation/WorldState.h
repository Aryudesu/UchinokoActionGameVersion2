#pragma once

#include "Coordinates.h"
#include "TileInteraction.h"

#include <vector>

namespace uchinoko {

class TileCatalog;
class TileMap;

struct WorldStateUpdate {
	std::vector<TilePosition> ChangedTiles;
	std::vector<TilePosition> ActivatedSolidTiles;
};

class WorldState {
public:
	void Reset(int SwitchCount = 1, bool InitialValue = true);

	bool GetSwitch(int Channel) const;
	void SetSwitch(int Channel, bool Value);
	bool ToggleSwitch(int Channel);

	// ToggleSwitch Effectを反映した後、スイッチに束縛された全タイルを同期する。
	WorldStateUpdate ApplyEffects(
		const std::vector<TileEffect>& Effects,
		TileMap& Map, const TileCatalog& Catalog);

	WorldStateUpdate Synchronize(TileMap& Map, const TileCatalog& Catalog) const;

	// タイル定義の AutoTogglePeriod を見てタイマーを進める。
	// 周期到達時だけ対象チャネルを反転して地形を同期する。
	WorldStateUpdate AdvanceFrame(TileMap& Map, const TileCatalog& Catalog);

	int GetAutoToggleCounter(int Channel) const;

private:
	void EnsureSwitch(int Channel);
	std::vector<bool> Switches_;
	std::vector<int> AutoToggleCounters_;
};

} // namespace uchinoko
