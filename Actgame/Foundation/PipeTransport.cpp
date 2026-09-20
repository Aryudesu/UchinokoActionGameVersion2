#include "PipeTransport.h"

#include <cmath>

namespace uchinoko {

bool PipeNetwork::Add(const PipeLink& Link) {
	for (std::size_t Index = 0; Index < Links_.size(); ++Index) {
		if (Links_[Index].Id == Link.Id) return false;
	}
	Links_.push_back(Link);
	return true;
}

WorldPosition PipeTransport::DirectionVector(PipeDirection Direction) {
	switch (Direction) {
	case PipeDirection::Down: return {0.0f, 1.0f};
	case PipeDirection::Up: return {0.0f, -1.0f};
	case PipeDirection::Right: return {1.0f, 0.0f};
	case PipeDirection::Left: return {-1.0f, 0.0f};
	}
	return {0.0f, 0.0f};
}

bool PipeTransport::MatchesInput(
	PipeDirection Direction, const CharacterInput& Input) {
	switch (Direction) {
	case PipeDirection::Down: return Input.Vertical > 0.5f;
	case PipeDirection::Up: return Input.Vertical < -0.5f;
	case PipeDirection::Right: return Input.Horizontal > 0.5f;
	case PipeDirection::Left: return Input.Horizontal < -0.5f;
	}
	return false;
}

void PipeTransport::Reset() {
	Phase_ = PipeTransportPhase::Idle;
	Frame_ = 0;
	HasCurrentLink_ = false;
	CurrentLink_ = PipeLink();
	TransferPending_ = false;
	TransferRequest_ = PipeTransferRequest();
	EmergenceTarget_ = {};
	EmergenceDirection_ = PipeDirection::Up;
}

bool PipeTransport::TryBegin(
	const CharacterInput& Input,
	const CharacterController& Player,
	const PipeNetwork& Network,
	float Tolerance) {
	if (IsActive()) return false;

	const WorldPosition Position = Player.Body().Position;
	for (std::size_t Index = 0; Index < Network.Links().size(); ++Index) {
		const PipeLink& Link = Network.Links()[Index];
		if (!MatchesInput(Link.EnterDirection, Input)) continue;
		// V1の左右土管は MoveX 内で Land 条件付き。
		if ((Link.EnterDirection == PipeDirection::Right ||
			 Link.EnterDirection == PipeDirection::Left) &&
			!Player.Body().Grounded) continue;
		if (std::fabs(Position.X - Link.EntryPosition.X) > Tolerance) continue;
		if (std::fabs(Position.Y - Link.EntryPosition.Y) > Tolerance) continue;

		CurrentLink_ = Link;
		HasCurrentLink_ = true;
		Phase_ = PipeTransportPhase::Entering;
		Frame_ = 0;
		TransferPending_ = false;
		return true;
	}
	return false;
}

void PipeTransport::Update(CharacterController& Player) {
	if (Phase_ == PipeTransportPhase::Entering) {
		if (!HasCurrentLink_) {
			Reset();
			return;
		}
		const WorldPosition Vector = DirectionVector(CurrentLink_.EnterDirection);
		WorldPosition Next = Player.Body().Position;
		Next.X += Vector.X;
		Next.Y += Vector.Y;
		Player.Reposition(Next, true);
		++Frame_;

		if (Frame_ >= TransitionFrames) {
			Phase_ = PipeTransportPhase::WaitingForTransfer;
			Frame_ = 0;
			TransferPending_ = true;
			TransferRequest_.LinkId = CurrentLink_.Id;
			TransferRequest_.TargetStage = CurrentLink_.TargetStage;
			TransferRequest_.ExitPosition = CurrentLink_.ExitPosition;
			TransferRequest_.ExitDirection = CurrentLink_.ExitDirection;
		}
		return;
	}

	if (Phase_ == PipeTransportPhase::Emerging) {
		const WorldPosition Vector = DirectionVector(EmergenceDirection_);
		WorldPosition Next = Player.Body().Position;
		Next.X += Vector.X;
		Next.Y += Vector.Y;
		Player.Reposition(Next, true);
		++Frame_;

		if (Frame_ >= TransitionFrames) {
			Player.Reposition(EmergenceTarget_, true);
			Phase_ = PipeTransportPhase::Idle;
			Frame_ = 0;
			HasCurrentLink_ = false;
		}
	}
}

void PipeTransport::BeginEmergence(
	CharacterController& Player,
	WorldPosition ExitPosition,
	PipeDirection ExitDirection) {
	EmergenceTarget_ = ExitPosition;
	EmergenceDirection_ = ExitDirection;
	const WorldPosition Vector = DirectionVector(ExitDirection);

	WorldPosition Start = ExitPosition;
	Start.X -= Vector.X * static_cast<float>(TransitionFrames);
	Start.Y -= Vector.Y * static_cast<float>(TransitionFrames);
	Player.Reposition(Start, true);

	TransferPending_ = false;
	TransferRequest_ = PipeTransferRequest();
	Phase_ = PipeTransportPhase::Emerging;
	Frame_ = 0;
}

} // namespace uchinoko
