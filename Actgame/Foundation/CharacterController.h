#pragma once

#include "Coordinates.h"
#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

struct GroundHit;

struct CharacterBody {
	WorldPosition Position;
	WorldPosition Velocity;
	float Width = 24.0f;
	float Height = 30.0f;
	bool Grounded = false;
};

struct CharacterMotion {
	float MoveSpeed = 3.0f;
	float JumpSpeed = 9.0f;
	float Gravity = 0.5f;
	float MaxFallSpeed = 10.0f;
};

class CharacterController {
public:
	CharacterController() = default;
	explicit CharacterController(CharacterBody Body, CharacterMotion Motion = CharacterMotion());
	void Step(float HorizontalInput, bool JumpPressed, const TileMap& Map, const TileCatalog& Catalog);
	const CharacterBody& Body() const { return Body_; }
	CharacterBody& Body() { return Body_; }

private:
	bool IsSolid(const TileMap& Map, const TileCatalog& Catalog, int Column, int Row) const;
	void MoveHorizontal(float Amount, const TileMap& Map, const TileCatalog& Catalog);
	void MoveVertical(float Amount, const TileMap& Map, const TileCatalog& Catalog);
	bool FindGroundAtFeet(float FootY, float MaxRise, float MaxDrop,
		const TileMap& Map, const TileCatalog& Catalog, GroundHit& Hit) const;
	bool SnapToGround(float MaxRise, float MaxDrop, const TileMap& Map, const TileCatalog& Catalog);
	CharacterBody Body_;
	CharacterMotion Motion_;
};

} // namespace uchinoko
