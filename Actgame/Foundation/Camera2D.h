#pragma once

#include "Coordinates.h"

namespace uchinoko {

struct PlatformerCameraSettings {
	// Xは向きが変わっただけでは追従せず、PlayerがTriggerを越えてから
	// 進行方向側を広く見せるAnchor位置へCameraを寄せる。
	// 0=左端, 1=右端。
	float HorizontalRightAnchor = 0.45f;
	float HorizontalLeftAnchor = 0.55f;
	float HorizontalRightTrigger = 0.55f;
	float HorizontalLeftTrigger = 0.45f;
	// Trigger後、1 frameで目標Camera位置との差分の何割を追うか。0..1。
	float HorizontalFollowRate = 0.08f;

	// Playerの基準位置。0=上端, 1=下端。
	float VerticalAnchor = 0.55f;
	// Anchorからこの範囲内ではYカメラを動かさない。
	float VerticalDeadZoneUp = 64.0f;
	float VerticalDeadZoneDown = 16.0f;
};

// World座標上の表示領域を表すDxLib非依存の2Dカメラ。
// ViewSizeはscreen pixelではなくworld unitで指定する。
class Camera2D {
public:
	Camera2D() = default;
	Camera2D(WorldPosition Position, WorldPosition ViewSize)
		: Position_(Position), ViewSize_(ViewSize) {}

	WorldPosition Position() const { return Position_; }
	WorldPosition ViewSize() const { return ViewSize_; }
	float HorizontalOffsetX() const { return HorizontalOffsetX_; }
	int HorizontalFollowDirection() const { return HorizontalFollowDirection_; }

	void SetPosition(WorldPosition Position) { Position_ = Position; }
	void SetViewSize(WorldPosition ViewSize) { ViewSize_ = ViewSize; }

	WorldPosition WorldToView(WorldPosition World) const {
		return {World.X - Position_.X, World.Y - Position_.Y};
	}

	WorldPosition ViewToWorld(WorldPosition View) const {
		return {View.X + Position_.X, View.Y + Position_.Y};
	}

	// Targetの中心をViewport中央へ合わせ、WorldSize内へ収める。
	// WorldがViewportより小さい軸は原点へ固定する。
	void FollowCentered(WorldPosition Target, WorldPosition WorldSize);

	// 横スクロールアクション向け追従。
	// Xは方向別Triggerを越えた後だけAnchorへ追従し、YはDead Zoneを越えた分だけ追従する。
	// MoveDirectionX: -1=左, 0=停止, +1=右（速度値をそのまま渡してもよい）。
	void FollowPlatformer(
		WorldPosition Target,
		float MoveDirectionX,
		WorldPosition WorldSize,
		const PlatformerCameraSettings& Settings = {});

private:
	void ClampToWorld(WorldPosition WorldSize);

	WorldPosition Position_ = {0.0f, 0.0f};
	WorldPosition ViewSize_ = {512.0f, 320.0f};
	float HorizontalOffsetX_ = 0.0f;
	int HorizontalFollowDirection_ = 0;
};

} // namespace uchinoko
