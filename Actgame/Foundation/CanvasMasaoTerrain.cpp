#include "CanvasMasaoTerrain.h"

#include <cstdlib>

namespace uchinoko {
namespace {
int FloorDiv32(int Value) {
	if (Value >= 0) return Value >> 5;
	return -static_cast<int>((static_cast<unsigned int>(-Value) + 31U) >> 5);
}
}

int CanvasMasaoTerrain::CodeFor(CollisionShape Shape) {
	switch (Shape) {
	case CollisionShape::Solid: return SolidCode;
	case CollisionShape::OneWay:
	case CollisionShape::DropThroughOneWay:
		return OneWayCode;
	case CollisionShape::SlopeUpRight: return SlopeUpRightCode;
	case CollisionShape::SlopeUpLeft: return SlopeUpLeftCode;
	default: return EmptyCode;
	}
}

int CanvasMasaoTerrain::CodeAt(
	const TileMap& Map, const TileCatalog& Catalog, int X, int Y) {
	const int* Id = Map.TryGet({FloorDiv32(X), FloorDiv32(Y)});
	const TileDefinition* Definition = Id == nullptr ? nullptr : Catalog.Find(*Id);
	return Definition == nullptr ? EmptyCode : CodeFor(Definition->Collision);
}

bool CanvasMasaoTerrain::IsSlope(int Code) {
	return Code == SlopeUpRightCode || Code == SlopeUpLeftCode;
}

bool CanvasMasaoTerrain::IsSolid(int Code) { return Code >= SolidCode; }

int CanvasMasaoTerrain::RoundDown(double Value) {
	return static_cast<int>(Value);
}

int CanvasMasaoTerrain::GetSakamichiY(
	const TileMap& Map, const TileCatalog& Catalog, int X, int Y) {
	const int Row = FloorDiv32(Y);
	const int LocalX = X - FloorDiv32(X) * 32;
	const int Code = CodeAt(Map, Catalog, X, Y);
	if (Code == SlopeUpRightCode) return Row * 32 + (31 - LocalX) - 31;
	if (Code == SlopeUpLeftCode) return Row * 32 + LocalX - 31;
	return Y - 31;
}

bool CanvasMasaoTerrain::ResolveHorizontalSolid(
	const TileMap& Map, const TileCatalog& Catalog, int& X, int Y, bool MovingRight) {
	const int ProbeX = X + 15;
	if (!IsSolid(CodeAt(Map, Catalog, ProbeX, Y)) &&
		!IsSolid(CodeAt(Map, Catalog, ProbeX, Y + 31))) return false;
	const int Column = FloorDiv32(ProbeX);
	X = MovingRight ? Column * 32 - 16 : Column * 32 + 17;
	return true;
}

bool CanvasMasaoTerrain::ResolveVerticalSolid(
	const TileMap& Map, const TileCatalog& Catalog, int X, int& Y, bool MovingDown) {
	const int ProbeY = MovingDown ? Y + 31 : Y;
	if (!IsSolid(CodeAt(Map, Catalog, X + 15, ProbeY))) return false;
	const int Row = FloorDiv32(ProbeY);
	Y = MovingDown ? Row * 32 - 32 : Row * 32 + 32;
	return true;
}

bool CanvasMasaoTerrain::FollowHorizontalSlope(
	const TileMap& Map, const TileCatalog& Catalog, int OldX, int NewX, int& Y,
	int VelocityX10, int& VelocityY10, bool& Grounded) {
	if (!Grounded) return false;
	const int OldCenter = OldX + 15;
	const int NewCenter = NewX + 15;
	const int Foot = Y + 31;
	const int Code = CodeAt(Map, Catalog, OldCenter, Foot);
	if (!IsSlope(Code)) return false;
	const int OldColumn = FloorDiv32(OldCenter);
	const int NewColumn = FloorDiv32(NewCenter);
	if (OldColumn == NewColumn) {
		Y = GetSakamichiY(Map, Catalog, NewCenter, Foot);
		VelocityY10 = 0;
		return true;
	}
	const bool MovingRight = NewCenter > OldCenter;
	const bool LeavesHigh = (Code == SlopeUpRightCode && MovingRight) ||
		(Code == SlopeUpLeftCode && !MovingRight);
	const int Row = FloorDiv32(Foot);
	const int EdgeY = LeavesHigh ? (Row - 1) * 32 : Row * 32;
	for (int ProbeRow = Row - 1; ProbeRow <= Row + 1; ++ProbeRow) {
		const int ProbeY = ProbeRow * 32 + 31;
		if (!IsSlope(CodeAt(Map, Catalog, NewCenter, ProbeY))) continue;
		const int Candidate = GetSakamichiY(Map, Catalog, NewCenter, ProbeY);
		if (std::abs(Candidate - EdgeY) > 1) continue;
		Y = Candidate;
		VelocityY10 = 0;
		return true;
	}
	const int FloorProbeY = LeavesHigh ? Row * 32 : (Row + 1) * 32;
	if (IsSolid(CodeAt(Map, Catalog, NewCenter, FloorProbeY))) {
		Y = EdgeY;
		VelocityY10 = 0;
		return true;
	}
	Y = EdgeY;
	VelocityY10 = LeavesHigh ? -std::abs(VelocityX10) : std::abs(VelocityX10);
	Grounded = false;
	return true;
}

bool CanvasMasaoTerrain::ResolveHorizontalSlopeSide(
	const TileMap& Map, const TileCatalog& Catalog, int OldX, int& NewX, int Y,
	bool MovingRight, bool Grounded) {
	const int OldColumn = FloorDiv32(OldX + 15);
	const int NewColumn = FloorDiv32(NewX + 15);
	if (OldColumn == NewColumn) return false;
	const int Blocking = MovingRight ? SlopeUpLeftCode : SlopeUpRightCode;
	const int ProbeYs[] = {Y, Y + 31};
	for (int ProbeY : ProbeYs) {
		if (CodeAt(Map, Catalog, NewX + 15, ProbeY) != Blocking) continue;
		if (Grounded && Y <= GetSakamichiY(Map, Catalog, NewX + 15, ProbeY)) continue;
		NewX = MovingRight ? NewColumn * 32 - 16 : NewColumn * 32 + 17;
		return true;
	}
	return false;
}

bool CanvasMasaoTerrain::ResolveRisingSlope(
	const TileMap& Map, const TileCatalog& Catalog, int X, int OldY, int& NewY) {
	if (FloorDiv32(NewY) >= FloorDiv32(OldY) || !IsSlope(CodeAt(Map, Catalog, X + 15, NewY))) return false;
	NewY = FloorDiv32(NewY) * 32 + 32;
	return true;
}

bool CanvasMasaoTerrain::ResolveFallingSlope(
	const TileMap& Map, const TileCatalog& Catalog, int X, int OldY, int& NewY) {
	const int ProbeY = NewY + 31;
	if (!IsSlope(CodeAt(Map, Catalog, X + 15, ProbeY))) return false;
	const int SlopeY = GetSakamichiY(Map, Catalog, X + 15, ProbeY);
	if (SlopeY >= NewY || SlopeY < OldY) return false;
	NewY = SlopeY;
	return true;
}

bool CanvasMasaoTerrain::ResolveFallingOneWay(
	const TileMap& Map, const TileCatalog& Catalog, int X, int OldY, int& NewY) {
	const int OldRow = FloorDiv32(OldY + 31);
	const int NewRow = FloorDiv32(NewY + 31);
	if (NewRow <= OldRow || CodeAt(Map, Catalog, X + 15, NewY + 31) != OneWayCode) return false;
	NewY = NewRow * 32 - 32;
	return true;
}

bool CanvasMasaoTerrain::ResolveRisingOneWay(
	const TileMap& Map, const TileCatalog& Catalog, int X, int OldY, int& NewY) {
	const int OldRow = FloorDiv32(OldY);
	const int NewRow = FloorDiv32(NewY);
	if (NewRow >= OldRow || CodeAt(Map, Catalog, X + 15, NewY) != OneWayCode) return false;
	NewY = (NewRow + 1) * 32;
	return true;
}

bool CanvasMasaoTerrain::ResolveDirectionalVerticalSolid(
	const TileMap& Map, const TileCatalog& Catalog, int& X, int OldY, int& NewY,
	int Direction, bool MovingDown) {
	if (Direction == 0) return false;
	const int ProbeX = X + (Direction > 0 ? 16 : 14);
	const int OldProbeY = MovingDown ? OldY + 31 : OldY;
	const int NewProbeY = MovingDown ? NewY + 31 : NewY;
	if (FloorDiv32(OldProbeY) == FloorDiv32(NewProbeY) ||
		CodeAt(Map, Catalog, ProbeX, OldProbeY) > 17 ||
		!IsSolid(CodeAt(Map, Catalog, ProbeX, NewProbeY))) return false;
	const int Row = FloorDiv32(NewProbeY);
	NewY = MovingDown ? Row * 32 - 32 : Row * 32 + 32;
	if (MovingDown) X += Direction > 0 ? 1 : -1;
	return true;
}

} // namespace uchinoko
