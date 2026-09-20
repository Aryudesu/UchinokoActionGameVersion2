#pragma once

#include "CharacterController.h"
#include "Coordinates.h"

#include <vector>

namespace uchinoko {

enum class PipeDirection {
	Down,
	Up,
	Right,
	Left
};

struct PipeLink {
	WorldPosition EntryPosition;
	PipeDirection EnterDirection = PipeDirection::Down;
	WorldPosition ExitPosition;
	PipeDirection ExitDirection = PipeDirection::Up;
};

enum class PipeTransportPhase {
	Idle,
	Entering,
	FadeOut,
	FadeIn,
	Emerging
};

class PipeTransport {
public:
	static constexpr int TransitionFrames = 32;
	static constexpr int FadeStep = 16;

	void Reset();

	bool IsActive() const { return Phase_ != PipeTransportPhase::Idle; }
	PipeTransportPhase Phase() const { return Phase_; }
	int Frame() const { return Frame_; }
	int FadeAlpha() const { return FadeAlpha_; }

	bool TryBegin(
		const CharacterInput& Input,
		const CharacterController& Player,
		const std::vector<PipeLink>& Links,
		float Tolerance = 4.0f);

	void Update(CharacterController& Player);

	static WorldPosition DirectionVector(PipeDirection Direction);
	static bool MatchesInput(PipeDirection Direction, const CharacterInput& Input);

private:
	void MoveToExitInterior(CharacterController& Player);

	PipeTransportPhase Phase_ = PipeTransportPhase::Idle;
	int Frame_ = 0;
	int FadeAlpha_ = 0;
	PipeLink CurrentLink_;
	bool HasCurrentLink_ = false;
};

} // namespace uchinoko
