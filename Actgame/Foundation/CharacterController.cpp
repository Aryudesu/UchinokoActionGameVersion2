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
bool IsSlopeShape(CollisionShape Shape) {
	return Shape != CollisionShape::None && Shape != CollisionShape::Solid &&
		Shape != CollisionShape::OneWay;
}
} // namespace

CharacterController::CharacterController(CharacterBody Body, CharacterMotion Motion)
	: Body_(Body), Motion_(Motion) {}

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
	const float BodyTop = Body_.Position.Y + ContactMargin;
	const float BodyBottom = Body_.Position.Y + Body_.Height - ContactMargin;
	return BodyBottom > BlockTop + ContactMargin && BodyTop < BlockBottom - ContactMargin;
}

bool CharacterController::IsBlockedAtCenterSide(
	const TileMap& Map, const TileCatalog& Catalog,
	int Column, bool TargetLeftSide) const {
	const int TopRow = TileAt(Body_.Position.Y + ContactMargin, Map.TileHeight());
	const int BottomRow = TileAt(Body_.Position.Y + Body_.Height - ContactMargin, Map.TileHeight());
	for (int Row = TopRow; Row <= BottomRow; ++Row) {
		if (IsSideBlocked(Map, Catalog, Column, Row, TargetLeftSide)) return true;
	}
	return false;
}

bool CharacterController::FindGroundAtCenter(
	float FootY, float MaxRise, float MaxDrop,
	float MinimumSurfaceY, const TileMap& Map, const TileCatalog& Catalog, GroundHit& Hit) const {
	const float CenterX = Body_.Position.X + Body_.Width * 0.5f;
	if (!TerrainCollision::FindGround(
		Map, Catalog, {CenterX, FootY}, MaxRise, MaxDrop, Hit)) return false;
	return Hit.SurfaceY >= MinimumSurfaceY;
}

bool CharacterController::FollowGround(
	float HorizontalAmount, const TileMap& Map, const TileCatalog& Catalog,
	GroundHit* FollowedGround) {
	const float FootY = Body_.Position.Y + Body_.Height;
	const float FollowDistance = std::fabs(HorizontalAmount) * 2.0f + 1.0f;
	GroundHit Hit;
	if (!FindGroundAtCenter(FootY, FollowDistance, FollowDistance,
		Body_.Position.Y - ContactMargin, Map, Catalog, Hit)) return false;
	if (FollowedGround != nullptr) *FollowedGround = Hit;
	Body_.Position.Y = Hit.SurfaceY - Body_.Height;
	Body_.Velocity.Y = 0.0f;
	Body_.Grounded = true;
	return true;
}

void CharacterController::MoveHorizontal(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount == 0.0f) {
		if (Body_.Grounded && !FollowGround(0.0f, Map, Catalog)) Body_.Grounded = false;
		return;
	}
	const float OldY = Body_.Position.Y;
	const bool WasGrounded = Body_.Grounded;
	const float HalfWidth = Body_.Width * 0.5f;
	Body_.Position.X += Amount;
	const int NewColumn = TileAt(Body_.Position.X + HalfWidth, Map.TileWidth());

	// CanvasMasao と同じく、接地中は先に移動先の坂面へ追従する。
	// 1フレームで追従できない高さだけを側面として止める。
	GroundHit FollowedGround;
	if (WasGrounded && FollowGround(Amount, Map, Catalog, &FollowedGround)) {
		const bool MovingRight = Amount > 0.0f;
		if (IsSlopeShape(FollowedGround.Shape) ||
			!IsBlockedAtCenterSide(Map, Catalog, NewColumn, MovingRight)) {
			const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - Body_.Width;
			Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
			return;
		}
	}

	Body_.Position.Y = OldY;
	Body_.Grounded = WasGrounded;
	const bool MovingRight = Amount > 0.0f;
	// 追従に失敗した時点で、同じ列内に既に入っていても側面を確認する。
	// 坂下りは上の FollowGround で確定するため、入口へ押し戻されない。
	if (IsBlockedAtCenterSide(Map, Catalog, NewColumn, MovingRight)) {
		Body_.Position.X = MovingRight
			? static_cast<float>(NewColumn * Map.TileWidth()) - HalfWidth - ContactMargin
			: static_cast<float>((NewColumn + 1) * Map.TileWidth()) - HalfWidth + ContactMargin;
		Body_.Velocity.X = 0.0f;
	}
	if (WasGrounded && !FollowGround(Amount, Map, Catalog)) Body_.Grounded = false;
	const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - Body_.Width;
	Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
}

bool CharacterController::IsCeilingTile(
	const TileMap& Map, const TileCatalog& Catalog,
	int Column, int Row, bool IncludeSlopes) const {
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	if (Definition == nullptr) return false;
	if (Definition->Collision == CollisionShape::Solid) return true;
	return IncludeSlopes && IsSlopeShape(Definition->Collision);
}

void CharacterController::MoveVertical(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	const float OldTop = Body_.Position.Y;
	const float OldBottom = Body_.Position.Y + Body_.Height;
	if (Amount >= 0.0f) {
		Body_.Position.Y += Amount;
		const float NewBottom = Body_.Position.Y + Body_.Height;
		GroundHit Hit;
		// 移動前後で足元が横切った面だけに着地する。
		if (FindGroundAtCenter(OldBottom, 0.0f,
			NewBottom - OldBottom + ContactMargin, OldBottom - ContactMargin,
			Map, Catalog, Hit)) {
			Body_.Position.Y = Hit.SurfaceY - Body_.Height;
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = true;
		} else {
			Body_.Grounded = false;
		}
		return;
	}

	const float NewTop = OldTop + Amount;
	const float CenterX = Body_.Position.X + Body_.Width * 0.5f;
	const float ProbeXs[] = {CenterX, CenterX - 1.0f, CenterX + 1.0f};
	const int OldRow = TileAt(OldTop, Map.TileHeight());
	const int NewRow = TileAt(NewTop, Map.TileHeight());
	for (int Row = OldRow; Row >= NewRow; --Row) {
		const bool EnteredRow = Row < OldRow;
		bool Blocked = false;
		for (float ProbeX : ProbeXs) {
			const int Column = TileAt(ProbeX, Map.TileWidth());
			if (IsCeilingTile(Map, Catalog, Column, Row, EnteredRow)) {
				Blocked = true;
				break;
			}
		}
		if (Blocked) {
			Body_.Position.Y = static_cast<float>((Row + 1) * Map.TileHeight());
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = false;
			return;
		}
	}
	Body_.Position.Y = NewTop;
	Body_.Grounded = false;
}

void CharacterController::Step(
	float HorizontalInput, bool JumpPressed, const TileMap& Map, const TileCatalog& Catalog) {
	HorizontalInput = std::max(-1.0f, std::min(1.0f, HorizontalInput));
	Body_.Velocity.X = HorizontalInput * Motion_.MoveSpeed;
	// CanvasMasao の順序: 横移動と坂追従 -> ジャンプ開始 -> 重力と縦衝突。
	MoveHorizontal(Body_.Velocity.X, Map, Catalog);
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
