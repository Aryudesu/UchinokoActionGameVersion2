#pragma once

#include "Coordinates.h"
#include "Result.h"
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
	int Variant = 0;
	float MoveSpeed = 0.0f;
	float Gravity = 0.5f;
	float MaxFallSpeed = 12.0f;
	bool Grounded = false;
	bool Active = true;

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

	void Update(
		const TileMap& Map,
		const TileCatalog& Catalog);

	std::vector<NativeObjectContact> FindContacts(
		WorldPosition Position,
		WorldPosition Size,
		float PlayerVerticalVelocity = 0.0f) const;

	bool Deactivate(const std::string& ObjectId);

private:
	static Result<NativeObjectRuntime> BuildRuntime(
		const ObjectSpawn& Spawn);
	static void UpdateWalkingEnemy(
		NativeObjectRuntime& Object,
		const TileMap& Map,
		const TileCatalog& Catalog);

	std::vector<NativeObjectRuntime> Objects_;
};

} // namespace uchinoko
