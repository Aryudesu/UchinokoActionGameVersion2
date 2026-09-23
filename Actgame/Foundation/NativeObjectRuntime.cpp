#include "NativeObjectRuntime.h"

namespace uchinoko {

namespace {

bool TryReadVector2(
	const StagePropertyMap& Properties,
	const char* Name,
	WorldPosition& Value,
	std::string& Error,
	const std::string& ObjectId) {
	const auto Found = Properties.find(Name);
	if (Found == Properties.end()) return true;
	if (!Found->second.TryGetVector2(Value)) {
		Error =
			"Object property must be vector2: " +
			ObjectId + "." + Name;
		return false;
	}
	return true;
}

bool TryReadInteger(
	const StagePropertyMap& Properties,
	const char* Name,
	int& Value,
	std::string& Error,
	const std::string& ObjectId) {
	const auto Found = Properties.find(Name);
	if (Found == Properties.end()) return true;
	if (!Found->second.TryGetInteger(Value)) {
		Error =
			"Object property must be integer: " +
			ObjectId + "." + Name;
		return false;
	}
	return true;
}

} // namespace

bool ObjectHitBounds::IsValid() const {
	return Size.X > 0.0f && Size.Y > 0.0f;
}

bool ObjectHitBounds::Intersects(
	WorldPosition OtherPosition,
	WorldPosition OtherSize) const {
	if (!IsValid() ||
		OtherSize.X <= 0.0f ||
		OtherSize.Y <= 0.0f) {
		return false;
	}

	const float Right = Position.X + Size.X;
	const float Bottom = Position.Y + Size.Y;
	const float OtherRight = OtherPosition.X + OtherSize.X;
	const float OtherBottom = OtherPosition.Y + OtherSize.Y;

	return Position.X < OtherRight &&
		Right > OtherPosition.X &&
		Position.Y < OtherBottom &&
		Bottom > OtherPosition.Y;
}

ObjectHitBounds NativeObjectRuntime::HitBounds() const {
	ObjectHitBounds Bounds;
	Bounds.Position = {
		Position.X + HitboxOffset.X,
		Position.Y + HitboxOffset.Y
	};
	Bounds.Size = HitboxSize;
	return Bounds;
}

Result<NativeObjectRuntime> NativeObjectSystem::BuildRuntime(
	const ObjectSpawn& Spawn) {
	NativeObjectRuntime Runtime;
	Runtime.Id = Spawn.Id;
	Runtime.TypeId = Spawn.TypeId;
	Runtime.Position = Spawn.Position;

	if (Spawn.TypeId == "WalkingEnemy") {
		// V1 WalkingEnemy1:
		// 32x32 sprite, gap.x=8 / gap.y=1
		// => contact rectangle is approximately 16x31.
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
	} else if (Spawn.TypeId == "HorizontalLift") {
		// NativeStageSandboxで従来debug描画していた44x10の足場形状。
		Runtime.HitboxOffset = {-6.0f, 11.0f};
		Runtime.HitboxSize = {44.0f, 10.0f};
		Runtime.ContactDamage = 0;
	} else {
		// TypeId schemaが増えるまでの安全なdebug/runtime既定値。
		Runtime.HitboxOffset = {0.0f, 0.0f};
		Runtime.HitboxSize = {32.0f, 32.0f};
		Runtime.ContactDamage = 0;
	}

	std::string Error;
	if (!TryReadVector2(
			Spawn.Properties,
			"hitboxOffset",
			Runtime.HitboxOffset,
			Error,
			Spawn.Id)) {
		return Result<NativeObjectRuntime>::Failure(Error);
	}
	if (!TryReadVector2(
			Spawn.Properties,
			"hitboxSize",
			Runtime.HitboxSize,
			Error,
			Spawn.Id)) {
		return Result<NativeObjectRuntime>::Failure(Error);
	}
	if (!TryReadInteger(
			Spawn.Properties,
			"contactDamage",
			Runtime.ContactDamage,
			Error,
			Spawn.Id)) {
		return Result<NativeObjectRuntime>::Failure(Error);
	}

	if (Runtime.HitboxSize.X <= 0.0f ||
		Runtime.HitboxSize.Y <= 0.0f) {
		return Result<NativeObjectRuntime>::Failure(
			"Object hitboxSize must be positive: " + Spawn.Id);
	}
	if (Runtime.ContactDamage < 0) {
		return Result<NativeObjectRuntime>::Failure(
			"Object contactDamage must not be negative: " + Spawn.Id);
	}

	return Result<NativeObjectRuntime>::Success(Runtime);
}

Result<bool> NativeObjectSystem::Reset(const StageArea& Area) {
	Objects_.clear();

	for (const ObjectLayer& Layer : Area.ObjectLayers) {
		for (const ObjectSpawn& Spawn : Layer.Objects) {
			if (Spawn.TypeId == "PlayerSpawn") continue;

			Result<NativeObjectRuntime> Built =
				BuildRuntime(Spawn);
			if (Built.IsFailure()) {
				Objects_.clear();
				return Result<bool>::Failure(Built.Error());
			}
			Objects_.push_back(Built.Value());
		}
	}
	return Result<bool>::Success(true);
}

const NativeObjectRuntime* NativeObjectSystem::Find(
	const std::string& ObjectId) const {
	for (const NativeObjectRuntime& Object : Objects_) {
		if (Object.Id == ObjectId) return &Object;
	}
	return nullptr;
}

std::vector<NativeObjectContact> NativeObjectSystem::FindContacts(
	WorldPosition Position,
	WorldPosition Size) const {
	std::vector<NativeObjectContact> Contacts;

	for (std::size_t Index = 0; Index < Objects_.size(); ++Index) {
		const NativeObjectRuntime& Object = Objects_[Index];
		if (!Object.Active ||
			!Object.HitBounds().Intersects(Position, Size)) {
			continue;
		}

		NativeObjectContact Contact;
		Contact.ObjectIndex = Index;
		Contact.ObjectId = Object.Id;
		Contact.TypeId = Object.TypeId;
		Contact.ContactDamage = Object.ContactDamage;
		Contacts.push_back(Contact);
	}

	return Contacts;
}

} // namespace uchinoko
