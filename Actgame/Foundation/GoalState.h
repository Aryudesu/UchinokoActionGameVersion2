#pragma once

namespace uchinoko {

enum class GoalKind {
	Normal = 0,
	Secret = 1
};

inline bool TryGoalKindFromValue(int Value, GoalKind& Kind) {
	if (Value == static_cast<int>(GoalKind::Normal)) {
		Kind = GoalKind::Normal;
		return true;
	}
	if (Value == static_cast<int>(GoalKind::Secret)) {
		Kind = GoalKind::Secret;
		return true;
	}
	return false;
}

struct StageClearState {
	bool NormalCleared = false;
	bool SecretCleared = false;

	void Record(GoalKind Kind) {
		if (Kind == GoalKind::Normal) NormalCleared = true;
		else if (Kind == GoalKind::Secret) SecretCleared = true;
	}

	bool IsCleared(GoalKind Kind) const {
		return Kind == GoalKind::Normal ? NormalCleared : SecretCleared;
	}

	bool AllCleared() const {
		return NormalCleared && SecretCleared;
	}
};

} // namespace uchinoko
