#pragma once

#include "Coordinates.h"
#include "Result.h"
#include "ProjectileSystem.h"
#include "StageData.h"

#include <cstddef>
#include <string>
#include <vector>

namespace uchinoko {

struct ObjectHitBounds {
	WorldPosition Position;
	WorldPosition Size;

	bool IsValid() const;
	bool Intersects(
		WorldPosition OtherPosition,
		WorldPosition OtherSize) const;
};

enum class ObjectLifeState {
	Active,
	Dormant,
	Defeated
};

struct NativeObjectRuntime {
	std::string Id;
	std::string TypeId;
	WorldPosition Position;
	WorldPosition InitialPosition;
	WorldPosition Velocity;
	WorldPosition HitboxOffset;
	WorldPosition HitboxSize;
	int ContactDamage = 0;
	int Direction = -1;
	int InitialDirection = -1;
	int Variant = 0;
	float MoveSpeed = 0.0f;
	float Gravity = 0.5f;
	float MaxFallSpeed = 12.0f;
	bool Grounded = false;
	bool Active = true;
	bool ContactEnabled = true;
	bool Stompable = false;
	int BehaviorState = 0;
	int BehaviorTimer = 0;
	std::string AttackPattern;
	int AttackIntervalFrames = 101;
	ObjectLifeState LifeState = ObjectLifeState::Active;
	bool RespawnArmed = true;

	ObjectHitBounds HitBounds() const;
};

enum class NativeObjectContactKind {
	Touch,
	Stomp
};

struct NativeObjectContact {
	std::size_t ObjectIndex = 0;
	std::string ObjectId;
	std::string TypeId;
	int ContactDamage = 0;
	NativeObjectContactKind Kind = NativeObjectContactKind::Touch;
};

class NativeObjectSystem {
public:
	Result<bool> Reset(const StageArea& Area);

	const std::vector<NativeObjectRuntime>& Objects() const {
		return Objects_;
	}

	const NativeObjectRuntime* Find(const std::string& ObjectId) const;
	NativeObjectRuntime* Find(const std::string& ObjectId);

	void UpdateLifecycle(
		WorldPosition CameraPosition,
		WorldPosition ViewSize,
		float ActivationMargin = 32.0f,
		float DormancyMargin = 96.0f);

	void Update(
		const TileMap& Map,
		const TileCatalog& Catalog,
		WorldPosition PlayerPosition = {0.0f, 0.0f});

	std::vector<NativeObjectContact> FindContacts(
		WorldPosition Position,
		WorldPosition Size,
		float PlayerVerticalVelocity = 0.0f) const;

	std::vector<ProjectileSpawnRequest> TakeProjectileSpawns();

	bool Deactivate(const std::string& ObjectId);
	bool HandleStomp(const std::string& ObjectId);
	bool HandlePlayerTouch(
		const std::string& ObjectId,
		float PlayerCenterX);

private:
	static Result<NativeObjectRuntime> BuildRuntime(
		const ObjectSpawn& Spawn);
	static void ResetToSpawn(NativeObjectRuntime& Object);
	static void UpdateWalkingEnemy(
		NativeObjectRuntime& Object,
		const TileMap& Map,
		const TileCatalog& Catalog);
	static void UpdateCarrotMan(
		NativeObjectRuntime& Object,
		const TileMap& Map,
		const TileCatalog& Catalog,
		WorldPosition PlayerPosition);
	static void UpdateBallSlime(
		NativeObjectRuntime& Object,
		const TileMap& Map,
		const TileCatalog& Catalog);
	void UpdateStationaryShooter(
		NativeObjectRuntime& Object);
	void EmitStationaryShooterPattern(
		const NativeObjectRuntime& Object);

	std::vector<NativeObjectRuntime> Objects_;
	std::vector<ProjectileSpawnRequest> PendingProjectileSpawns_;
};

} // namespace uchinoko
