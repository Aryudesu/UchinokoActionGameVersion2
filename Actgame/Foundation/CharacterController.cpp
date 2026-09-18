#include "CharacterController.h"

#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {

namespace {
constexpr float CenterX = 15.0f;
constexpr float LeftProbeX = 14.0f;
constexpr float RightProbeX = 16.0f;
constexpr float BottomY = 31.0f;
constexpr float BelowY = 32.0f;

int TileAt(float Coordinate, int TileSize) {
	return static_cast<int>(std::floor(Coordinate / static_cast<float>(TileSize)));
}

bool IsSlope(CollisionShape Shape) {
	return Shape != CollisionShape::None && Shape != CollisionShape::Solid &&
		Shape != CollisionShape::OneWay;
}

bool IsMasaoSlope(CollisionShape Shape) {
	return Shape == CollisionShape::SlopeUpRight || Shape == CollisionShape::SlopeUpLeft;
}
} // namespace

CharacterController::CharacterController(CharacterBody Body, CharacterMotion Motion)
	: Body_(Body), Motion_(Motion) {}

CollisionShape CharacterController::ShapeAt(
	const TileMap& Map, const TileCatalog& Catalog, float X, float Y) const {
	const int Column = TileAt(X, Map.TileWidth());
	const int Row = TileAt(Y, Map.TileHeight());
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	return Definition == nullptr ? CollisionShape::None : Definition->Collision;
}

bool CharacterController::IsSolidAt(
	const TileMap& Map, const TileCatalog& Catalog, float X, float Y) const {
	return ShapeAt(Map, Catalog, X, Y) == CollisionShape::Solid;
}

float CharacterController::SlopeCharacterY(
	CollisionShape Shape, int Column, int Row, float WorldX,
	const TileMap& Map) const {
	const float Left = static_cast<float>(Column * Map.TileWidth());
	const float Top = static_cast<float>(Row * Map.TileHeight());
	const int LocalX = static_cast<int>(std::floor(WorldX - Left));
	// CanvasMasao#getSakamichiY をそのまま座標式へ移したもの。
	if (Shape == CollisionShape::SlopeUpRight) return Top - LocalX;
	if (Shape == CollisionShape::SlopeUpLeft) return Top + LocalX - 31.0f;

	// 2x1・1x2 は正男にはない。正男互換コアとは分離して中心軸へ接続する。
	float SurfaceY = 0.0f;
	if (TerrainCollision::TryGetSurfaceY(
		Shape, {Column, Row}, WorldX,
		Map.TileWidth(), Map.TileHeight(), SurfaceY)) return SurfaceY - BelowY;
	return Body_.Position.Y;
}

bool CharacterController::TrySlopeCharacterY(
	const TileMap& Map, const TileCatalog& Catalog,
	float WorldX, float ProbeY, float& CharacterY, CollisionShape* FoundShape) const {
	const int Column = TileAt(WorldX, Map.TileWidth());
	const int Row = TileAt(ProbeY, Map.TileHeight());
	const CollisionShape Shape = ShapeAt(Map, Catalog, WorldX, ProbeY);
	if (!IsSlope(Shape)) return false;
	if (!IsMasaoSlope(Shape)) {
		float SurfaceY = 0.0f;
		if (!TerrainCollision::TryGetSurfaceY(
			Shape, {Column, Row}, WorldX,
			Map.TileWidth(), Map.TileHeight(), SurfaceY)) return false;
		CharacterY = SurfaceY - BelowY;
		if (FoundShape != nullptr) *FoundShape = Shape;
		return true;
	}
	CharacterY = SlopeCharacterY(Shape, Column, Row, WorldX, Map);
	if (FoundShape != nullptr) *FoundShape = Shape;
	return true;
}

void CharacterController::RefreshGround(
	const TileMap& Map, const TileCatalog& Catalog) {
	const float X = Body_.Position.X + CenterX;
	const float FootY = Body_.Position.Y + BottomY;
	Body_.Grounded = IsSolidAt(Map, Catalog, X, Body_.Position.Y + BelowY);

	float SlopeY = 0.0f;
	if (TrySlopeCharacterY(Map, Catalog, X, FootY, SlopeY) &&
		SlopeY <= Body_.Position.Y) {
		Body_.Position.Y = SlopeY;
		if (Body_.Velocity.Y >= 0.0f) Body_.Grounded = true;
	}
	if (Body_.Grounded && Body_.Velocity.Y > 0.0f) Body_.Velocity.Y = 0.0f;
}

void CharacterController::ResolveHorizontalWall(
	float OldCenterX, bool MovingRight,
	const TileMap& Map, const TileCatalog& Catalog) {
	const float NewCenterX = Body_.Position.X + CenterX;
	const int OldColumn = TileAt(OldCenterX, Map.TileWidth());
	const int NewColumn = TileAt(NewCenterX, Map.TileWidth());
	const float ProbeYs[] = {Body_.Position.Y, Body_.Position.Y + BottomY};
	bool Blocked = false;

	for (float ProbeY : ProbeYs) {
		const CollisionShape Shape = ShapeAt(Map, Catalog, NewCenterX, ProbeY);
		if (Shape == CollisionShape::Solid) {
			Blocked = true;
			break;
		}
		if (OldColumn != NewColumn) {
			const CollisionShape BlockingSlope = MovingRight
				? CollisionShape::SlopeUpLeft : CollisionShape::SlopeUpRight;
			const float SlopeY = Shape == BlockingSlope
				? SlopeCharacterY(Shape, NewColumn,
					TileAt(ProbeY, Map.TileHeight()), NewCenterX, Map)
				: Body_.Position.Y;
			if (Shape == BlockingSlope && Body_.Position.Y > SlopeY) {
				Blocked = true;
				break;
			}
		}
		if (!IsSlope(Shape) || IsMasaoSlope(Shape)) continue;
		float SurfaceY = 0.0f;
		const bool HasSurface = TerrainCollision::TryGetSurfaceY(
			Shape, {NewColumn, TileAt(ProbeY, Map.TileHeight())}, NewCenterX,
			Map.TileWidth(), Map.TileHeight(), SurfaceY);
		if (HasSurface && Body_.Grounded && Body_.Position.Y <= SurfaceY - BelowY) continue;
		if (!HasSurface && (Shape == CollisionShape::Stair1x2UpRightTop ||
			Shape == CollisionShape::Stair1x2UpLeftTop)) continue;
		float BlockTop = 0.0f;
		float BlockBottom = 0.0f;
		if (TerrainCollision::TryGetSideBlock(
			Shape, {NewColumn, TileAt(ProbeY, Map.TileHeight())},
			MovingRight ? TerrainCollision::TileSide::Left : TerrainCollision::TileSide::Right,
			Map.TileWidth(), Map.TileHeight(), BlockTop, BlockBottom) &&
			ProbeY >= BlockTop && ProbeY < BlockBottom) {
			Blocked = true;
			break;
		}
	}

	if (!Blocked) return;
	Body_.Position.X = MovingRight
		? static_cast<float>(NewColumn * Map.TileWidth()) - RightProbeX
		: static_cast<float>((NewColumn + 1) * Map.TileWidth()) - CenterX;
	Body_.Velocity.X = 0.0f;
}

void CharacterController::FollowMasaoSlopeAfterHorizontal(
	float OldX, float OldY, bool WasGrounded,
	const TileMap& Map, const TileCatalog& Catalog) {
	if (!WasGrounded) return;
	const float OldCenterX = OldX + CenterX;
	const float NewCenterX = Body_.Position.X + CenterX;
	const CollisionShape OldFootShape = ShapeAt(Map, Catalog, OldCenterX, OldY + BottomY);
	const float MaxStep = std::fabs(Body_.Position.X - OldX) * 2.0f + 1.0f;

	float SlopeY = 0.0f;
	const bool FoundCurrentSlope =
		TrySlopeCharacterY(Map, Catalog, NewCenterX, OldY + BottomY, SlopeY) ||
		TrySlopeCharacterY(Map, Catalog, NewCenterX, OldY + BelowY, SlopeY);
	if (FoundCurrentSlope && std::fabs(SlopeY - OldY) <= MaxStep) {
		Body_.Position.Y = SlopeY;
		Body_.Grounded = true;
		Body_.Velocity.Y = 0.0f;
		return;
	}
	// 正男にない複数タイル坂だけは、隣接する上下パーツから到達可能な面を探す。
	const int NewColumn = TileAt(NewCenterX, Map.TileWidth());
	const int BaseRow = TileAt(OldY + BottomY, Map.TileHeight());
	bool FoundExtended = false;
	float BestDistance = 0.0f;
	for (int Row = BaseRow - 2; Row <= BaseRow + 2; ++Row) {
		const int* Id = Map.TryGet({NewColumn, Row});
		const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr || !IsSlope(Definition->Collision) ||
			IsMasaoSlope(Definition->Collision)) continue;
		float SurfaceY = 0.0f;
		if (!TerrainCollision::TryGetSurfaceY(
			Definition->Collision, {NewColumn, Row}, NewCenterX,
			Map.TileWidth(), Map.TileHeight(), SurfaceY)) continue;
		const float CandidateY = SurfaceY - BelowY;
		const float Distance = std::fabs(CandidateY - OldY);
		if (Distance > MaxStep || (FoundExtended && Distance >= BestDistance)) continue;
		FoundExtended = true;
		BestDistance = Distance;
		SlopeY = CandidateY;
	}
	if (FoundExtended) {
		Body_.Position.Y = SlopeY;
		Body_.Grounded = true;
		Body_.Velocity.Y = 0.0f;
		return;
	}
	const int NewFootRow = TileAt(OldY + BottomY, Map.TileHeight());
	if (ShapeAt(Map, Catalog, NewCenterX, OldY + BottomY) == CollisionShape::Solid &&
		IsSlope(OldFootShape)) {
		// 坂の高い端から同じ行の平地へ移る分岐。
		Body_.Position.Y = static_cast<float>(NewFootRow * Map.TileHeight()) - BelowY;
		Body_.Grounded = true;
		Body_.Velocity.Y = 0.0f;
		return;
	}

	// 坂を出たフレームは、正男と同じく直下のタイル行だけを見る。
	if (IsSlope(OldFootShape)) {
		const int BelowRow = TileAt(OldY + BottomY, Map.TileHeight()) + 1;
		const float BelowProbeY = static_cast<float>(BelowRow * Map.TileHeight());
		if (IsSolidAt(Map, Catalog, NewCenterX, BelowProbeY)) {
			Body_.Position.Y = BelowProbeY - BelowY;
			Body_.Grounded = true;
			Body_.Velocity.Y = 0.0f;
		}
	}
}

void CharacterController::MoveHorizontal(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount == 0.0f) return;
	const float OldX = Body_.Position.X;
	const float OldY = Body_.Position.Y;
	const float OldCenterX = OldX + CenterX;
	const bool WasGrounded = Body_.Grounded;
	const bool MovingRight = Amount > 0.0f;
	Body_.Position.X += Amount;

	FollowMasaoSlopeAfterHorizontal(OldX, OldY, WasGrounded, Map, Catalog);
	ResolveHorizontalWall(OldCenterX, MovingRight, Map, Catalog);

	const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - BelowY;
	Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
}

void CharacterController::MoveUp(
	float Amount, float HorizontalInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	const float OldTop = Body_.Position.Y;
	const int OldRow = TileAt(OldTop, Map.TileHeight());
	Body_.Position.Y += Amount;
	const int NewRow = TileAt(Body_.Position.Y, Map.TileHeight());
	const float CenterProbeX = Body_.Position.X + CenterX;

	for (int Row = OldRow; Row >= NewRow; --Row) {
		const float ProbeY = static_cast<float>(Row * Map.TileHeight());
		const CollisionShape CenterShape = ShapeAt(Map, Catalog, CenterProbeX, ProbeY);
		const bool EnteredRow = Row < OldRow;
		bool Blocked = CenterShape == CollisionShape::Solid ||
			(EnteredRow && IsSlope(CenterShape));
		if (!Blocked && EnteredRow && HorizontalInput > 0.0f) {
			Blocked = IsSolidAt(Map, Catalog, Body_.Position.X + RightProbeX, ProbeY);
		}
		if (!Blocked && EnteredRow && HorizontalInput < 0.0f) {
			Blocked = IsSolidAt(Map, Catalog, Body_.Position.X + LeftProbeX, ProbeY);
		}
		if (!Blocked) continue;
		Body_.Position.Y = static_cast<float>((Row + 1) * Map.TileHeight());
		Body_.Velocity.Y = 0.0f;
		Body_.Grounded = false;
		return;
	}
	Body_.Grounded = false;
}

void CharacterController::MoveDown(
	float Amount, float HorizontalInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	const float OldY = Body_.Position.Y;
	const int OldRow = TileAt(OldY + BottomY, Map.TileHeight());
	Body_.Position.Y += Amount;
	const float NewFootY = Body_.Position.Y + BottomY;
	const int NewRow = TileAt(NewFootY, Map.TileHeight());
	const float CenterProbeX = Body_.Position.X + CenterX;

	float SlopeY = 0.0f;
	if (TrySlopeCharacterY(Map, Catalog, CenterProbeX, NewFootY, SlopeY) &&
		SlopeY < Body_.Position.Y && SlopeY >= OldY) {
		Body_.Position.Y = SlopeY;
		Body_.Velocity.Y = 0.0f;
		Body_.Grounded = true;
		return;
	}

	if (NewRow > OldRow) {
		const float RowProbeY = static_cast<float>(NewRow * Map.TileHeight());
		if (IsSolidAt(Map, Catalog, CenterProbeX, RowProbeY)) {
			Body_.Position.Y = RowProbeY - BelowY;
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = true;
			return;
		}
		// jM100 は下降時の方向補助判定でXも1px補正する。
		if (HorizontalInput > 0.0f && IsSolidAt(
			Map, Catalog, Body_.Position.X + RightProbeX, RowProbeY)) {
			Body_.Position.Y = RowProbeY - BelowY;
			Body_.Position.X += 1.0f;
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = true;
			return;
		}
		if (HorizontalInput < 0.0f && IsSolidAt(
			Map, Catalog, Body_.Position.X + LeftProbeX, RowProbeY)) {
			Body_.Position.Y = RowProbeY - BelowY;
			Body_.Position.X -= 1.0f;
			Body_.Velocity.Y = 0.0f;
			Body_.Grounded = true;
			return;
		}
	}
	Body_.Grounded = false;
}

void CharacterController::MoveVertical(
	float Amount, float HorizontalInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount < 0.0f) MoveUp(Amount, HorizontalInput, Map, Catalog);
	else if (Amount > 0.0f) MoveDown(Amount, HorizontalInput, Map, Catalog);
}

void CharacterController::Step(
	float HorizontalInput, bool JumpPressed,
	const TileMap& Map, const TileCatalog& Catalog) {
	HorizontalInput = std::max(-1.0f, std::min(1.0f, HorizontalInput));
	// jM100 と同じく、入力処理より前に現在座標から接地を再判定する。
	RefreshGround(Map, Catalog);
	Body_.Velocity.X = HorizontalInput * Motion_.MoveSpeed;
	MoveHorizontal(Body_.Velocity.X, Map, Catalog);

	if (JumpPressed && Body_.Grounded) {
		Body_.Velocity.Y = -Motion_.JumpSpeed;
		Body_.Grounded = false;
	}
	if (!Body_.Grounded) {
		Body_.Velocity.Y = std::min(Motion_.MaxFallSpeed, Body_.Velocity.Y + Motion_.Gravity);
		MoveVertical(Body_.Velocity.Y, HorizontalInput, Map, Catalog);
	}
}

} // namespace uchinoko
