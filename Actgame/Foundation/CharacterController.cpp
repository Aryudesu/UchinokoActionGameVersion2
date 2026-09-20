#include "CharacterController.h"

#include "CanvasMasaoTerrain.h"
#include "ExtendedSlopeTerrain.h"
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
		Shape != CollisionShape::OneWay &&
		Shape != CollisionShape::HitFromBelowOnly;
}

bool IsMasaoSlope(CollisionShape Shape) {
	return Shape == CollisionShape::SlopeUpRight || Shape == CollisionShape::SlopeUpLeft;
}

bool Is2x1Slope(CollisionShape Shape) {
	return Shape == CollisionShape::Stair2x1UpRightLow ||
		Shape == CollisionShape::Stair2x1UpRightHigh ||
		Shape == CollisionShape::Stair2x1UpLeftHigh ||
		Shape == CollisionShape::Stair2x1UpLeftLow;
}

bool Is1x2Slope(CollisionShape Shape) {
	return Shape == CollisionShape::Stair1x2UpRightBottom ||
		Shape == CollisionShape::Stair1x2UpRightTop ||
		Shape == CollisionShape::Stair1x2UpLeftTop ||
		Shape == CollisionShape::Stair1x2UpLeftBottom;
}
} // namespace

CharacterController::CharacterController(CharacterBody Body, CharacterMotion Motion)
	: Body_(Body), Motion_(Motion),
	  VelocityX10_(static_cast<int>(std::round(Body.Velocity.X * 10.0f))),
	  VelocityY10_(static_cast<int>(std::round(Body.Velocity.Y * 10.0f))) {}

void CharacterController::Reposition(WorldPosition Position, bool ResetVelocity) {
	Body_.Position = Position;
	Body_.Grounded = false;
	Mode_ = MovementMode::Normal;
	InWater_ = false;
	if (!ResetVelocity) return;
	Body_.Velocity = {0.0f, 0.0f};
	VelocityX10_ = 0;
	VelocityY10_ = 0;
}


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

MovementRegion CharacterController::MovementRegionAt(
	const TileMap& Map, const TileCatalog& Catalog, float X, float Y) const {
	const int Column = TileAt(X, Map.TileWidth());
	const int Row = TileAt(Y, Map.TileHeight());
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	return Definition == nullptr ? MovementRegion::None : Definition->Movement;
}

bool CharacterController::IsInsideLadder(
	const TileMap& Map, const TileCatalog& Catalog) const {
	// V1 の Object::gap.x=8 に相当する、少し内側の4点で判定する。
	const float Left = Body_.Position.X + 8.0f;
	const float Right = Body_.Position.X + Body_.Width - 9.0f;
	const float Top = Body_.Position.Y + 1.0f;
	const float Bottom = Body_.Position.Y + Body_.Height - 2.0f;
	return MovementRegionAt(Map, Catalog, Left, Top) == MovementRegion::Ladder &&
		MovementRegionAt(Map, Catalog, Right, Top) == MovementRegion::Ladder &&
		MovementRegionAt(Map, Catalog, Left, Bottom) == MovementRegion::Ladder &&
		MovementRegionAt(Map, Catalog, Right, Bottom) == MovementRegion::Ladder;
}

bool CharacterController::IsCenterInWater(
	const TileMap& Map, const TileCatalog& Catalog) const {
	// V1 の MapHitCC(M) == 5 と同じく、中心点だけで水中判定する。
	// 横衝突で使う中心軸 x+15 と揃える。
	// x+16 を使うと、右壁へ接した x=48 で 64px 境界の右タイルを
	// 誤って参照し、水中なのに Water=false になる。
	const float CenterWorldX = Body_.Position.X + CenterX;
	const float CenterWorldY = Body_.Position.Y + Body_.Height * 0.5f;
	return MovementRegionAt(
		Map, Catalog, CenterWorldX, CenterWorldY) == MovementRegion::Water;
}

void CharacterController::ApplyWaterBoundaryTransition(
	bool WasInWater, bool IsInWater) {
	if (WasInWater == IsInWater) return;

	// V1: 水面を上向きに跨いだ時だけ speed.y *= 2.5。
	if (VelocityY10_ < 0) {
		VelocityY10_ = static_cast<int>(std::round(
			static_cast<float>(VelocityY10_) *
			Motion_.WaterBoundaryVelocityScale));
		Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
	}
}

float CharacterController::SlopeCharacterY(
	CollisionShape Shape, int Column, int Row, float WorldX,
	const TileMap& Map, const TileCatalog& Catalog) const {
	// CanvasMasao#getSakamichiY をそのまま座標式へ移したもの。
	if (IsMasaoSlope(Shape)) return static_cast<float>(CanvasMasaoTerrain::GetSakamichiY(
		Map, Catalog, static_cast<int>(WorldX), Row * Map.TileHeight() + 31));

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
	if (Is2x1Slope(Shape)) {
		if (!ExtendedSlopeTerrain::TryCharacterY(
			Map, Catalog, WorldX, ProbeY, CharacterY)) return false;
		if (FoundShape != nullptr) *FoundShape = Shape;
		return true;
	}
	if (Is1x2Slope(Shape)) {
		if (!ExtendedSlopeTerrain::TryCharacterY1x2(
			Map, Catalog, WorldX, ProbeY, CharacterY)) return false;
		if (FoundShape != nullptr) *FoundShape = Shape;
		return true;
	}
	if (!IsMasaoSlope(Shape)) {
		float SurfaceY = 0.0f;
		if (!TerrainCollision::TryGetSurfaceY(
			Shape, {Column, Row}, WorldX,
			Map.TileWidth(), Map.TileHeight(), SurfaceY)) return false;
		CharacterY = SurfaceY - BelowY;
		if (FoundShape != nullptr) *FoundShape = Shape;
		return true;
	}
	CharacterY = SlopeCharacterY(Shape, Column, Row, WorldX, Map, Catalog);
	if (FoundShape != nullptr) *FoundShape = Shape;
	return true;
}

void CharacterController::RefreshGround(
	const TileMap& Map, const TileCatalog& Catalog) {
	const float X = Body_.Position.X + CenterX;
	const float FootY = Body_.Position.Y + BottomY;
	Body_.Grounded = IsSolidAt(Map, Catalog, X, Body_.Position.Y + BelowY);

	float SlopeY = 0.0f;
	CollisionShape GroundShape = CollisionShape::None;
	if (TrySlopeCharacterY(Map, Catalog, X, FootY, SlopeY, &GroundShape)) {
		const float Tolerance = Is2x1Slope(GroundShape) ? 1.0f : 0.0f;
		if (SlopeY <= Body_.Position.Y + Tolerance) {
			Body_.Position.Y = SlopeY;
			if (Body_.Velocity.Y >= 0.0f) Body_.Grounded = true;
		}
	}
	if (Body_.Grounded && Body_.Velocity.Y > 0.0f) Body_.Velocity.Y = 0.0f;
	if (Body_.Grounded) VelocityY10_ = 0;
}

void CharacterController::ResolveHorizontalWall(
	float OldCenterX, bool MovingRight,
	const TileMap& Map, const TileCatalog& Catalog) {
	int X = static_cast<int>(Body_.Position.X);
	float ExtendedX = Body_.Position.X;
	if (ExtendedSlopeTerrain::ResolveHighSide(
			Map, Catalog, OldCenterX - CenterX, ExtendedX, Body_.Position.Y,
			MovingRight, Body_.Grounded) ||
		ExtendedSlopeTerrain::ResolveHighSide1x2(
			Map, Catalog, OldCenterX - CenterX, ExtendedX, Body_.Position.Y,
			MovingRight, Body_.Grounded)) {
		Body_.Position.X = ExtendedX;
		Body_.Velocity.X = 0.0f;
		VelocityX10_ = 0;
		return;
	}
	if (CanvasMasaoTerrain::ResolveHorizontalSolid(
		Map, Catalog, X, static_cast<int>(Body_.Position.Y), MovingRight) ||
		CanvasMasaoTerrain::ResolveHorizontalSlopeSide(
			Map, Catalog, static_cast<int>(OldCenterX - CenterX), X,
			static_cast<int>(Body_.Position.Y), MovingRight, Body_.Grounded)) {
		Body_.Position.X = static_cast<float>(X);
		Body_.Velocity.X = 0.0f;
		VelocityX10_ = 0;
		return;
	}
	const float NewCenterX = Body_.Position.X + CenterX;
	const int NewColumn = TileAt(NewCenterX, Map.TileWidth());
	const float ProbeYs[] = {Body_.Position.Y, Body_.Position.Y + BottomY};
	bool Blocked = false;

	for (float ProbeY : ProbeYs) {
		const CollisionShape Shape = ShapeAt(Map, Catalog, NewCenterX, ProbeY);
		if (Shape == CollisionShape::Solid || IsMasaoSlope(Shape)) continue;
		if (!IsSlope(Shape) || IsMasaoSlope(Shape)) continue;
		ExtendedSlopeTerrain::Slope2x1 LogicalSlope2x1;
		if (ExtendedSlopeTerrain::TryFind2x1(
			Map, Catalog, static_cast<int>(NewCenterX), static_cast<int>(ProbeY),
			LogicalSlope2x1)) continue;
		ExtendedSlopeTerrain::Slope1x2 LogicalSlope1x2;
		if (ExtendedSlopeTerrain::TryFind1x2(
			Map, Catalog, static_cast<int>(NewCenterX), static_cast<int>(ProbeY),
			LogicalSlope1x2)) continue;
		float SurfaceY = 0.0f;
		const bool HasSurface = TerrainCollision::TryGetSurfaceY(
			Shape, {NewColumn, TileAt(ProbeY, Map.TileHeight())}, NewCenterX,
			Map.TileWidth(), Map.TileHeight(), SurfaceY);
		if (HasSurface && Body_.Grounded && Body_.Position.Y <= SurfaceY - BelowY) continue;
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
	VelocityX10_ = 0;
}

void CharacterController::FollowMasaoSlopeAfterHorizontal(
	float OldX, float OldY, bool WasGrounded,
	const TileMap& Map, const TileCatalog& Catalog) {
	float ExtendedY = Body_.Position.Y;
	bool ExtendedGrounded = WasGrounded;
	if (ExtendedSlopeTerrain::FollowHorizontal(
			Map, Catalog, OldX, Body_.Position.X, OldY, ExtendedY,
			VelocityX10_, VelocityY10_, WasGrounded, ExtendedGrounded) ||
		ExtendedSlopeTerrain::FollowHorizontal1x2(
			Map, Catalog, OldX, Body_.Position.X, OldY, ExtendedY,
			VelocityX10_, VelocityY10_, WasGrounded, ExtendedGrounded)) {
		Body_.Position.Y = ExtendedY;
		Body_.Grounded = ExtendedGrounded;
		Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
		return;
	}
	int Y = static_cast<int>(Body_.Position.Y);
	bool Grounded = WasGrounded;
	if (CanvasMasaoTerrain::FollowHorizontalSlope(
		Map, Catalog, static_cast<int>(OldX), static_cast<int>(Body_.Position.X),
		Y, VelocityX10_, VelocityY10_, Grounded)) {
		Body_.Position.Y = static_cast<float>(Y);
		Body_.Grounded = Grounded;
		Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
		return;
	}
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
		VelocityY10_ = 0;
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
			IsMasaoSlope(Definition->Collision) || Is2x1Slope(Definition->Collision) ||
			Is1x2Slope(Definition->Collision)) continue;
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
		VelocityY10_ = 0;
		return;
	}
	const int NewFootRow = TileAt(OldY + BottomY, Map.TileHeight());
	if (ShapeAt(Map, Catalog, NewCenterX, OldY + BottomY) == CollisionShape::Solid &&
		IsSlope(OldFootShape)) {
		// 坂の高い端から同じ行の平地へ移る分岐。
		Body_.Position.Y = static_cast<float>(NewFootRow * Map.TileHeight()) - BelowY;
		Body_.Grounded = true;
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
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
			VelocityY10_ = 0;
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
	const int OldY = static_cast<int>(Body_.Position.Y);
	Body_.Position.Y += Amount;
	int X = static_cast<int>(Body_.Position.X);
	int NewY = static_cast<int>(Body_.Position.Y);
	const int Direction = HorizontalInput > 0.0f ? 1 : HorizontalInput < 0.0f ? -1 : 0;

	// V1 の透明？ブロック(num=13)相当。
	// 横・上からは存在しないが、下から頭が境界を跨いだ時だけ天井として扱う。
	const int OldTopRow = TileAt(static_cast<float>(OldY), Map.TileHeight());
	const int NewTopRow = TileAt(Body_.Position.Y, Map.TileHeight());
	for (int Row = OldTopRow - 1; Row >= NewTopRow; --Row) {
		const float Bottom = static_cast<float>((Row + 1) * Map.TileHeight());
		if (static_cast<float>(OldY) < Bottom || Body_.Position.Y >= Bottom) continue;
		const float ProbeY = static_cast<float>(Row * Map.TileHeight()) + 0.5f;
		const float LeftHeadX = Body_.Position.X + 1.0f;
		const float RightHeadX = Body_.Position.X + Body_.Width - 2.0f;
		if (ShapeAt(Map, Catalog, LeftHeadX, ProbeY) == CollisionShape::HitFromBelowOnly ||
			ShapeAt(Map, Catalog, RightHeadX, ProbeY) == CollisionShape::HitFromBelowOnly) {
			Body_.Position.Y = Bottom;
			Body_.Velocity.Y = 0.0f;
			VelocityY10_ = 0;
			Body_.Grounded = false;
			return;
		}
	}

	if (CanvasMasaoTerrain::ResolveVerticalSolid(Map, Catalog, X, NewY, false) ||
		CanvasMasaoTerrain::ResolveRisingSlope(Map, Catalog, X, OldY, NewY) ||
		CanvasMasaoTerrain::ResolveDirectionalVerticalSolid(
			Map, Catalog, X, OldY, NewY, Direction, false)) {
		Body_.Position.X = static_cast<float>(X);
		Body_.Position.Y = static_cast<float>(NewY);
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = false;
		return;
	}

	float ExtendedY = Body_.Position.Y;
	if (ExtendedSlopeTerrain::ResolveRising(
			Map, Catalog, Body_.Position.X, static_cast<float>(OldY), ExtendedY) ||
		ExtendedSlopeTerrain::ResolveRising1x2(
			Map, Catalog, Body_.Position.X, static_cast<float>(OldY), ExtendedY)) {
		Body_.Position.Y = ExtendedY;
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = false;
		return;
	}

	// CanvasMasao互換・2x1・1x2の論理坂に含まれない独自形状だけ、
	// 既存の下面判定を残す。
	const int OldRow = TileAt(static_cast<float>(OldY), Map.TileHeight());
	const int NewRow = TileAt(Body_.Position.Y, Map.TileHeight());
	const float CenterProbeX = Body_.Position.X + CenterX;
	for (int Row = OldRow - 1; Row >= NewRow; --Row) {
		const float ProbeY = static_cast<float>(Row * Map.TileHeight());
		const CollisionShape Shape = ShapeAt(Map, Catalog, CenterProbeX, ProbeY);
		if (!IsSlope(Shape) || IsMasaoSlope(Shape) ||
			Is2x1Slope(Shape) || Is1x2Slope(Shape)) continue;
		Body_.Position.Y = static_cast<float>((Row + 1) * Map.TileHeight());
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = false;
		return;
	}
	Body_.Grounded = false;
}

void CharacterController::MoveDown(
	float Amount, float HorizontalInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	const int OldY = static_cast<int>(Body_.Position.Y);
	Body_.Position.Y += Amount;
	int X = static_cast<int>(Body_.Position.X);
	int NewY = static_cast<int>(Body_.Position.Y);
	const int Direction = HorizontalInput > 0.0f ? 1 : HorizontalInput < 0.0f ? -1 : 0;
	float ExtendedY = Body_.Position.Y;
	if (ExtendedSlopeTerrain::ResolveFalling(
			Map, Catalog, Body_.Position.X, static_cast<float>(OldY), ExtendedY) ||
		ExtendedSlopeTerrain::ResolveFalling1x2(
			Map, Catalog, Body_.Position.X, static_cast<float>(OldY), ExtendedY)) {
		Body_.Position.Y = ExtendedY;
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = true;
		return;
	}
	if (CanvasMasaoTerrain::ResolveVerticalSolid(Map, Catalog, X, NewY, true) ||
		CanvasMasaoTerrain::ResolveFallingSlope(Map, Catalog, X, OldY, NewY) ||
		CanvasMasaoTerrain::ResolveFallingOneWay(Map, Catalog, X, OldY, NewY) ||
		CanvasMasaoTerrain::ResolveDirectionalVerticalSolid(
			Map, Catalog, X, OldY, NewY, Direction, true)) {
		Body_.Position.X = static_cast<float>(X);
		Body_.Position.Y = static_cast<float>(NewY);
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = true;
		return;
	}

	const float NewFootY = Body_.Position.Y + BottomY;
	const float CenterProbeX = Body_.Position.X + CenterX;
	float SlopeY = 0.0f;
	CollisionShape FoundShape = CollisionShape::None;
	if (TrySlopeCharacterY(Map, Catalog, CenterProbeX, NewFootY, SlopeY, &FoundShape) &&
		!IsMasaoSlope(FoundShape) && !Is2x1Slope(FoundShape) &&
		!Is1x2Slope(FoundShape) &&
		SlopeY < Body_.Position.Y && SlopeY >= OldY) {
		Body_.Position.Y = SlopeY;
		Body_.Velocity.Y = 0.0f;
		VelocityY10_ = 0;
		Body_.Grounded = true;
		return;
	}
	Body_.Grounded = false;
}

void CharacterController::MoveVertical(
	float Amount, float HorizontalInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount < 0.0f) MoveUp(Amount, HorizontalInput, Map, Catalog);
	else if (Amount > 0.0f) MoveDown(Amount, HorizontalInput, Map, Catalog);
}

void CharacterController::EmitInteractionAtWorld(
	TileTrigger Trigger, const TileMap& Map, float X, float Y) {
	TilePosition Position;
	if (!Map.TryWorldToTile({X, Y}, Position)) return;
	const int* Id = Map.TryGet(Position);
	if (Id == nullptr) return;

	for (std::size_t Index = 0; Index < Interactions_.size(); ++Index) {
		const TileInteraction& Existing = Interactions_[Index];
		if (Existing.Trigger == Trigger &&
			Existing.Position.Column == Position.Column &&
			Existing.Position.Row == Position.Row) return;
	}

	TileInteraction Interaction;
	Interaction.Trigger = Trigger;
	Interaction.Position = Position;
	Interaction.TileId = *Id;
	Interactions_.push_back(Interaction);
}

void CharacterController::EmitTouchInteractions(const TileMap& Map) {
	const float Left = Body_.Position.X + 1.0f;
	const float Right = Body_.Position.X + Body_.Width - 2.0f;
	const float Top = Body_.Position.Y + 1.0f;
	const float Bottom = Body_.Position.Y + Body_.Height - 2.0f;
	const float Center = Body_.Position.X + CenterX;

	EmitInteractionAtWorld(TileTrigger::Touch, Map, Left, Top);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Right, Top);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Left, Bottom);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Right, Bottom);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Center,
		Body_.Position.Y + Body_.Height * 0.5f);
}

void CharacterController::EmitStandInteractions(const TileMap& Map) {
	if (!Body_.Grounded) return;
	const float ProbeY = Body_.Position.Y + Body_.Height + 0.01f;
	const float Left = Body_.Position.X + 1.0f;
	const float Right = Body_.Position.X + Body_.Width - 2.0f;
	EmitInteractionAtWorld(TileTrigger::StandOn, Map, Left, ProbeY);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Left, ProbeY);
	EmitInteractionAtWorld(TileTrigger::StandOn, Map, Right, ProbeY);
	EmitInteractionAtWorld(TileTrigger::Touch, Map, Right, ProbeY);
}

void CharacterController::StepClimbing(
	const CharacterInput& Input,
	const TileMap& Map, const TileCatalog& Catalog) {
	float Horizontal = std::max(-1.0f, std::min(1.0f, Input.Horizontal));
	float Vertical = std::max(-1.0f, std::min(1.0f, Input.Vertical));

	VelocityX10_ = static_cast<int>(
		std::round(Horizontal * Motion_.ClimbHorizontalSpeed * 10.0f));
	VelocityY10_ = static_cast<int>(
		std::round(Vertical * Motion_.ClimbVerticalSpeed * 10.0f));
	Body_.Velocity.X = static_cast<float>(VelocityX10_) / 10.0f;
	Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
	Body_.Grounded = false;

	const float HorizontalAmount = static_cast<float>(
		CanvasMasaoTerrain::RoundDown(static_cast<double>(VelocityX10_) / 10.0));
	const float OldX = Body_.Position.X;
	MoveHorizontal(HorizontalAmount, Map, Catalog);

	const float ActualHorizontal = Body_.Position.X - OldX;
	if (HorizontalAmount > 0.0f && ActualHorizontal < HorizontalAmount - 0.01f) {
		const float ProbeX = Body_.Position.X + Body_.Width + 0.01f;
		EmitInteractionAtWorld(
			TileTrigger::PushFromLeft, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::PushFromLeft, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
	} else if (HorizontalAmount < 0.0f &&
		ActualHorizontal > HorizontalAmount + 0.01f) {
		const float ProbeX = Body_.Position.X - 0.01f;
		EmitInteractionAtWorld(
			TileTrigger::PushFromRight, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::PushFromRight, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
	}

	// V1 は横移動後に Lad() を再判定する。はしごから外れたらそのフレームで通常へ戻す。
	if (!IsInsideLadder(Map, Catalog)) {
		Mode_ = MovementMode::Normal;
		VelocityY10_ = 0;
		Body_.Velocity.Y = 0.0f;
		return;
	}

	const float VerticalAmount = static_cast<float>(
		CanvasMasaoTerrain::RoundDown(static_cast<double>(VelocityY10_) / 10.0));
	MoveVertical(VerticalAmount, Horizontal, Map, Catalog);

	if (VerticalAmount < 0.0f && VelocityY10_ == 0) {
		const float ProbeY = Body_.Position.Y - 0.01f;
		EmitInteractionAtWorld(
			TileTrigger::HitFromBelow, Map, Body_.Position.X + 1.0f, ProbeY);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, Body_.Position.X + 1.0f, ProbeY);
		EmitInteractionAtWorld(
			TileTrigger::HitFromBelow, Map,
			Body_.Position.X + Body_.Width - 2.0f, ProbeY);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map,
			Body_.Position.X + Body_.Width - 2.0f, ProbeY);
	}

	if (!IsInsideLadder(Map, Catalog)) {
		Mode_ = MovementMode::Normal;
	}
}

void CharacterController::Step(
	float HorizontalInput, bool JumpPressed,
	const TileMap& Map, const TileCatalog& Catalog) {
	CharacterInput Input;
	Input.Horizontal = HorizontalInput;
	Input.JumpPressed = JumpPressed;
	Step(Input, Map, Catalog);
}

void CharacterController::Step(
	const CharacterInput& RawInput,
	const TileMap& Map, const TileCatalog& Catalog) {
	Interactions_.clear();

	CharacterInput Input = RawInput;
	Input.Horizontal = std::max(-1.0f, std::min(1.0f, Input.Horizontal));
	Input.Vertical = std::max(-1.0f, std::min(1.0f, Input.Vertical));

	// jM100 と同じく、入力処理より前に現在座標から接地を再判定する。
	RefreshGround(Map, Catalog);
	const bool OnLadder = IsInsideLadder(Map, Catalog);
	InWater_ = IsCenterInWater(Map, Catalog);

	if (Mode_ == MovementMode::Climbing) {
		if (!OnLadder || (Body_.Grounded && Input.Vertical > 0.0f)) {
			Mode_ = MovementMode::Normal;
		}
	} else {
		// V1: 上入力で開始。空中なら下入力でもはしごへ移れる。
		if (OnLadder &&
			(Input.Vertical < 0.0f ||
			 (!Body_.Grounded && Input.Vertical > 0.0f))) {
			Mode_ = MovementMode::Climbing;
			VelocityX10_ = 0;
			VelocityY10_ = 0;
			Body_.Velocity = {0.0f, 0.0f};
			Body_.Grounded = false;
		}
	}

	if (Mode_ == MovementMode::Climbing) {
		StepClimbing(Input, Map, Catalog);
		EmitTouchInteractions(Map);
		EmitStandInteractions(Map);
		return;
	}

	const float HorizontalSpeed =
		InWater_ ? Motion_.WaterMoveSpeed : Motion_.MoveSpeed;
	VelocityX10_ = static_cast<int>(std::round(
		Input.Horizontal * HorizontalSpeed * 10.0f));
	Body_.Velocity.X = static_cast<float>(VelocityX10_) / 10.0f;
	const float OldX = Body_.Position.X;
	const float HorizontalAmount = static_cast<float>(CanvasMasaoTerrain::RoundDown(
		static_cast<double>(VelocityX10_) / 10.0));
	MoveHorizontal(HorizontalAmount, Map, Catalog);

	// 壁へ押し付けた事実だけをイベント化する。ギミックの意味はここでは判断しない。
	const float ActualHorizontal = Body_.Position.X - OldX;
	if (HorizontalAmount > 0.0f && ActualHorizontal < HorizontalAmount - 0.01f) {
		const float ProbeX = Body_.Position.X + Body_.Width + 0.01f;
		EmitInteractionAtWorld(
			TileTrigger::PushFromLeft, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::PushFromLeft, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
	} else if (HorizontalAmount < 0.0f && ActualHorizontal > HorizontalAmount + 0.01f) {
		const float ProbeX = Body_.Position.X - 0.01f;
		EmitInteractionAtWorld(
			TileTrigger::PushFromRight, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX, Body_.Position.Y + 1.0f);
		EmitInteractionAtWorld(
			TileTrigger::PushFromRight, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
		EmitInteractionAtWorld(
			TileTrigger::Touch, Map, ProbeX,
			Body_.Position.Y + Body_.Height - 2.0f);
	}

	// 横方向の移動でもWater状態自体は即時更新する。
	// ただしV1の speed.y *= 2.5 は MoveY 内だけなので、
	// 横移動によるWater境界通過では縦速度を増幅しない。
	InWater_ = IsCenterInWater(Map, Catalog);

	bool WaterJumped = false;
	if (Input.JumpPressed && InWater_) {
		float JumpSpeed = Motion_.WaterJumpSpeed;
		if (Input.Vertical < 0.0f) JumpSpeed = Motion_.WaterJumpUpSpeed;
		else if (Input.Vertical > 0.0f) JumpSpeed = Motion_.WaterJumpDownSpeed;
		VelocityY10_ = -static_cast<int>(std::round(JumpSpeed * 10.0f));
		Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
		Body_.Grounded = false;
		WaterJumped = true;
	} else if (Input.JumpPressed && Body_.Grounded) {
		VelocityY10_ = -static_cast<int>(std::round(Motion_.JumpSpeed * 10.0f));
		Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
		Body_.Grounded = false;
	}
	if (!Body_.Grounded) {
		if (!WaterJumped) {
			const float GravityScale =
				InWater_ ? Motion_.WaterGravityScale : 1.0f;
			const float MaxFallScale =
				InWater_ ? Motion_.WaterMaxFallSpeedScale : 1.0f;
			VelocityY10_ += static_cast<int>(std::round(
				Motion_.Gravity * GravityScale * 10.0f));
			VelocityY10_ = std::min(
				static_cast<int>(std::round(
					Motion_.MaxFallSpeed * MaxFallScale * 10.0f)),
				VelocityY10_);
			Body_.Velocity.Y = static_cast<float>(VelocityY10_) / 10.0f;
		}

		const float VerticalAmount = static_cast<float>(CanvasMasaoTerrain::RoundDown(
			static_cast<double>(VelocityY10_) / 10.0));
		const bool WaterBeforeVertical = InWater_;
		MoveVertical(VerticalAmount, Input.Horizontal, Map, Catalog);
		InWater_ = IsCenterInWater(Map, Catalog);
		ApplyWaterBoundaryTransition(WaterBeforeVertical, InWater_);

		// 上昇が地形で止められた場合、V1 の Hited() 相当を左右2点から通知する。
		if (VerticalAmount < 0.0f && VelocityY10_ == 0) {
			const float ProbeY = Body_.Position.Y - 0.01f;
			EmitInteractionAtWorld(
				TileTrigger::HitFromBelow, Map, Body_.Position.X + 1.0f, ProbeY);
			EmitInteractionAtWorld(
				TileTrigger::Touch, Map, Body_.Position.X + 1.0f, ProbeY);
			EmitInteractionAtWorld(
				TileTrigger::HitFromBelow, Map,
				Body_.Position.X + Body_.Width - 2.0f, ProbeY);
			EmitInteractionAtWorld(
				TileTrigger::Touch, Map,
				Body_.Position.X + Body_.Width - 2.0f, ProbeY);
		}
	}

	EmitTouchInteractions(Map);
	EmitStandInteractions(Map);
}

} // namespace uchinoko
