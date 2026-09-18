#pragma once

#include "Coordinates.h"
#include "TileDefinition.h"
#include "TileMap.h"

namespace uchinoko {

struct GroundHit;

struct CharacterBody {
	WorldPosition Position;
	WorldPosition Velocity;
	float Width = 32.0f;
	float Height = 32.0f;
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
	CollisionShape ShapeAt(const TileMap& Map, const TileCatalog& Catalog,
		float X, float Y) const;
	bool IsSolidAt(const TileMap& Map, const TileCatalog& Catalog,
		float X, float Y) const;
	float SlopeCharacterY(CollisionShape Shape, int Column, int Row,
		float WorldX, const TileMap& Map, const TileCatalog& Catalog) const;
	bool TrySlopeCharacterY(const TileMap& Map, const TileCatalog& Catalog,
		float WorldX, float ProbeY, float& CharacterY,
		CollisionShape* FoundShape = nullptr) const;
	void RefreshGround(const TileMap& Map, const TileCatalog& Catalog);
	void ResolveHorizontalWall(float OldCenterX, bool MovingRight,
		const TileMap& Map, const TileCatalog& Catalog);
	void FollowMasaoSlopeAfterHorizontal(float OldX, float OldY, bool WasGrounded,
		const TileMap& Map, const TileCatalog& Catalog);
	void MoveHorizontal(float Amount, const TileMap& Map, const TileCatalog& Catalog);
	void MoveUp(float Amount, float HorizontalInput,
		const TileMap& Map, const TileCatalog& Catalog);
	void MoveDown(float Amount, float HorizontalInput,
		const TileMap& Map, const TileCatalog& Catalog);
	void MoveVertical(float Amount, float HorizontalInput,
		const TileMap& Map, const TileCatalog& Catalog);
	CharacterBody Body_;
	CharacterMotion Motion_;
	int VelocityX10_ = 0;
	int VelocityY10_ = 0;
};

} // namespace uchinoko
