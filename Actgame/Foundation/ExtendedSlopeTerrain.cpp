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
		static_cast<int>(WorldX) - Slope.LeftColumn * 32));
	// 64px幅を仮想32px幅へ正規化し、CanvasMasaoの整数坂座標へ渡す。
	const int VirtualX = LocalX >> 1;
	const int Top = Slope.Row * 32;
	return static_cast<float>(Slope.UpRight ? Top + 32 - VirtualX : Top + VirtualX);
}

bool ExtendedSlopeTerrain::TryCharacterY(
	const TileMap& Map, const TileCatalog& Catalog, float WorldX, float ProbeY,
	float& CharacterY, Slope2x1* Found) {
	for (int Offset = -1; Offset <= 1; ++Offset) {
		Slope2x1 Slope;
		if (!TryFind2x1(Map, Catalog, static_cast<int>(WorldX),
			static_cast<int>(ProbeY) + Offset * 32, Slope)) continue;
		CharacterY = SurfaceY(Slope, WorldX) - 32.0f;
		if (Found != nullptr) *Found = Slope;
		return true;
	}
	return false;
}

bool ExtendedSlopeTerrain::FollowHorizontal(
	const TileMap& Map, const TileCatalog& Catalog, float OldX, float NewX,
	float OldY, float& NewY, bool WasGrounded, bool& Grounded) {
	if (!WasGrounded) return false;
	Slope2x1 OldSlope;
	float IgnoredY = 0.0f;
	if (!TryCharacterY(Map, Catalog, OldX + 15.0f, OldY + 31.0f, IgnoredY, &OldSlope)) return false;
	Slope2x1 NewSlope;
	float CandidateY = 0.0f;
	if (TryCharacterY(Map, Catalog, NewX + 15.0f, OldY + 31.0f, CandidateY, &NewSlope)) {
		const float MaxRiseOrDrop = std::fabs(NewX - OldX) * 0.5f + 0.51f;
		if (std::fabs(CandidateY - OldY) > MaxRiseOrDrop) return false;
		NewY = CandidateY;
		Grounded = true;
		return true;
	}

	const float NewCenter = NewX + 15.0f;
	const float Left = OldSlope.LeftColumn * 32.0f;
	const float Right = Left + 64.0f;
	if (NewCenter >= Left && NewCenter < Right) return false;
	const bool MovingRight = NewX > OldX;
	const bool LeavesHigh = (OldSlope.UpRight && MovingRight) ||
		(!OldSlope.UpRight && !MovingRight);
	NewY = LeavesHigh ? OldSlope.Row * 32.0f - 32.0f : OldSlope.Row * 32.0f;
	const int Column = static_cast<int>(std::floor(NewCenter / 32.0f));
	const int FloorRow = static_cast<int>(std::floor((NewY + 32.0f) / 32.0f));
	Grounded = ShapeAt(Map, Catalog, Column, FloorRow) == CollisionShape::Solid;
	return true;
}

bool ExtendedSlopeTerrain::ResolveHighSide(
	const TileMap& Map, const TileCatalog& Catalog, float OldX, float& NewX,
	float Y, bool MovingRight, bool Grounded) {
	const int OldCenter = static_cast<int>(OldX + 15.0f);
	const int NewCenter = static_cast<int>(NewX + 15.0f);
	const int ProbeYs[] = {static_cast<int>(Y), static_cast<int>(Y + 31.0f)};
	for (int ProbeY : ProbeYs) {
		Slope2x1 Slope;
		if (!TryFind2x1(Map, Catalog, NewCenter, ProbeY, Slope)) continue;
		const int Boundary = Slope.UpRight ? (Slope.LeftColumn + 2) * 32 : Slope.LeftColumn * 32;
		// CanvasMasaoと同様、外側から高い端の境界を越えた瞬間だけ側壁とする。
		// 坂の内部にいるキャラクターのジャンプや下り移動には適用しない。
		const bool CrossedHighBoundary = Slope.UpRight
			? (!MovingRight && OldCenter >= Boundary && NewCenter < Boundary)
			: (MovingRight && OldCenter < Boundary && NewCenter >= Boundary);
		if (!CrossedHighBoundary) continue;
		// 高い端と同じ行の通常ブロックは、上面に接地しているときだけ連続床。
		// 下からジャンプ上昇中まで無条件に通すと、ブロックと坂の側面を貫通する。
		const int OutsideColumn = Slope.UpRight ? Boundary / 32 : Boundary / 32 - 1;
		if (ShapeAt(Map, Catalog, OutsideColumn, Slope.Row) == CollisionShape::Solid &&
			Grounded && Y <= static_cast<float>(Slope.Row * 32)) continue;
		// 境界で坂面の高さがつながる隣接2x1坂は、外壁ではなく連続面。
		// 反対向きの山頂だけでなく、1行ずらした同方向坂の連結も含む。
		Slope2x1 Neighbor;
		const int NeighborX = Slope.UpRight ? Boundary : Boundary - 1;
		for (int RowOffset = -1; RowOffset <= 1; ++RowOffset) {
			if (!TryFind2x1(Map, Catalog, NeighborX,
				(Slope.Row + RowOffset) * 32 + 16, Neighbor)) continue;
			const float CurrentHighY = static_cast<float>(Slope.Row * 32);
			const float NeighborY = SurfaceY(Neighbor, static_cast<float>(NeighborX));
			// 連続する坂面を開けるのは、キャラクターがその面の上側にいる場合だけ。
			// 下面側から高い端をこすって下降している最中まで開けると、段違い境界から
			// 坂の実体へ入り込んでしまう。
			if (std::fabs(NeighborY - CurrentHighY) <= 1.0f &&
				Y + 31.0f <= CurrentHighY + 1.0f) {
				return false;
			}
		}
		NewX = MovingRight ? static_cast<float>(Boundary - 16) : static_cast<float>(Boundary - 15);
		return true;
	}
	return false;
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
