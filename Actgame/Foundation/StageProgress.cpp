#include "StageProgress.h"

namespace uchinoko {

bool StageClearState::IsCleared(GoalKind Kind) const {
	switch (Kind) {
	case GoalKind::Normal:
		return NormalCleared;
	case GoalKind::Secret:
		return SecretCleared;
	}
	return false;
}

bool StageClearState::Satisfies(ClearRequirement Requirement) const {
	switch (Requirement) {
	case ClearRequirement::Normal:
		return NormalCleared;
	case ClearRequirement::Secret:
		return SecretCleared;
	case ClearRequirement::Either:
		return NormalCleared || SecretCleared;
	case ClearRequirement::Both:
		return NormalCleared && SecretCleared;
	}
	return false;
}

void StageProgress::Reset() {
	Stages_.clear();
}

bool StageProgress::MarkCleared(int StageId, GoalKind Kind) {
	if (StageId < 0) return false;
	if (static_cast<int>(Stages_.size()) <= StageId) {
		Stages_.resize(static_cast<std::size_t>(StageId + 1));
	}

	StageClearState& State = Stages_[static_cast<std::size_t>(StageId)];
	bool* Flag = nullptr;
	switch (Kind) {
	case GoalKind::Normal:
		Flag = &State.NormalCleared;
		break;
	case GoalKind::Secret:
		Flag = &State.SecretCleared;
		break;
	}

	if (Flag == nullptr || *Flag) return false;
	*Flag = true;
	return true;
}

const StageClearState* StageProgress::TryGet(int StageId) const {
	if (StageId < 0 || static_cast<int>(Stages_.size()) <= StageId) return nullptr;
	return &Stages_[static_cast<std::size_t>(StageId)];
}

StageClearState StageProgress::GetOrDefault(int StageId) const {
	const StageClearState* State = TryGet(StageId);
	return State == nullptr ? StageClearState() : *State;
}

bool StageProgress::IsCleared(int StageId, GoalKind Kind) const {
	const StageClearState* State = TryGet(StageId);
	return State != nullptr && State->IsCleared(Kind);
}

bool StageProgress::Satisfies(
	int StageId, ClearRequirement Requirement) const {
	const StageClearState* State = TryGet(StageId);
	return State != nullptr && State->Satisfies(Requirement);
}

bool TryParseGoalKind(int Value, GoalKind& Kind) {
	switch (Value) {
	case 0:
		Kind = GoalKind::Normal;
		return true;
	case 1:
		Kind = GoalKind::Secret;
		return true;
	default:
		return false;
	}
}

int GoalKindValue(GoalKind Kind) {
	return static_cast<int>(Kind);
}

} // namespace uchinoko
