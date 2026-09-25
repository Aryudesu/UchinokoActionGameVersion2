#include "Camera2D.h"

#include <algorithm>

namespace uchinoko {

void Camera2D::FollowCentered(
	WorldPosition Target,
	WorldPosition WorldSize) {
	const float MaxX = (std::max)(0.0f, WorldSize.X - ViewSize_.X);
	const float MaxY = (std::max)(0.0f, WorldSize.Y - ViewSize_.Y);

	const float DesiredX = Target.X - ViewSize_.X * 0.5f;
	const float DesiredY = Target.Y - ViewSize_.Y * 0.5f;

	Position_.X = (std::clamp)(DesiredX, 0.0f, MaxX);
	Position_.Y = (std::clamp)(DesiredY, 0.0f, MaxY);
}

} // namespace uchinoko
