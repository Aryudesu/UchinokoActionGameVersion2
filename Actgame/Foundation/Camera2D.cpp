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
	HorizontalFollowDirection_ = 0;
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

	const int MoveDirection =
		MoveDirectionX > 0.0f ? 1 :
		MoveDirectionX < 0.0f ? -1 : 0;

	// 逆方向へ振り返った瞬間はCamera追従を解除する。
	// その後、Player自身が反対側Triggerまで移動してから
	// 新しい進行方向のAnchor追従へ切り替える。
	if (MoveDirection != 0 &&
		HorizontalFollowDirection_ != 0 &&
		MoveDirection != HorizontalFollowDirection_) {
		HorizontalFollowDirection_ = 0;
	}

	const float TargetViewX = Target.X - Position_.X;
	if (HorizontalFollowDirection_ == 0) {
		if (MoveDirection > 0 &&
			TargetViewX > ViewSize_.X * RightTrigger) {
			HorizontalFollowDirection_ = 1;
		} else if (MoveDirection < 0 &&
			TargetViewX < ViewSize_.X * LeftTrigger) {
			HorizontalFollowDirection_ = -1;
		}
	}

	float DesiredX = Position_.X;
	if (HorizontalFollowDirection_ > 0) {
		DesiredX = Target.X - ViewSize_.X * RightAnchor;
	} else if (HorizontalFollowDirection_ < 0) {
		DesiredX = Target.X - ViewSize_.X * LeftAnchor;
	}

	if (HorizontalFollowDirection_ != 0) {
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
