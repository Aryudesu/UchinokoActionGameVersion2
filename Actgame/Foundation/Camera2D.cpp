#include "Camera2D.h"

#include <algorithm>

namespace uchinoko {

void Camera2D::ClampToWorld(WorldPosition WorldSize) {
	const float MaxX = (std::max)(0.0f, WorldSize.X - ViewSize_.X);
	const float MaxY = (std::max)(0.0f, WorldSize.Y - ViewSize_.Y);
	Position_.X = (std::clamp)(Position_.X, 0.0f, MaxX);
	Position_.Y = (std::clamp)(Position_.Y, 0.0f, MaxY);
}

void Camera2D::FollowCentered(
	WorldPosition Target,
	WorldPosition WorldSize) {
	Position_.X = Target.X - ViewSize_.X * 0.5f;
	Position_.Y = Target.Y - ViewSize_.Y * 0.5f;
	LookAheadX_ = 0.0f;
	ClampToWorld(WorldSize);
}

void Camera2D::FollowPlatformer(
	WorldPosition Target,
	float MoveDirectionX,
	WorldPosition WorldSize,
	const PlatformerCameraSettings& Settings) {
	const float Rate = (std::clamp)(
		Settings.HorizontalLookAheadRate,
		0.0f,
		1.0f);

	float TargetLookAhead = LookAheadX_;
	if (MoveDirectionX > 0.0f) {
		TargetLookAhead = Settings.HorizontalLookAhead;
	} else if (MoveDirectionX < 0.0f) {
		TargetLookAhead = -Settings.HorizontalLookAhead;
	}
	LookAheadX_ += (TargetLookAhead - LookAheadX_) * Rate;

	// LookAheadが正ならCameraを右へ送り、Playerを画面左側へ寄せる。
	Position_.X =
		Target.X - ViewSize_.X * 0.5f + LookAheadX_;

	const float Anchor = (std::clamp)(
		Settings.VerticalAnchor,
		0.0f,
		1.0f);
	const float AnchorWorldY =
		Position_.Y + ViewSize_.Y * Anchor;
	const float TopLimit =
		AnchorWorldY - (std::max)(0.0f, Settings.VerticalDeadZoneUp);
	const float BottomLimit =
		AnchorWorldY + (std::max)(0.0f, Settings.VerticalDeadZoneDown);

	if (Target.Y < TopLimit) {
		Position_.Y -= TopLimit - Target.Y;
	} else if (Target.Y > BottomLimit) {
		Position_.Y += Target.Y - BottomLimit;
	}

	ClampToWorld(WorldSize);
}

} // namespace uchinoko
