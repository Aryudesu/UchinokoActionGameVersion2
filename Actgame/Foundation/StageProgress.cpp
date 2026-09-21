#include "StageProgress.h"

namespace uchinoko {

namespace {

bool SatisfiesRequirement(
	const StageClearState& State, ClearRequirement Requirement) {
	switch (Requirement) {
	case ClearRequirement::Normal:
		return State.NormalCleared;
	case ClearRequirement::Secret:
		return State.SecretCleared;
	case ClearRequirement::Either:
		return State.NormalCleared || State.SecretCleared;
	case ClearRequirement::Both:
		return State.AllCleared();
	}
	return false;
}

} // namespace

void StageProgress::Reset() {
	Stages_.clear();
}

bool StageProgress::MarkCleared(int StageId, GoalKind Kind) {
	if (StageId < 0) return false;
	if (static_cast<int>(Stages_.size()) <= StageId) {
		Stages_.resize(static_cast<std::size_t>(StageId + 1));
	}

	StageClearState& State = Stages_[static_cast<std::size_t>(StageId)];
	if (State.IsCleared(Kind)) return false;

	State.Record(Kind);
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
	return State != nullptr && SatisfiesRequirement(*State, Requirement);
}

} // namespace uchinoko
