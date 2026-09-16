#include "CharacterController.h"

#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {

namespace {
constexpr float ContactMargin = 0.01f;
int TileAt(float Coordinate, int TileSize) {
	return static_cast<int>(std::floor(Coordinate / static_cast<float>(TileSize)));
}

} // namespace

CharacterController::CharacterController(CharacterBody Body, CharacterMotion Motion)
	: Body_(Body), Motion_(Motion) {}

bool CharacterController::IsSolid(
	const TileMap& Map, const TileCatalog& Catalog, int Column, int Row) const {
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	return Definition != nullptr && Definition->Collision == CollisionShape::Solid;
}

bool CharacterController::IsSideBlocked(
	const TileMap& Map, const TileCatalog& Catalog,
	int Column, int Row, bool TargetLeftSide) const {
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	if (Definition == nullptr) return false;
	float BlockTop = 0.0f;
	float BlockBottom = 0.0f;
	if (!TerrainCollision::TryGetSideBlock(
		Definition->Collision, {Column, Row},
		TargetLeftSide ? TerrainCollision::TileSide::Left : TerrainCollision::TileSide::Right,
		Map.TileWidth(), Map.TileHeight(), BlockTop, BlockBottom)) return false;
	const float CenterY = Body_.Position.Y + Body_.Height * 0.5f;
	return CenterY > BlockTop + ContactMargin && CenterY < BlockBottom - ContactMargin;
}

void CharacterController::MoveHorizontal(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount == 0.0f) return;
	const float OldX = Body_.Position.X;
	Body_.Position.X += Amount;
	const float HalfWidth = Body_.Width * 0.5f;
	const int Row = TileAt(Body_.Position.Y + Body_.Height * 0.5f, Map.TileHeight());
	if (Amount > 0.0f) {
		const int OldColumn = TileAt(OldX + HalfWidth, Map.TileWidth());
		const int Column = TileAt(Body_.Position.X + HalfWidth, Map.TileWidth());
		if (Column != OldColumn) {
			if (IsSideBlocked(Map, Catalog, Column, Row, true)) {
				Body_.Position.X = static_cast<float>(Column * Map.TileWidth()) - HalfWidth - ContactMargin;
				Body_.Velocity.X = 0.0f;
			}
		}
	} else {
		const int OldColumn = TileAt(OldX + HalfWidth, Map.TileWidth());
		const int Column = TileAt(Body_.Position.X + HalfWidth, Map.TileWidth());
		if (Column != OldColumn) {
			if (IsSideBlocked(Map, Catalog, Column, Row, false)) {
				Body_.Position.X = static_cast<float>((Column + 1) * Map.TileWidth()) - HalfWidth + ContactMargin;
				Body_.Velocity.X = 0.0f;
			}
		}
	}
	const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - Body_.Width;
	Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
}

bool CharacterController::SnapToGround(
	float MaxRise, float MaxDrop, const TileMap& Map, const TileCatalog& Catalog) {
	GroundHit Hit;
	// 中央の接地点が地形へ食い込んだ場合も、身体内の接地面まで戻す。
	const float GroundSearchRise = std::max(MaxRise, Body_.Height + ContactMargin);
	if (!FindGroundAtCenter(Body_.Position.Y + Body_.Height,
		GroundSearchRise, MaxDrop, Body_.Position.Y - ContactMargin,
		Map, Catalog, Hit)) return false;
	Body_.Position.Y = Hit.SurfaceY - Body_.Height;
	Body_.Velocity.Y = 0.0f;
	Body_.Grounded = true;
	return true;
}

bool CharacterController::FindGroundAtCenter(
	float FootY, float MaxRise, float MaxDrop,
	float MinimumSurfaceY, const TileMap& Map, const TileCatalog& Catalog, GroundHit& Hit) const {
	const float CenterX = Body_.Position.X + Body_.Width * 0.5f;
	if (!TerrainCollision::FindGround(
		Map, Catalog, {CenterX, FootY}, MaxRise, MaxDrop, Hit)) return false;
	return Hit.SurfaceY >= MinimumSurfaceY;
}

void CharacterController::MoveVertical(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	const float OldBottom = Body_.Position.Y + Body_.Height;
	Body_.Position.Y += Amount;
	if (Amount >= 0.0f) {
		const float NewBottom = Body_.Position.Y + Body_.Height;
		GroundHit Hit;
		// 空中で横から坂へ入った場合も、中央点が通過した面まで戻して着地する。
		const float LandingSearch = std::max(
			NewBottom - OldBottom + ContactMargin, Body_.Height + ContactMargin);
		if (FindGroundAtCenter(NewBottom, LandingSearch, 0.0f,
			Body_.Position.Y - ContactMargin, Map, Catalog, Hit)) {
			Body_.Position.Y = Hit.SurfaceY - Body_.Height;
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = true;
		} else {
			Body_.Grounded = false;
		}
		return;
	}
	const int Row = TileAt(Body_.Position.Y, Map.TileHeight());
	const int Column = TileAt(Body_.Position.X + Body_.Width * 0.5f, Map.TileWidth());
	if (IsSolid(Map, Catalog, Column, Row)) {
		Body_.Position.Y = static_cast<float>((Row + 1) * Map.TileHeight());
		Body_.Velocity.Y = 0.0f;
	}
}

void CharacterController::Step(
	float HorizontalInput, bool JumpPressed, const TileMap& Map, const TileCatalog& Catalog) {
	HorizontalInput = std::max(-1.0f, std::min(1.0f, HorizontalInput));
	Body_.Velocity.X = HorizontalInput * Motion_.MoveSpeed;
	MoveHorizontal(Body_.Velocity.X, Map, Catalog);
	if (Body_.Grounded) {
		// 1x2 の急勾配は横移動量の2倍だけ上下する。
		const float StepDistance = std::fabs(Body_.Velocity.X) * 2.0f + 1.0f;
		if (!SnapToGround(StepDistance, StepDistance, Map, Catalog)) Body_.Grounded = false;
	}
	if (JumpPressed && Body_.Grounded) {
		Body_.Velocity.Y = -Motion_.JumpSpeed;
		Body_.Grounded = false;
	}
	if (!Body_.Grounded) {
		Body_.Velocity.Y = std::min(Motion_.MaxFallSpeed, Body_.Velocity.Y + Motion_.Gravity);
		MoveVertical(Body_.Velocity.Y, Map, Catalog);
	}
}

} // namespace uchinoko
