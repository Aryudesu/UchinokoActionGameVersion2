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
	const float LocalX = std::max(0.0f, std::min(64.0f, WorldX - Slope.LeftColumn * 32.0f));
	const float Top = Slope.Row * 32.0f;
	return Slope.UpRight ? Top + 32.0f - LocalX * 0.5f : Top + LocalX * 0.5f;
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
	float OldY, float& NewY, bool WasGrounded) {
	if (!WasGrounded) return false;
	Slope2x1 OldSlope;
	float IgnoredY = 0.0f;
	if (!TryCharacterY(Map, Catalog, OldX + 15.0f, OldY + 31.0f, IgnoredY, &OldSlope)) return false;
	Slope2x1 NewSlope;
	float CandidateY = 0.0f;
	if (!TryCharacterY(Map, Catalog, NewX + 15.0f, OldY + 31.0f, CandidateY, &NewSlope)) return false;
	const float MaxRiseOrDrop = std::fabs(NewX - OldX) * 0.5f + 0.51f;
	if (std::fabs(CandidateY - OldY) > MaxRiseOrDrop) return false;
	NewY = CandidateY;
	return true;
}

bool ExtendedSlopeTerrain::ResolveHighSide(
	const TileMap& Map, const TileCatalog& Catalog, float OldX, float& NewX,
	float Y, bool MovingRight, bool Grounded) {
	const int OldCenter = static_cast<int>(OldX + 15.0f);
	const int NewCenter = static_cast<int>(NewX + 15.0f);
	if ((OldCenter >> 5) == (NewCenter >> 5)) return false;
	Slope2x1 Slope;
	if (!TryFind2x1(Map, Catalog, NewCenter, static_cast<int>(Y + 31.0f), Slope)) return false;
	const bool EntersHighSide = (Slope.UpRight && !MovingRight && NewCenter >= (Slope.LeftColumn + 1) * 32) ||
		(!Slope.UpRight && MovingRight && NewCenter < (Slope.LeftColumn + 1) * 32);
	if (!EntersHighSide) return false;
	const float SurfaceCharacterY = SurfaceY(Slope, static_cast<float>(NewCenter)) - 32.0f;
	if (Grounded && Y <= SurfaceCharacterY) return false;
	const int Boundary = Slope.UpRight ? (Slope.LeftColumn + 2) * 32 : Slope.LeftColumn * 32;
	NewX = MovingRight ? static_cast<float>(Boundary - 16) : static_cast<float>(Boundary - 15);
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
