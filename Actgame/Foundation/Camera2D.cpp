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
	HorizontalOffsetX_ = 0.0f;
	ClampToWorld(WorldSize);
}

void Camera2D::FollowPlatformer(
	WorldPosition Target,
	float MoveDirectionX,
	WorldPosition WorldSize,
	const PlatformerCameraSettings& Settings) {
	const float HorizontalRate = (std::clamp)(
		Settings.HorizontalFollowRate,
		0.0f,
		1.0f);
	const float RightAnchor = (std::clamp)(
		Settings.HorizontalRightAnchor,
		0.0f,
		1.0f);
	const float LeftAnchor = (std::clamp)(
		Settings.HorizontalLeftAnchor,
		0.0f,
		1.0f);
	const float RightTrigger = (std::clamp)(
		Settings.HorizontalRightTrigger,
		0.0f,
		1.0f);
	const float LeftTrigger = (std::clamp)(
		Settings.HorizontalLeftTrigger,
		0.0f,
		1.0f);

	const float TargetViewX = Target.X - Position_.X;
	bool FollowHorizontal = false;
	float DesiredX = Position_.X;

	if (MoveDirectionX > 0.0f &&
		TargetViewX > ViewSize_.X * RightTrigger) {
		DesiredX = Target.X - ViewSize_.X * RightAnchor;
		FollowHorizontal = true;
	} else if (MoveDirectionX < 0.0f &&
		TargetViewX < ViewSize_.X * LeftTrigger) {
		DesiredX = Target.X - ViewSize_.X * LeftAnchor;
		FollowHorizontal = true;
	}

	if (FollowHorizontal) {
		Position_.X += (DesiredX - Position_.X) * HorizontalRate;
	}

	// debug用。中心基準でPlayerがどちら側にいるかをworld unitで保持する。
	HorizontalOffsetX_ =
		Target.X - (Position_.X + ViewSize_.X * 0.5f);

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
