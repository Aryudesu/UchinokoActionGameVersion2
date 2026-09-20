#pragma once

#include <vector>

namespace uchinoko {

enum class GoalKind {
	Normal = 0,
	Secret = 1
};

enum class ClearRequirement {
	Normal,
	Secret,
	Either,
	Both
};

struct StageClearState {
	bool NormalCleared = false;
	bool SecretCleared = false;

	bool IsCleared(GoalKind Kind) const;
	bool Satisfies(ClearRequirement Requirement) const;
};

class StageProgress {
public:
	void Reset();

	// true: 今回初めてその種類のゴールを記録した
	// false: 不正なStageId、または既に記録済み
	bool MarkCleared(int StageId, GoalKind Kind);

	const StageClearState* TryGet(int StageId) const;
	StageClearState GetOrDefault(int StageId) const;

	bool IsCleared(int StageId, GoalKind Kind) const;
	bool Satisfies(int StageId, ClearRequirement Requirement) const;

private:
	std::vector<StageClearState> Stages_;
};

bool TryParseGoalKind(int Value, GoalKind& Kind);
int GoalKindValue(GoalKind Kind);

} // namespace uchinoko
