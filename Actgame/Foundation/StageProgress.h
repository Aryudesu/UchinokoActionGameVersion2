#pragma once

#include "GoalState.h"

#include <vector>

namespace uchinoko {

enum class ClearRequirement {
	Normal,
	Secret,
	Either,
	Both
};

// ステージ単位の永続的なクリア履歴。
// 1プレイ中の終了状態は StageCompletionState、
// 通常/裏ゴールの意味と1ステージ分の状態は GoalState.h を正本とする。
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

} // namespace uchinoko
