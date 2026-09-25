#pragma once

#include "Coordinates.h"

namespace uchinoko {

struct PlatformerCameraSettings {
	// 進行方向側を広く見せるための最大先読み量(world unit)。
	float HorizontalLookAhead = 64.0f;
	// 1 frameでLookAhead差分の何割を追うか。0..1。
	float HorizontalLookAheadRate = 0.10f;

	// Playerの基準位置。0=上端, 1=下端。
	float VerticalAnchor = 0.65f;
	// Anchorからこの範囲内ではYカメラを動かさない。
	float VerticalDeadZoneUp = 48.0f;
	float VerticalDeadZoneDown = 32.0f;
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
	float LookAheadX() const { return LookAheadX_; }

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
	// Xは進行方向を先読みし、YはDead Zoneを越えた分だけ追従する。
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
	float LookAheadX_ = 0.0f;
};

} // namespace uchinoko
