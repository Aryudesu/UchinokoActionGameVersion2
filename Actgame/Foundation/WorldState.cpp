#include "WorldState.h"

#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

void WorldState::Reset(int SwitchCount, bool InitialValue) {
	if (SwitchCount < 0) SwitchCount = 0;
	Switches_.assign(static_cast<std::size_t>(SwitchCount), InitialValue);
	AutoToggleCounters_.assign(static_cast<std::size_t>(SwitchCount), 0);
}

void WorldState::EnsureSwitch(int Channel) {
	if (Channel < 0) return;
	if (Channel >= static_cast<int>(Switches_.size())) {
		Switches_.resize(static_cast<std::size_t>(Channel + 1), false);
	}
	if (Channel >= static_cast<int>(AutoToggleCounters_.size())) {
		AutoToggleCounters_.resize(static_cast<std::size_t>(Channel + 1), 0);
	}
}

bool WorldState::GetSwitch(int Channel) const {
	if (Channel < 0 || Channel >= static_cast<int>(Switches_.size())) return false;
	return Switches_[Channel];
}

void WorldState::SetSwitch(int Channel, bool Value) {
	if (Channel < 0) return;
	EnsureSwitch(Channel);
	Switches_[Channel] = Value;
}

bool WorldState::ToggleSwitch(int Channel) {
	if (Channel < 0) return false;
	EnsureSwitch(Channel);
	Switches_[Channel] = !Switches_[Channel];
	return Switches_[Channel];
}

WorldStateUpdate WorldState::ApplyEffects(
	const std::vector<TileEffect>& Effects,
	TileMap& Map, const TileCatalog& Catalog) {
	for (std::size_t Index = 0; Index < Effects.size(); ++Index) {
		if (Effects[Index].Type == TileEffectType::ToggleSwitch) {
			ToggleSwitch(Effects[Index].Value);
		}
	}
	return Synchronize(Map, Catalog);
}

int WorldState::GetAutoToggleCounter(int Channel) const {
	if (Channel < 0 || Channel >= static_cast<int>(AutoToggleCounters_.size())) return 0;
	return AutoToggleCounters_[Channel];
}

WorldStateUpdate WorldState::AdvanceFrame(
	TileMap& Map, const TileCatalog& Catalog) {
	std::vector<int> Periods(Switches_.size(), 0);

	for (int Row = 0; Row < Map.Height(); ++Row) {
		for (int Column = 0; Column < Map.Width(); ++Column) {
			const int* Id = Map.TryGet({Column, Row});
			const TileDefinition* Definition =
				Id == nullptr ? nullptr : Catalog.Find(*Id);
			if (Definition == nullptr ||
				Definition->SwitchChannel < 0 ||
				Definition->AutoTogglePeriod <= 0) continue;

			EnsureSwitch(Definition->SwitchChannel);
			if (Periods.size() < Switches_.size()) {
				Periods.resize(Switches_.size(), 0);
			}
			const int Channel = Definition->SwitchChannel;
			if (Periods[Channel] == 0) {
				Periods[Channel] = Definition->AutoTogglePeriod;
			}
		}
	}

	bool Changed = false;
	for (std::size_t Channel = 0; Channel < Periods.size(); ++Channel) {
		const int Period = Periods[Channel];
		if (Period <= 0) continue;

		++AutoToggleCounters_[Channel];
		if (AutoToggleCounters_[Channel] >= Period) {
			AutoToggleCounters_[Channel] = 0;
			Switches_[Channel] = !Switches_[Channel];
			Changed = true;
		}
	}

	return Changed ? Synchronize(Map, Catalog) : WorldStateUpdate();
}

WorldStateUpdate WorldState::Synchronize(
	TileMap& Map, const TileCatalog& Catalog) const {
	WorldStateUpdate Update;

	for (int Row = 0; Row < Map.Height(); ++Row) {
		for (int Column = 0; Column < Map.Width(); ++Column) {
			const TilePosition Position = {Column, Row};
			int* CurrentId = Map.TryGet(Position);
			if (CurrentId == nullptr) continue;

			const TileDefinition* Current = Catalog.Find(*CurrentId);
			if (Current == nullptr || Current->SwitchChannel < 0) continue;

			const int DesiredId = GetSwitch(Current->SwitchChannel)
				? Current->SwitchOnTileId
				: Current->SwitchOffTileId;
			if (DesiredId < 0 || DesiredId == *CurrentId) continue;

			const TileDefinition* Desired = Catalog.Find(DesiredId);
			if (Desired == nullptr) continue;

			const bool WasSolid = Current->Collision == CollisionShape::Solid;
			const bool IsSolid = Desired->Collision == CollisionShape::Solid;
			*CurrentId = DesiredId;
			Update.ChangedTiles.push_back(Position);
			if (!WasSolid && IsSolid) {
				Update.ActivatedSolidTiles.push_back(Position);
			}
		}
	}

	return Update;
}

} // namespace uchinoko
