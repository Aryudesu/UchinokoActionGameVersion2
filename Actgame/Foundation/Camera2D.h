#pragma once

#include "StageTypes.h"

namespace uchinoko {

// World座標上の表示領域を表すDxLib非依存の2Dカメラ。
// ViewSizeはscreen pixelではなくworld unitで指定する。
class Camera2D {
public:
	Camera2D() = default;
	Camera2D(WorldPosition Position, WorldPosition ViewSize)
		: Position_(Position), ViewSize_(ViewSize) {}

	WorldPosition Position() const { return Position_; }
	WorldPosition ViewSize() const { return ViewSize_; }

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

private:
	WorldPosition Position_ = {0.0f, 0.0f};
	WorldPosition ViewSize_ = {512.0f, 320.0f};
};

} // namespace uchinoko
