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
	return Shape != CollisionShape::None &&
		Shape != CollisionShape::Solid &&
		Shape != CollisionShape::OneWay;
}

} // namespace

CharacterController::CharacterController(CharacterBody Body, CharacterMotion Motion)
	: Body_(Body), Motion_(Motion) {}

bool CharacterController::IsCeilingBlocked(
	const TileMap& Map, const TileCatalog& Catalog,
	WorldPosition Head, bool BlockSlopes) const {
	// スーパー正男と同様に中央を主判定とし、タイルの継ぎ目だけ左右1pxで補う。
	const float ProbeOffsets[] = {0.0f, -1.0f, 1.0f};
	for (float Offset : ProbeOffsets) {
		const WorldPosition Probe = {Head.X + Offset, Head.Y};
		const int Column = TileAt(Probe.X, Map.TileWidth());
		const int Row = TileAt(Probe.Y, Map.TileHeight());
		const int* Id = Map.TryGet({Column, Row});
		const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr) continue;
		if (Definition->Collision == CollisionShape::Solid) return true;
		// CanvasMasao は、上昇して別のタイル行へ入った瞬間だけ坂タイル全体を天井扱いする。
		// 頭が既に坂タイル内にある場合は、坂へ張り付いた状態でもジャンプできる。
		if (BlockSlopes && Definition->Collision != CollisionShape::None &&
			Definition->Collision != CollisionShape::OneWay) return true;
	}
	return false;
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
	// CanvasMasao の y / y+31 に相当する身体の縦範囲と、坂の側面の
	// 実体範囲が重なる間だけ横壁として扱う。上下2点だけだと、
	// 短い側面が身体の途中に入ったときに見逃してしまう。
	// 接地面そのものを拾わないよう、両端はわずかに身体の内側へ置く。
	const float TopProbeY = Body_.Position.Y + ContactMargin;
	const float BottomProbeY = Body_.Position.Y + Body_.Height - ContactMargin;
	return BottomProbeY > BlockTop + ContactMargin &&
		TopProbeY < BlockBottom - ContactMargin;
}

void CharacterController::MoveHorizontal(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount == 0.0f) return;
	const float OldX = Body_.Position.X;
	Body_.Position.X += Amount;
	const float HalfWidth = Body_.Width * 0.5f;
	const int TopRow = TileAt(Body_.Position.Y + ContactMargin, Map.TileHeight());
	const int BottomRow = TileAt(
		Body_.Position.Y + Body_.Height - ContactMargin, Map.TileHeight());
	const auto IsBlockedAtSide = [&](int Column, bool TargetLeftSide) {
		return IsSideBlocked(Map, Catalog, Column, TopRow, TargetLeftSide) ||
			(BottomRow != TopRow &&
				IsSideBlocked(Map, Catalog, Column, BottomRow, TargetLeftSide));
	};
	if (Amount > 0.0f) {
		const int OldColumn = TileAt(OldX + HalfWidth, Map.TileWidth());
		const int Column = TileAt(Body_.Position.X + HalfWidth, Map.TileWidth());
		// 空中では、列へ入った後に上昇して坂の側壁へ重なる場合があるため毎フレーム確認する。
		if (Column != OldColumn || !Body_.Grounded) {
			if (IsBlockedAtSide(Column, true)) {
				Body_.Position.X = static_cast<float>(Column * Map.TileWidth()) - HalfWidth - ContactMargin;
				Body_.Velocity.X = 0.0f;
			}
		}
	} else {
		const int OldColumn = TileAt(OldX + HalfWidth, Map.TileWidth());
		const int Column = TileAt(Body_.Position.X + HalfWidth, Map.TileWidth());
		if (Column != OldColumn || !Body_.Grounded) {
			if (IsBlockedAtSide(Column, false)) {
				Body_.Position.X = static_cast<float>((Column + 1) * Map.TileWidth()) - HalfWidth + ContactMargin;
				Body_.Velocity.X = 0.0f;
			}
		}
	}
	if (Body_.Grounded) {
		// CanvasMasao と同様に、横移動後の中央足元が坂面へ深く入り込んだ場合は
		// 接地補正で坂上へ持ち上げず、坂タイルの側面で止める。
		// 通常の坂上り（1フレーム分の高低差）はこの判定を通過させる。
		GroundHit Hit;
		const float FootY = Body_.Position.Y + Body_.Height;
		const float MaxClimb = std::fabs(Amount) * 2.0f + 1.0f;
		if (FindGroundAtCenter(FootY, Body_.Height + ContactMargin, 0.0f,
			Body_.Position.Y - ContactMargin, Map, Catalog, Hit) &&
			IsSlopeShape(Hit.Shape) && FootY - Hit.SurfaceY > MaxClimb + ContactMargin) {
			const int Column = TileAt(
				Body_.Position.X + Body_.Width * 0.5f, Map.TileWidth());
			Body_.Position.X = Amount > 0.0f
				? static_cast<float>(Column * Map.TileWidth()) - HalfWidth - ContactMargin
				: static_cast<float>((Column + 1) * Map.TileWidth()) - HalfWidth + ContactMargin;
			Body_.Velocity.X = 0.0f;
		}
	}
	const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - Body_.Width;
	Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
}

bool CharacterController::SnapToGround(
	float MaxRise, float MaxDrop, const TileMap& Map, const TileCatalog& Catalog) {
	GroundHit Hit;
	const float FootY = Body_.Position.Y + Body_.Height;
	// まず、そのフレームで実際に追従できる範囲の地面を優先する。
	// 下の床に立っている時、同じ列の上方に坂が重なっていても、そちらへワープさせない。
	if (!FindGroundAtCenter(FootY, MaxRise, MaxDrop,
		Body_.Position.Y - ContactMargin, Map, Catalog, Hit)) {
		// 有効な足場が近くにない場合だけ、食い込みからの復帰範囲を身体全体へ広げる。
		const float RecoveryRise = std::max(MaxRise, Body_.Height + ContactMargin);
		if (!FindGroundAtCenter(FootY, RecoveryRise, MaxDrop,
			Body_.Position.Y - ContactMargin, Map, Catalog, Hit)) return false;
	}
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
	const float OldTop = Body_.Position.Y;
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
	const WorldPosition Head = {
		Body_.Position.X + Body_.Width * 0.5f,
		Body_.Position.Y
	};
	const int Row = TileAt(Head.Y, Map.TileHeight());
	const bool EnteredUpperRow = Row < TileAt(OldTop, Map.TileHeight());
	if (IsCeilingBlocked(Map, Catalog, Head, EnteredUpperRow)) {
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
