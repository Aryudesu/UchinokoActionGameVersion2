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
	WorldPosition HitboxOffset;
	WorldPosition HitboxSize;
	int ContactDamage = 0;
	bool Active = true;

	ObjectHitBounds HitBounds() const;
};

struct NativeObjectContact {
	std::size_t ObjectIndex = 0;
	std::string ObjectId;
	std::string TypeId;
	int ContactDamage = 0;
};

class NativeObjectSystem {
public:
	Result<bool> Reset(const StageArea& Area);

	const std::vector<NativeObjectRuntime>& Objects() const {
		return Objects_;
	}

	const NativeObjectRuntime* Find(const std::string& ObjectId) const;

	std::vector<NativeObjectContact> FindContacts(
		WorldPosition Position,
		WorldPosition Size) const;

private:
	static Result<NativeObjectRuntime> BuildRuntime(
		const ObjectSpawn& Spawn);

	std::vector<NativeObjectRuntime> Objects_;
};

} // namespace uchinoko
