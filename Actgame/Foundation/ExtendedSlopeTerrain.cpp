#include "ExtendedSlopeTerrain.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {
namespace {
CollisionShape ShapeAt(const TileMap& Map, const TileCatalog& Catalog, int Column, int Row) {
	const int* Id = Map.TryGet({Column, Row});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	return Definition == nullptr ? CollisionShape::None : Definition->Collision;
}

int TileAt(float Coordinate) {
	return static_cast<int>(std::floor(Coordinate / 32.0f));
}
}

bool ExtendedSlopeTerrain::TryFind2x1(
	const TileMap& Map, const TileCatalog& Catalog, int WorldX, int WorldY, Slope2x1& Result) {
	const int Column = static_cast<int>(std::floor(WorldX / 32.0));
	const int Row = static_cast<int>(std::floor(WorldY / 32.0));
	const CollisionShape Shape = ShapeAt(Map, Catalog, Column, Row);
	if (Shape == CollisionShape::Stair2x1UpRightLow ||
		Shape == CollisionShape::Stair2x1UpRightHigh) {
		const int Left = Shape == CollisionShape::Stair2x1UpRightLow ? Column : Column - 1;
		if (ShapeAt(Map, Catalog, Left, Row) != CollisionShape::Stair2x1UpRightLow ||
			ShapeAt(Map, Catalog, Left + 1, Row) != CollisionShape::Stair2x1UpRightHigh) return false;
		Result = {Left, Row, true};
		return true;
	}
	if (Shape == CollisionShape::Stair2x1UpLeftHigh ||
		Shape == CollisionShape::Stair2x1UpLeftLow) {
		const int Left = Shape == CollisionShape::Stair2x1UpLeftHigh ? Column : Column - 1;
		if (ShapeAt(Map, Catalog, Left, Row) != CollisionShape::Stair2x1UpLeftHigh ||
			ShapeAt(Map, Catalog, Left + 1, Row) != CollisionShape::Stair2x1UpLeftLow) return false;
		Result = {Left, Row, false};
		return true;
	}
	return false;
}

float ExtendedSlopeTerrain::SurfaceY(const Slope2x1& Slope, float WorldX) {
	const int LocalX = std::max(0, std::min(63,
		static_cast<int>(std::floor(WorldX)) - Slope.LeftColumn * 32));
	// CanvasMasao#getSakamichiY の横座標だけを 1/2 にした 64x32 坂。
	// SurfaceY は従来API互換の「キャラクターY + 32」を返す。
	const int VirtualX = LocalX >> 1;
	const int Top = Slope.Row * 32;
	return static_cast<float>(Slope.UpRight
		? Top + 32 - VirtualX
		: Top + 1 + VirtualX);
}

bool ExtendedSlopeTerrain::TryCharacterY(
	const TileMap& Map, const TileCatalog& Catalog, float WorldX, float ProbeY,
	float& CharacterY, Slope2x1* Found) {
	// 1x1坂と同じく、現在プローブしている座標の坂だけを見る。
	// 隣接行探索は坂端を跨いだ FollowHorizontal 内だけで行う。
	Slope2x1 Slope;
	if (!TryFind2x1(
		Map, Catalog,
		static_cast<int>(std::floor(WorldX)),
		static_cast<int>(std::floor(ProbeY)),
		Slope)) return false;
	CharacterY = SurfaceY(Slope, WorldX) - 32.0f;
	if (Found != nullptr) *Found = Slope;
	return true;
}

bool ExtendedSlopeTerrain::FollowHorizontal(
	const TileMap& Map, const TileCatalog& Catalog, float OldX, float NewX,
	float OldY, float& NewY, int VelocityX10, int& VelocityY10,
	bool WasGrounded, bool& Grounded) {
	if (!WasGrounded) return false;

	const float OldCenter = OldX + 15.0f;
	const float NewCenter = NewX + 15.0f;
	const float Foot = OldY + 31.0f;

	Slope2x1 OldSlope;
	if (!TryFind2x1(
		Map, Catalog,
		static_cast<int>(std::floor(OldCenter)),
		static_cast<int>(std::floor(Foot)),
		OldSlope)) return false;

	const float Left = OldSlope.LeftColumn * 32.0f;
	const float Right = Left + 64.0f;
	if (NewCenter >= Left && NewCenter < Right) {
		NewY = SurfaceY(OldSlope, NewCenter) - 32.0f;
		VelocityY10 = 0;
		Grounded = true;
		return true;
	}

	const bool MovingRight = NewCenter > OldCenter;
	const bool LeavesHigh = (OldSlope.UpRight && MovingRight) ||
		(!OldSlope.UpRight && !MovingRight);
	const float EdgeY = LeavesHigh
		? static_cast<float>((OldSlope.Row - 1) * 32)
		: static_cast<float>(OldSlope.Row * 32);

	// CanvasMasao の 1x1 坂と同じく、坂端を跨いだ瞬間だけ前後1行を見て
	// 高さが連続する次の論理坂へ接続する。
	for (int ProbeRow = OldSlope.Row - 1; ProbeRow <= OldSlope.Row + 1; ++ProbeRow) {
		Slope2x1 CandidateSlope;
		if (!TryFind2x1(
			Map, Catalog,
			static_cast<int>(std::floor(NewCenter)),
			ProbeRow * 32 + 16,
			CandidateSlope)) continue;

		// 移動量が3pxでも接続判定がぶれないよう、移動後位置ではなく
		// 候補坂そのものの接続端の高さを比較する。
		const float CandidateEdgeX = MovingRight
			? static_cast<float>(CandidateSlope.LeftColumn * 32)
			: static_cast<float>((CandidateSlope.LeftColumn + 2) * 32 - 1);
		const float CandidateEdgeY = SurfaceY(CandidateSlope, CandidateEdgeX) - 32.0f;
		if (std::fabs(CandidateEdgeY - EdgeY) > 1.0f) continue;

		NewY = SurfaceY(CandidateSlope, NewCenter) - 32.0f;
		VelocityY10 = 0;
		Grounded = true;
		return true;
	}

	const float FloorProbeY = LeavesHigh
		? static_cast<float>(OldSlope.Row * 32)
		: static_cast<float>((OldSlope.Row + 1) * 32);
	if (ShapeAt(Map, Catalog, TileAt(NewCenter), TileAt(FloorProbeY)) == CollisionShape::Solid) {
		NewY = EdgeY;
		VelocityY10 = 0;
		Grounded = true;
		return true;
	}

	NewY = EdgeY;
	// 64px進んで32px上下するため、坂を離れる縦速度も横速度の1/2。
	VelocityY10 = LeavesHigh
		? -(std::abs(VelocityX10) / 2)
		: std::abs(VelocityX10) / 2;
	Grounded = false;
	return true;
}

bool ExtendedSlopeTerrain::ResolveHighSide(
	const TileMap& Map, const TileCatalog& Catalog, float OldX, float& NewX,
	float Y, bool MovingRight, bool Grounded) {
	const float OldCenter = OldX + 15.0f;
	const float NewCenter = NewX + 15.0f;
	const int ProbeYs[] = {
		static_cast<int>(std::floor(Y)),
		static_cast<int>(std::floor(Y + 31.0f))
	};

	for (int ProbeY : ProbeYs) {
		Slope2x1 Slope;
		if (!TryFind2x1(
			Map, Catalog,
			static_cast<int>(std::floor(NewCenter)),
			ProbeY,
			Slope)) continue;

		const float Boundary = Slope.UpRight
			? static_cast<float>((Slope.LeftColumn + 2) * 32)
			: static_cast<float>(Slope.LeftColumn * 32);
		const bool CrossedHighBoundary = Slope.UpRight
			? (!MovingRight && OldCenter >= Boundary && NewCenter < Boundary)
			: (MovingRight && OldCenter < Boundary && NewCenter >= Boundary);
		if (!CrossedHighBoundary) continue;

		const float SlopeCharacterY = SurfaceY(Slope, NewCenter) - 32.0f;
		if (Grounded && Y <= SlopeCharacterY) continue;

		NewX = MovingRight ? Boundary - 16.0f : Boundary - 15.0f;
		return true;
	}
	return false;
}

bool ExtendedSlopeTerrain::ResolveRising(
	const TileMap& Map, const TileCatalog& Catalog, float X, float OldY, float& NewY) {
	const int OldRow = TileAt(OldY);
	const int NewRow = TileAt(NewY);
	if (NewRow >= OldRow) return false;

	Slope2x1 Slope;
	if (!TryFind2x1(
		Map, Catalog,
		static_cast<int>(std::floor(X + 15.0f)),
		static_cast<int>(std::floor(NewY)),
		Slope)) return false;

	NewY = static_cast<float>((NewRow + 1) * 32);
	return true;
}

bool ExtendedSlopeTerrain::ResolveFalling(
	const TileMap& Map, const TileCatalog& Catalog, float X, float OldY, float& NewY) {
	float CandidateY = 0.0f;
	if (!TryCharacterY(Map, Catalog, X + 15.0f, NewY + 31.0f, CandidateY)) return false;
	if (CandidateY >= NewY || CandidateY < OldY) return false;
	NewY = CandidateY;
	return true;
}

} // namespace uchinoko
