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

void CharacterController::MoveHorizontal(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	if (Amount == 0.0f) return;
	Body_.Position.X += Amount;
	const int FirstRow = TileAt(Body_.Position.Y + ContactMargin, Map.TileHeight());
	// 足元のタイルは床や坂の土台なので、側面衝突には上半身までを使う。
	const int LastRow = TileAt(Body_.Position.Y + Body_.Height * 0.5f - ContactMargin, Map.TileHeight());
	if (Amount > 0.0f) {
		const int Column = TileAt(Body_.Position.X + Body_.Width, Map.TileWidth());
		for (int Row = FirstRow; Row <= LastRow; ++Row) {
			if (IsSolid(Map, Catalog, Column, Row)) {
				Body_.Position.X = static_cast<float>(Column * Map.TileWidth()) - Body_.Width;
				Body_.Velocity.X = 0.0f;
				break;
			}
		}
	} else {
		const int Column = TileAt(Body_.Position.X, Map.TileWidth());
		for (int Row = FirstRow; Row <= LastRow; ++Row) {
			if (IsSolid(Map, Catalog, Column, Row)) {
				Body_.Position.X = static_cast<float>((Column + 1) * Map.TileWidth());
				Body_.Velocity.X = 0.0f;
				break;
			}
		}
	}
	const float MaxX = static_cast<float>(Map.Width() * Map.TileWidth()) - Body_.Width;
	Body_.Position.X = std::max(0.0f, std::min(MaxX, Body_.Position.X));
}

bool CharacterController::SnapToGround(
	float MaxRise, float MaxDrop, const TileMap& Map, const TileCatalog& Catalog) {
	GroundHit Hit;
	if (!FindGroundAtFeet(Body_.Position.Y + Body_.Height,
		MaxRise, MaxDrop, Body_.Position.Y + Body_.Height - MaxRise - ContactMargin,
		Map, Catalog, Hit)) return false;
	Body_.Position.Y = Hit.SurfaceY - Body_.Height;
	Body_.Velocity.Y = 0.0f;
	Body_.Grounded = true;
	return true;
}

bool CharacterController::FindGroundAtFeet(
	float FootY, float MaxRise, float MaxDrop,
	float MinimumSurfaceY, const TileMap& Map, const TileCatalog& Catalog, GroundHit& Hit) const {
	const float FootXs[] = {
		Body_.Position.X + ContactMargin,
		Body_.Position.X + Body_.Width - ContactMargin
	};
	bool Found = false;
	for (float FootX : FootXs) {
		GroundHit Candidate;
		if (!TerrainCollision::FindGround(
			Map, Catalog, {FootX, FootY}, MaxRise, MaxDrop, Candidate)) continue;
		if (Candidate.SurfaceY < MinimumSurfaceY) continue;
		// 矩形の左右どちらも地形へ入らないよう、最も高い接地面を採用する。
		if (!Found || Candidate.SurfaceY < Hit.SurfaceY) {
			Hit = Candidate;
			Found = true;
		}
	}
	return Found;
}

void CharacterController::MoveVertical(
	float Amount, const TileMap& Map, const TileCatalog& Catalog) {
	const float OldBottom = Body_.Position.Y + Body_.Height;
	Body_.Position.Y += Amount;
	if (Amount >= 0.0f) {
		const float NewBottom = Body_.Position.Y + Body_.Height;
		GroundHit Hit;
		// 空中で横から坂へ入った場合も、身体と重なった最上面まで戻して着地する。
		const float LandingSearch = std::max(
			NewBottom - OldBottom + ContactMargin, Body_.Height + ContactMargin);
		if (FindGroundAtFeet(NewBottom, LandingSearch, 0.0f,
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
	const int FirstColumn = TileAt(Body_.Position.X + ContactMargin, Map.TileWidth());
	const int LastColumn = TileAt(Body_.Position.X + Body_.Width - ContactMargin, Map.TileWidth());
	for (int Column = FirstColumn; Column <= LastColumn; ++Column) {
		if (IsSolid(Map, Catalog, Column, Row)) {
			Body_.Position.Y = static_cast<float>((Row + 1) * Map.TileHeight());
			Body_.Velocity.Y = 0.0f;
			break;
		}
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
