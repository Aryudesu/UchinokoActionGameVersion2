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
	float ClimbHorizontalSpeed = 2.0f;
	float ClimbVerticalSpeed = 3.0f;
	float WaterHorizontalSpeedScale = 0.3f;
	float WaterGravityScale = 1.0f / 3.0f;
	float WaterMaxFallSpeedScale = 0.5f;
	float WaterJumpSpeed = 4.0f;
	float WaterJumpUpSpeed = 6.0f;
	float WaterJumpDownSpeed = 2.0f;
	float WaterBoundaryVelocityScale = 2.5f;
};

struct CharacterInput {
	float Horizontal = 0.0f;
	float Vertical = 0.0f; // 上=-1、下=+1
	bool JumpPressed = false;
};

enum class MovementMode {
	Normal,
	Climbing
};

class CharacterController {
public:
	CharacterController() = default;
	explicit CharacterController(CharacterBody Body, CharacterMotion Motion = CharacterMotion());
	void Step(float HorizontalInput, bool JumpPressed, const TileMap& Map, const TileCatalog& Catalog);
	void Step(const CharacterInput& Input, const TileMap& Map, const TileCatalog& Catalog);
	const CharacterBody& Body() const { return Body_; }
	CharacterBody& Body() { return Body_; }
	void Reposition(WorldPosition Position, bool ResetVelocity = true);
	const std::vector<TileInteraction>& Interactions() const { return Interactions_; }
	MovementMode Mode() const { return Mode_; }
	bool IsClimbing() const { return Mode_ == MovementMode::Climbing; }
	bool IsInWater() const { return InWater_; }

private:
	CollisionShape ShapeAt(const TileMap& Map, const TileCatalog& Catalog,
		float X, float Y) const;
	bool IsSolidAt(const TileMap& Map, const TileCatalog& Catalog,
		float X, float Y) const;
	MovementRegion MovementRegionAt(
		const TileMap& Map, const TileCatalog& Catalog, float X, float Y) const;
	bool IsInsideLadder(const TileMap& Map, const TileCatalog& Catalog) const;
	bool IsCenterInWater(const TileMap& Map, const TileCatalog& Catalog) const;
	void ApplyWaterBoundaryTransition(bool WasInWater, bool IsInWater);
	void StepClimbing(
		const CharacterInput& Input, const TileMap& Map, const TileCatalog& Catalog);
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
	void EmitInteractionAtWorld(TileTrigger Trigger, const TileMap& Map, float X, float Y);
	void EmitTouchInteractions(const TileMap& Map);
	void EmitStandInteractions(const TileMap& Map);
	CharacterBody Body_;
	CharacterMotion Motion_;
	int VelocityX10_ = 0;
	int VelocityY10_ = 0;
	std::vector<TileInteraction> Interactions_;
	MovementMode Mode_ = MovementMode::Normal;
	bool InWater_ = false;
};

} // namespace uchinoko
