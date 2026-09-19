#include "TileDefinition.h"

#include <utility>

#include <algorithm>

namespace uchinoko {

Result<bool> TileCatalog::Register(TileDefinition Definition) {
	if (Find(Definition.Id) != nullptr) {
		return Result<bool>::Failure("Tile ID " + std::to_string(Definition.Id) + " is already registered");
	}

	// 既存の breakable / damaging は新しいルール形式へ一度だけ変換する。
	// これにより旧CSVを壊さず、新規タイルは Rules の組み合わせだけで表現できる。
	if (Definition.Breakable) {
		bool Exists = false;
		for (std::size_t Index = 0; Index < Definition.Rules.size(); ++Index) {
			if (Definition.Rules[Index].Trigger == TileTrigger::HitFromBelow &&
				Definition.Rules[Index].Action == TileAction::BreakTile) {
				Exists = true;
				break;
			}
		}
		if (!Exists) {
			TileRule Rule;
			Rule.Trigger = TileTrigger::HitFromBelow;
			Rule.Action = TileAction::BreakTile;
			Rule.Value = 0;
			Rule.Once = true;
			Definition.Rules.push_back(Rule);
		}
	}
	if (Definition.Damaging) {
		bool Exists = false;
		for (std::size_t Index = 0; Index < Definition.Rules.size(); ++Index) {
			if (Definition.Rules[Index].Trigger == TileTrigger::Touch &&
				Definition.Rules[Index].Action == TileAction::Damage) {
				Exists = true;
				break;
			}
		}
		if (!Exists) {
			TileRule Rule;
			Rule.Trigger = TileTrigger::Touch;
			Rule.Action = TileAction::Damage;
			Rule.Value = 1;
			Rule.Once = false;
			Definition.Rules.push_back(Rule);
		}
	}

	Definitions_.push_back(std::move(Definition));
	return Result<bool>::Success(true);
}

const TileDefinition* TileCatalog::Find(int Id) const {
	for (const TileDefinition& Definition : Definitions_) {
		if (Definition.Id == Id) return &Definition;
	}
	return nullptr;
}

} // namespace uchinoko
