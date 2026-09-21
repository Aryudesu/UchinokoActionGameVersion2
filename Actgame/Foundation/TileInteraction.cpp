#include "TileInteraction.h"

#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

TileRuntimeMap::TileRuntimeMap(const TileMap& Map) {
	Reset(Map);
}

void TileRuntimeMap::Reset(const TileMap& Map) {
	States_.assign(
		static_cast<std::size_t>(Map.Height()),
		std::vector<TileRuntimeState>(static_cast<std::size_t>(Map.Width())));
}

TileRuntimeState* TileRuntimeMap::TryGet(TilePosition Position) {
	if (Position.Row < 0 || Position.Row >= static_cast<int>(States_.size())) return nullptr;
	if (Position.Column < 0 ||
		Position.Column >= static_cast<int>(States_[Position.Row].size())) return nullptr;
	return &States_[Position.Row][Position.Column];
}

const TileRuntimeState* TileRuntimeMap::TryGet(TilePosition Position) const {
	if (Position.Row < 0 || Position.Row >= static_cast<int>(States_.size())) return nullptr;
	if (Position.Column < 0 ||
		Position.Column >= static_cast<int>(States_[Position.Row].size())) return nullptr;
	return &States_[Position.Row][Position.Column];
}

namespace {

void AddEffect(
	TileBehaviorResult& Result, TileEffectType Type,
	const TileInteraction& Interaction, int Value) {
	TileEffect Effect;
	Effect.Type = Type;
	Effect.Position = Interaction.Position;
	Effect.Value = Value;
	Effect.SourceTileId = Interaction.TileId;
	Effect.Actor = Interaction.Actor;
	Result.Effects.push_back(Effect);
}

bool MatchesCurrentTile(const TileInteraction& Interaction, const TileMap& Map) {
	const int* Current = Map.TryGet(Interaction.Position);
	return Current != nullptr && *Current == Interaction.TileId;
}

bool MatchesCount(const TileRule& Rule, int Count) {
	switch (Rule.CountCondition) {
	case TileCountCondition::Any:
		return true;
	case TileCountCondition::LessThan:
		return Count < Rule.CountValue;
	case TileCountCondition::LessEqual:
		return Count <= Rule.CountValue;
	case TileCountCondition::Equal:
		return Count == Rule.CountValue;
	case TileCountCondition::GreaterEqual:
		return Count >= Rule.CountValue;
	case TileCountCondition::GreaterThan:
		return Count > Rule.CountValue;
	}
	return false;
}

bool MatchesTarget(TileTarget Target, TileActor Actor) {
	if (Target == TileTarget::Both) return true;
	if (Target == TileTarget::Player) return Actor == TileActor::Player;
	return Actor == TileActor::Enemy;
}

} // namespace

TileBehaviorResult TileBehaviorSystem::Apply(
	const TileInteraction& Interaction,
	TileMap& Map, const TileCatalog& Catalog, TileRuntimeMap& Runtime) {
	TileBehaviorResult Result;
	if (!MatchesCurrentTile(Interaction, Map)) return Result;

	const TileDefinition* Definition = Catalog.Find(Interaction.TileId);
	TileRuntimeState* State = Runtime.TryGet(Interaction.Position);
	if (Definition == nullptr || State == nullptr) return Result;

	if (State->ConsumedRules.size() < Definition->Rules.size()) {
		State->ConsumedRules.resize(Definition->Rules.size(), false);
	}

	for (std::size_t Index = 0; Index < Definition->Rules.size(); ++Index) {
		const TileRule& Rule = Definition->Rules[Index];
		if (Rule.Trigger != Interaction.Trigger) continue;
		if (!MatchesTarget(Rule.Target, Interaction.Actor)) continue;
		if (Rule.Once && State->ConsumedRules[Index]) continue;
		if (!MatchesCount(Rule, State->Count)) continue;

		Result.Handled = true;

		switch (Rule.Action) {
		case TileAction::None:
			break;
		case TileAction::ReplaceTile: {
			int* Current = Map.TryGet(Interaction.Position);
			if (Current != nullptr) *Current = Rule.Value;
			break;
		}
		case TileAction::BreakTile: {
			int* Current = Map.TryGet(Interaction.Position);
			if (Current != nullptr) *Current = Rule.Value;
			AddEffect(Result, TileEffectType::TileBroken, Interaction, Rule.Value);
			break;
		}
		case TileAction::AddCoin:
			AddEffect(Result, TileEffectType::AddCoin, Interaction, Rule.Value);
			break;
		case TileAction::AddHealth:
			AddEffect(Result, TileEffectType::AddHealth, Interaction, Rule.Value);
			break;
		case TileAction::AddLife:
			AddEffect(Result, TileEffectType::AddLife, Interaction, Rule.Value);
			break;
		case TileAction::AddScore:
			AddEffect(Result, TileEffectType::AddScore, Interaction, Rule.Value);
			break;
		case TileAction::Damage:
			AddEffect(Result, TileEffectType::Damage, Interaction, Rule.Value);
			break;
		case TileAction::InstantDeath:
			AddEffect(Result, TileEffectType::InstantDeath, Interaction, Rule.Value);
			break;
		case TileAction::SpawnItem:
			AddEffect(Result, TileEffectType::SpawnItem, Interaction, Rule.Value);
			break;
		case TileAction::IncrementCount:
			State->Count += Rule.Value;
			break;
		case TileAction::ToggleSwitch:
			AddEffect(Result, TileEffectType::ToggleSwitch, Interaction, Rule.Value);
			break;
		case TileAction::Goal:
			AddEffect(Result, TileEffectType::Goal, Interaction, Rule.Value);
			break;
		case TileAction::HitBrick:
			AddEffect(Result, TileEffectType::BrickHit, Interaction, Rule.Value);
			break;
		}
		if (Rule.Once) {
			State->ConsumedRules[Index] = true;
			State->Used = true;
		}
	}

	return Result;
}

std::vector<TileEffect> TileBehaviorSystem::ApplyAll(
	const std::vector<TileInteraction>& Interactions,
	TileMap& Map, const TileCatalog& Catalog, TileRuntimeMap& Runtime) {
	std::vector<TileEffect> Effects;
	for (std::size_t Index = 0; Index < Interactions.size(); ++Index) {
		TileBehaviorResult Result = Apply(Interactions[Index], Map, Catalog, Runtime);
		Effects.insert(Effects.end(), Result.Effects.begin(), Result.Effects.end());
	}
	return Effects;
}

} // namespace uchinoko
