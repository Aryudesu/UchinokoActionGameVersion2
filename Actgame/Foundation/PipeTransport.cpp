#include "PipeTransport.h"

#include <cmath>

namespace uchinoko {

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
	FadeAlpha_ = 0;
	CurrentLink_ = PipeLink();
	HasCurrentLink_ = false;
}

bool PipeTransport::TryBegin(
	const CharacterInput& Input,
	const CharacterController& Player,
	const std::vector<PipeLink>& Links,
	float Tolerance) {
	if (IsActive()) return false;

	const WorldPosition Position = Player.Body().Position;
	for (std::size_t Index = 0; Index < Links.size(); ++Index) {
		const PipeLink& Link = Links[Index];
		if (!MatchesInput(Link.EnterDirection, Input)) continue;

		if ((Link.EnterDirection == PipeDirection::Right ||
			 Link.EnterDirection == PipeDirection::Left) &&
			!Player.Body().Grounded) continue;

		if (std::fabs(Position.X - Link.EntryPosition.X) > Tolerance) continue;
		if (std::fabs(Position.Y - Link.EntryPosition.Y) > Tolerance) continue;

		CurrentLink_ = Link;
		HasCurrentLink_ = true;
		Phase_ = PipeTransportPhase::Entering;
		Frame_ = 0;
		return true;
	}
	return false;
}

void PipeTransport::MoveToExitInterior(CharacterController& Player) {
	const WorldPosition Vector = DirectionVector(CurrentLink_.ExitDirection);
	WorldPosition Start = CurrentLink_.ExitPosition;
	Start.X -= Vector.X * static_cast<float>(TransitionFrames);
	Start.Y -= Vector.Y * static_cast<float>(TransitionFrames);
	Player.Reposition(Start, true);
}

void PipeTransport::Update(CharacterController& Player) {
	if (!HasCurrentLink_) return;

	if (Phase_ == PipeTransportPhase::Entering) {
		const WorldPosition Vector = DirectionVector(CurrentLink_.EnterDirection);
		WorldPosition Next = Player.Body().Position;
		Next.X += Vector.X;
		Next.Y += Vector.Y;
		Player.Reposition(Next, true);

		++Frame_;
		if (Frame_ >= TransitionFrames) {
			Phase_ = PipeTransportPhase::FadeOut;
			Frame_ = 0;
			FadeAlpha_ = 0;
		}
		return;
	}

	if (Phase_ == PipeTransportPhase::FadeOut) {
		FadeAlpha_ += FadeStep;
		if (FadeAlpha_ >= 255) {
			FadeAlpha_ = 255;
			// 完全に暗くなった瞬間だけ出口内部へ移動する。
			MoveToExitInterior(Player);
			Phase_ = PipeTransportPhase::FadeIn;
			Frame_ = 0;
		}
		return;
	}

	if (Phase_ == PipeTransportPhase::FadeIn) {
		FadeAlpha_ -= FadeStep;
		if (FadeAlpha_ <= 0) {
			FadeAlpha_ = 0;
			Phase_ = PipeTransportPhase::Emerging;
			Frame_ = 0;
		}
		return;
	}

	if (Phase_ == PipeTransportPhase::Emerging) {
		const WorldPosition Vector = DirectionVector(CurrentLink_.ExitDirection);
		WorldPosition Next = Player.Body().Position;
		Next.X += Vector.X;
		Next.Y += Vector.Y;
		Player.Reposition(Next, true);

		++Frame_;
		if (Frame_ >= TransitionFrames) {
			Player.Reposition(CurrentLink_.ExitPosition, true);
			Reset();
		}
	}
}

} // namespace uchinoko
