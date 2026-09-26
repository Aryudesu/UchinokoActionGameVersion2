#include "NativeObjectRuntime.h"

#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {

namespace {

constexpr int CarrotHidden = 0;
constexpr int CarrotEmerging = 1;
constexpr int CarrotWalking = 2;
constexpr float CarrotTriggerDistance = 96.0f;
constexpr int CarrotTriggerFrames = 30;
constexpr float CarrotJumpSpeed = 10.0f;

constexpr int BallSlimeWalking = 0;
constexpr int BallSlimeShell = 1;
constexpr int BallSlimeKicked = 2;
constexpr int BallSlimeWakeFrames = 60 * 5;
constexpr int BallSlimeRecoverFrames = 60 * 7;
constexpr int BallSlimeKickGraceFrames = 30;
constexpr float BallSlimeWalkSpeed = 2.0f;
constexpr float BallSlimeKickSpeed = 8.0f;
constexpr float BallSlimeWakeJumpSpeed = 3.0f;

bool IsBallSlime(const NativeObjectRuntime& Object) {
	return Object.TypeId == "BallSlime";
}

bool IsKickedBallSlime(const NativeObjectRuntime& Object) {
	return IsBallSlime(Object) &&
		Object.BehaviorState == BallSlimeKicked;
}

bool UsesEnemyLifecycle(const NativeObjectRuntime& Object) {
	return Object.TypeId == "WalkingEnemy" ||
		Object.TypeId == "CarrotMan" ||
		Object.TypeId == "BallSlime";
}

bool IsWalkingCollisionEnemy(const NativeObjectRuntime& Object) {
	if (Object.TypeId == "WalkingEnemy") return true;
	if (Object.TypeId == "CarrotMan") {
		return Object.BehaviorState != CarrotHidden;
	}
	return IsBallSlime(Object) &&
		Object.BehaviorState != BallSlimeShell;
}

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

bool TryReadFloat(
	const StagePropertyMap& Properties,
	const char* Name,
	float& Value,
	std::string& Error,
	const std::string& ObjectId) {
	const auto Found = Properties.find(Name);
	if (Found == Properties.end()) return true;
	if (!Found->second.TryGetFloat(Value)) {
		Error =
			"Object property must be number: " +
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

bool TryReadString(
	const StagePropertyMap& Properties,
	const char* Name,
	std::string& Value,
	std::string& Error,
	const std::string& ObjectId) {
	const auto Found = Properties.find(Name);
	if (Found == Properties.end()) return true;
	if (!Found->second.TryGetString(Value)) {
		Error =
			"Object property must be string: " +
			ObjectId + "." + Name;
		return false;
	}
	return true;
}

bool TryObjectSurfaceY(
	CollisionShape Shape,
	TilePosition Tile,
	float WorldX,
	int TileWidth,
	int TileHeight,
	float& SurfaceY) {
	if (Shape == CollisionShape::DropThroughOneWay) {
		SurfaceY = static_cast<float>(Tile.Row * TileHeight);
		return true;
	}
	return TerrainCollision::TryGetSurfaceY(
		Shape,
		Tile,
		WorldX,
		TileWidth,
		TileHeight,
		SurfaceY);
}

bool FindObjectGround(
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition Foot,
	float MaxRise,
	float MaxDrop,
	GroundHit& Hit) {
	if (MaxRise < 0.0f ||
		MaxDrop < 0.0f ||
		Foot.X < 0.0f) {
		return false;
	}

	const int Column =
		static_cast<int>(std::floor(Foot.X / Map.TileWidth()));
	if (Column < 0 || Column >= Map.Width()) return false;

	const int FirstRow = (std::max)(
		0,
		static_cast<int>(
			std::floor((Foot.Y - MaxRise) / Map.TileHeight())) - 1);
	const int LastRow = (std::min)(
		Map.Height() - 1,
		static_cast<int>(
			std::floor((Foot.Y + MaxDrop) / Map.TileHeight())) + 1);

	bool Found = false;
	for (int Row = FirstRow; Row <= LastRow; ++Row) {
		const TilePosition Position = {Column, Row};
		const int* Id = Map.TryGet(Position);
		const TileDefinition* Definition =
			Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr) continue;

		float SurfaceY = 0.0f;
		if (!TryObjectSurfaceY(
			Definition->Collision,
			Position,
			Foot.X,
			Map.TileWidth(),
			Map.TileHeight(),
			SurfaceY)) {
			continue;
		}

		const float Distance = SurfaceY - Foot.Y;
		if (Distance < -MaxRise ||
			Distance > MaxDrop) {
			continue;
		}

		if (!Found || SurfaceY < Hit.SurfaceY) {
			Found = true;
			Hit.Tile = Position;
			Hit.SurfaceY = SurfaceY;
			Hit.Shape = Definition->Collision;
		}
	}
	return Found;
}

bool ResolveWalkingEnemySide(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	const ObjectHitBounds Bounds = Object.HitBounds();
	const float WorldWidth =
		static_cast<float>(Map.Width() * Map.TileWidth());

	if (Bounds.Position.X < 0.0f) {
		Object.Position.X -= Bounds.Position.X;
		Object.Direction = 1;
		return true;
	}
	if (Bounds.Position.X + Bounds.Size.X > WorldWidth) {
		Object.Position.X -=
			Bounds.Position.X + Bounds.Size.X - WorldWidth;
		Object.Direction = -1;
		return true;
	}

	const bool MovingRight = Object.Direction > 0;
	const float ProbeX =
		MovingRight
			? Bounds.Position.X + Bounds.Size.X - 0.01f
			: Bounds.Position.X + 0.01f;
	const int Column =
		static_cast<int>(std::floor(ProbeX / Map.TileWidth()));
	if (Column < 0 || Column >= Map.Width()) return false;

	const int FirstRow = (std::max)(
		0,
		static_cast<int>(
			std::floor(Bounds.Position.Y / Map.TileHeight())));
	const int LastRow = (std::min)(
		Map.Height() - 1,
		static_cast<int>(
			std::floor(
				(Bounds.Position.Y + Bounds.Size.Y - 0.01f) /
				Map.TileHeight())));

	for (int Row = FirstRow; Row <= LastRow; ++Row) {
		const TilePosition Position = {Column, Row};
		const int* Id = Map.TryGet(Position);
		const TileDefinition* Definition =
			Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr) continue;

		float BlockTop = 0.0f;
		float BlockBottom = 0.0f;
		if (!TerrainCollision::TryGetSideBlock(
			Definition->Collision,
			Position,
			MovingRight
				? TerrainCollision::TileSide::Left
				: TerrainCollision::TileSide::Right,
			Map.TileWidth(),
			Map.TileHeight(),
			BlockTop,
			BlockBottom)) {
			continue;
		}

		const float BoundsBottom =
			Bounds.Position.Y + Bounds.Size.Y;
		if (Bounds.Position.Y >= BlockBottom ||
			BoundsBottom <= BlockTop) {
			continue;
		}

		if (MovingRight) {
			const float TileLeft =
				static_cast<float>(Column * Map.TileWidth());
			Object.Position.X =
				TileLeft -
				Object.HitboxOffset.X -
				Object.HitboxSize.X;
			Object.Direction = -1;
		} else {
			const float TileRight =
				static_cast<float>((Column + 1) * Map.TileWidth());
			Object.Position.X =
				TileRight - Object.HitboxOffset.X;
			Object.Direction = 1;
		}
		return true;
	}
	return false;
}

bool HasWalkingEnemyGroundAhead(
	const NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	const ObjectHitBounds Bounds = Object.HitBounds();
	const float ProbeX =
		Object.Direction > 0
			? Bounds.Position.X + Bounds.Size.X + 0.5f
			: Bounds.Position.X - 0.5f;
	const float FootY =
		Bounds.Position.Y + Bounds.Size.Y + 0.5f;

	GroundHit Hit;
	return FindObjectGround(
		Map,
		Catalog,
		{ProbeX, FootY},
		2.0f,
		Object.MoveSpeed * 2.0f + 2.0f,
		Hit);
}

void ResolveWalkingEnemyVertical(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	Object.Velocity.Y = (std::min)(
		Object.MaxFallSpeed,
		Object.Velocity.Y + Object.Gravity);
	Object.Position.Y += Object.Velocity.Y;

	const ObjectHitBounds Bounds = Object.HitBounds();
	const float FootX =
		Bounds.Position.X + Bounds.Size.X * 0.5f;
	const float FootY =
		Bounds.Position.Y + Bounds.Size.Y;

	GroundHit Hit;
	const float SnapDistance =
		Object.MoveSpeed * 2.0f + 2.0f;
	const bool Found = FindObjectGround(
		Map,
		Catalog,
		{FootX, FootY},
		std::fabs(Object.Velocity.Y) + SnapDistance,
		Object.Grounded ? SnapDistance : 0.0f,
		Hit);

	if (!Found || Object.Velocity.Y < 0.0f) {
		Object.Grounded = false;
		return;
	}

	Object.Position.Y =
		Hit.SurfaceY -
		Object.HitboxOffset.Y -
		Object.HitboxSize.Y;
	Object.Velocity.Y = 0.0f;
	Object.Grounded = true;
}

bool RuleTargetsEnemy(const TileRule& Rule) {
	return Rule.Target == TileTarget::Enemy ||
		Rule.Target == TileTarget::Both;
}

bool IsEnemyDamageAction(const TileRule& Rule) {
	return Rule.Action == TileAction::Damage ||
		Rule.Action == TileAction::InstantDeath;
}

bool IntersectsExpandedView(
	const ObjectHitBounds& Bounds,
	WorldPosition CameraPosition,
	WorldPosition ViewSize,
	float Margin) {
	const float SafeMargin = (std::max)(0.0f, Margin);
	const WorldPosition ViewPosition = {
		CameraPosition.X - SafeMargin,
		CameraPosition.Y - SafeMargin
	};
	const WorldPosition ExpandedSize = {
		ViewSize.X + SafeMargin * 2.0f,
		ViewSize.Y + SafeMargin * 2.0f
	};
	return Bounds.Intersects(ViewPosition, ExpandedSize);
}

bool TouchesEnemyDamageTerrain(
	const NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	const ObjectHitBounds Bounds = Object.HitBounds();
	if (!Bounds.IsValid()) return false;

	// 接地したSolid等もTouchとして拾えるよう、判定矩形をわずかに広げる。
	constexpr float TouchEpsilon = 0.01f;
	const float Left = Bounds.Position.X - TouchEpsilon;
	const float Top = Bounds.Position.Y - TouchEpsilon;
	const float Right = Bounds.Position.X + Bounds.Size.X + TouchEpsilon;
	const float Bottom = Bounds.Position.Y + Bounds.Size.Y + TouchEpsilon;

	const int FirstColumn = (std::max)(
		0,
		static_cast<int>(std::floor(Left / Map.TileWidth())));
	const int LastColumn = (std::min)(
		Map.Width() - 1,
		static_cast<int>(std::floor(Right / Map.TileWidth())));
	const int FirstRow = (std::max)(
		0,
		static_cast<int>(std::floor(Top / Map.TileHeight())));
	const int LastRow = (std::min)(
		Map.Height() - 1,
		static_cast<int>(std::floor(Bottom / Map.TileHeight())));

	for (int Row = FirstRow; Row <= LastRow; ++Row) {
		for (int Column = FirstColumn; Column <= LastColumn; ++Column) {
			const int* Id = Map.TryGet({Column, Row});
			const TileDefinition* Definition =
				Id == nullptr ? nullptr : Catalog.Find(*Id);
			if (Definition == nullptr) continue;

			for (const TileRule& Rule : Definition->Rules) {
				if (Rule.Trigger != TileTrigger::Touch ||
					!RuleTargetsEnemy(Rule) ||
					!IsEnemyDamageAction(Rule)) {
					continue;
				}
				return true;
			}
		}
	}
	return false;
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
	Runtime.InitialPosition = Spawn.Position;

	if (Spawn.TypeId == "WalkingEnemy") {
		// V1 WalkingEnemy1:
		// 32x32 sprite, gap.x=8 / gap.y=1
		// => contact rectangle is approximately 16x31.
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.Variant = 1;
		Runtime.MoveSpeed = 2.0f;
		Runtime.Gravity = 0.5f;
		Runtime.MaxFallSpeed = 12.0f;
	} else if (Spawn.TypeId == "CarrotMan") {
		// V1 CarrotMan: 近づくまでは地中待機し、
		// 31frame目に上へ飛び出してから通常歩行へ移る。
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = false;
		Runtime.Stompable = false;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.MoveSpeed = 2.0f;
		Runtime.Gravity = 0.5f;
		Runtime.MaxFallSpeed = 12.0f;
		Runtime.BehaviorState = CarrotHidden;
		Runtime.BehaviorTimer = 0;
	} else if (Spawn.TypeId == "BallSlime") {
		// V1 BallSlime / BallSlime2:
		// Walking -> Shell -> Kicked の3状態を1 TypeIdで扱う。
		Runtime.HitboxOffset = {2.0f, 1.0f};
		Runtime.HitboxSize = {28.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = 1;
		Runtime.MoveSpeed = BallSlimeWalkSpeed;
		Runtime.Gravity = 0.5f;
		Runtime.MaxFallSpeed = 12.0f;
		Runtime.BehaviorState = BallSlimeWalking;
		Runtime.BehaviorTimer = 0;
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

	if (Spawn.TypeId == "WalkingEnemy" ||
		Spawn.TypeId == "BallSlime") {
		std::string Direction =
			Runtime.Direction < 0 ? "left" : "right";
		if (!TryReadString(
			Spawn.Properties,
			"direction",
			Direction,
			Error,
			Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}
		if (Direction == "left") {
			Runtime.Direction = -1;
		} else if (Direction == "right") {
			Runtime.Direction = 1;
		} else {
			return Result<NativeObjectRuntime>::Failure(
				Spawn.TypeId + " direction must be left or right: " +
				Spawn.Id);
		}
		Runtime.InitialDirection = Runtime.Direction;

		if (!TryReadInteger(
			Spawn.Properties,
			"variant",
			Runtime.Variant,
			Error,
			Spawn.Id) ||
			!TryReadFloat(
				Spawn.Properties,
				"speed",
				Runtime.MoveSpeed,
				Error,
				Spawn.Id) ||
			!TryReadFloat(
				Spawn.Properties,
				"gravity",
				Runtime.Gravity,
				Error,
				Spawn.Id) ||
			!TryReadFloat(
				Spawn.Properties,
				"maxFallSpeed",
				Runtime.MaxFallSpeed,
				Error,
				Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant != 1 &&
			Runtime.Variant != 2) {
			return Result<NativeObjectRuntime>::Failure(
				Spawn.TypeId + " variant must be 1 or 2: " +
				Spawn.Id);
		}
		if (Runtime.MoveSpeed < 0.0f ||
			Runtime.Gravity < 0.0f ||
			Runtime.MaxFallSpeed <= 0.0f) {
			return Result<NativeObjectRuntime>::Failure(
				Spawn.TypeId + " motion values are invalid: " +
				Spawn.Id);
		}
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

NativeObjectRuntime* NativeObjectSystem::Find(
	const std::string& ObjectId) {
	for (NativeObjectRuntime& Object : Objects_) {
		if (Object.Id == ObjectId) return &Object;
	}
	return nullptr;
}

void NativeObjectSystem::ResetToSpawn(
	NativeObjectRuntime& Object) {
	Object.Position = Object.InitialPosition;
	Object.Direction = Object.InitialDirection;
	Object.Velocity = {0.0f, 0.0f};
	Object.Grounded = false;

	if (Object.TypeId == "CarrotMan") {
		Object.BehaviorState = CarrotHidden;
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = false;
		Object.Stompable = false;
	} else if (Object.TypeId == "BallSlime") {
		Object.BehaviorState = BallSlimeWalking;
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
		Object.MoveSpeed = BallSlimeWalkSpeed;
	}
}

void NativeObjectSystem::UpdateLifecycle(
	WorldPosition CameraPosition,
	WorldPosition ViewSize,
	float ActivationMargin,
	float DormancyMargin) {
	for (NativeObjectRuntime& Object : Objects_) {
		// EnemyだけをCamera lifecycleの対象にする。
		// Lift等はCamera外でも状態を保持して動かし続ける。
		if (!UsesEnemyLifecycle(Object)) continue;
		if (Object.LifeState == ObjectLifeState::Defeated) continue;

		if (Object.LifeState == ObjectLifeState::Active) {
			if (IntersectsExpandedView(
				Object.HitBounds(),
				CameraPosition,
				ViewSize,
				DormancyMargin)) {
				continue;
			}

			Object.LifeState = ObjectLifeState::Dormant;
			Object.Active = false;
			ResetToSpawn(Object);

			const ObjectHitBounds SpawnBounds = Object.HitBounds();
			Object.RespawnArmed = !IntersectsExpandedView(
				SpawnBounds,
				CameraPosition,
				ViewSize,
				ActivationMargin);
			continue;
		}

		const ObjectHitBounds SpawnBounds = Object.HitBounds();
		const bool SpawnInActivationView = IntersectsExpandedView(
			SpawnBounds,
			CameraPosition,
			ViewSize,
			ActivationMargin);

		if (!SpawnInActivationView) {
			// 一度spawn地点を十分画面外へ出してからでないと再出現させない。
			Object.RespawnArmed = true;
			continue;
		}
		if (!Object.RespawnArmed) continue;

		Object.LifeState = ObjectLifeState::Active;
		Object.Active = true;
		Object.RespawnArmed = false;
		ResetToSpawn(Object);
	}
}

void NativeObjectSystem::UpdateWalkingEnemy(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	if (!Object.Active || Object.MoveSpeed <= 0.0f) return;

	Object.Velocity.X =
		static_cast<float>(Object.Direction) * Object.MoveSpeed;
	Object.Position.X += Object.Velocity.X;

	const bool HitWall =
		ResolveWalkingEnemySide(Object, Map, Catalog);
	if (HitWall) {
		Object.Velocity.X =
			static_cast<float>(Object.Direction) * Object.MoveSpeed;
	}

	// V1 WalkingEnemy2だけが崖手前で反転する。
	// WalkingEnemy1は崖からそのまま落下する。
	if (!HitWall &&
		Object.Variant == 2 &&
		Object.Grounded &&
		!HasWalkingEnemyGroundAhead(Object, Map, Catalog)) {
		Object.Direction *= -1;
		Object.Velocity.X =
			static_cast<float>(Object.Direction) * Object.MoveSpeed;
	}

	ResolveWalkingEnemyVertical(Object, Map, Catalog);

	// Damage / InstantDeath はデータ上は区別したまま保持する。
	// 現在のWalkingEnemyはHPを持たないため、どちらも接触時に非Active化する。
	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateCarrotMan(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	if (Object.BehaviorState == CarrotHidden) {
		Object.Velocity = {0.0f, 0.0f};
		Object.Grounded = false;
		if (std::fabs(PlayerPosition.X - Object.Position.X) <=
			CarrotTriggerDistance) {
			++Object.BehaviorTimer;
			if (Object.BehaviorTimer > CarrotTriggerFrames) {
				Object.BehaviorState = CarrotEmerging;
				Object.BehaviorTimer = 0;
				Object.ContactEnabled = true;
				Object.Stompable = true;
				Object.Velocity.Y = -CarrotJumpSpeed;
			}
		} else {
			Object.BehaviorTimer = 0;
		}
		return;
	}

	if (Object.BehaviorState == CarrotEmerging) {
		ResolveWalkingEnemyVertical(Object, Map, Catalog);
		if (Object.Grounded) {
			Object.BehaviorState = CarrotWalking;
			Object.Direction =
				PlayerPosition.X > Object.Position.X ? 1 : -1;
		}
	} else {
		Object.Velocity.X =
			static_cast<float>(Object.Direction) * Object.MoveSpeed;
		Object.Position.X += Object.Velocity.X;

		const bool HitWall =
			ResolveWalkingEnemySide(Object, Map, Catalog);
		if (HitWall) {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
		}

		ResolveWalkingEnemyVertical(Object, Map, Catalog);
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateBallSlime(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	if (!Object.Active) return;

	if (Object.BehaviorState == BallSlimeShell) {
		++Object.BehaviorTimer;
		Object.Velocity.X = 0.0f;
		Object.ContactDamage = 0;

		if (Object.BehaviorTimer >= BallSlimeWakeFrames) {
			Object.Stompable = true;
			if (Object.Grounded &&
				Object.BehaviorTimer == BallSlimeWakeFrames) {
				Object.Velocity.Y = -BallSlimeWakeJumpSpeed;
				Object.Grounded = false;
			}
		} else {
			Object.Stompable = false;
		}

		ResolveWalkingEnemyVertical(Object, Map, Catalog);

		if (Object.BehaviorTimer >= BallSlimeRecoverFrames) {
			Object.BehaviorState = BallSlimeWalking;
			Object.BehaviorTimer = 0;
			Object.MoveSpeed = BallSlimeWalkSpeed;
			Object.ContactDamage = 1;
			Object.Stompable = true;
		}
	} else {
		const bool Kicked =
			Object.BehaviorState == BallSlimeKicked;
		if (Kicked) {
			++Object.BehaviorTimer;
			Object.MoveSpeed = BallSlimeKickSpeed;
			Object.ContactDamage =
				Object.BehaviorTimer >= BallSlimeKickGraceFrames
					? 1
					: 0;
			Object.Stompable =
				Object.BehaviorTimer >= BallSlimeKickGraceFrames;
		} else {
			Object.MoveSpeed = BallSlimeWalkSpeed;
			Object.ContactDamage = 1;
			Object.Stompable = true;
		}

		Object.Velocity.X =
			static_cast<float>(Object.Direction) * Object.MoveSpeed;
		Object.Position.X += Object.Velocity.X;

		const bool HitWall =
			ResolveWalkingEnemySide(Object, Map, Catalog);
		if (HitWall) {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
		}

		// BallSlime2相当(variant=2)は通常歩行時だけ崖で反転。
		// 蹴られた甲羅はvariantに関係なく崖から落ちる。
		if (!HitWall &&
			!Kicked &&
			Object.Variant == 2 &&
			Object.Grounded &&
			!HasWalkingEnemyGroundAhead(Object, Map, Catalog)) {
			Object.Direction *= -1;
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
		}

		ResolveWalkingEnemyVertical(Object, Map, Catalog);
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		// V1のBallSlimeは通常/高速状態でdamage床に触れると
		// 即消滅ではなく甲羅状態へ移り、軽く跳ねる。
		if (Object.BehaviorState != BallSlimeShell) {
			Object.BehaviorState = BallSlimeShell;
			Object.BehaviorTimer = 0;
			Object.MoveSpeed = BallSlimeWalkSpeed;
			Object.ContactDamage = 0;
			Object.Stompable = false;
			Object.Velocity.X = 0.0f;
			Object.Velocity.Y = -6.0f;
			Object.Grounded = false;
		}
	}
}

void NativeObjectSystem::Update(
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	for (NativeObjectRuntime& Object : Objects_) {
		if (!Object.Active) continue;
		if (Object.TypeId == "WalkingEnemy") {
			UpdateWalkingEnemy(Object, Map, Catalog);
		} else if (Object.TypeId == "CarrotMan") {
			UpdateCarrotMan(Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "BallSlime") {
			UpdateBallSlime(Object, Map, Catalog);
		}
	}

	// 歩行中Enemy同士が横からぶつかった場合は、互いに反転させる。
	// 地形解決後のHitBoundsで判定し、縦方向の重なりがある組だけを対象にする。
	for (std::size_t LeftIndex = 0; LeftIndex < Objects_.size(); ++LeftIndex) {
		NativeObjectRuntime& Left = Objects_[LeftIndex];
		if (!Left.Active || !IsWalkingCollisionEnemy(Left)) continue;

		for (std::size_t RightIndex = LeftIndex + 1;
			RightIndex < Objects_.size();
			++RightIndex) {
			NativeObjectRuntime& Right = Objects_[RightIndex];
			if (!Right.Active || !IsWalkingCollisionEnemy(Right)) continue;

			const ObjectHitBounds LeftBounds = Left.HitBounds();
			const ObjectHitBounds RightBounds = Right.HitBounds();
			if (!LeftBounds.Intersects(
				RightBounds.Position,
				RightBounds.Size)) {
				continue;
			}

			const float LeftCenterY =
				LeftBounds.Position.Y + LeftBounds.Size.Y * 0.5f;
			const float RightCenterY =
				RightBounds.Position.Y + RightBounds.Size.Y * 0.5f;
			const float VerticalCenterDistance =
				std::fabs(LeftCenterY - RightCenterY);
			const float MaxSideContactDistance =
				(LeftBounds.Size.Y + RightBounds.Size.Y) * 0.25f;
			if (VerticalCenterDistance > MaxSideContactDistance) {
				continue;
			}

			const float LeftCenterX =
				LeftBounds.Position.X + LeftBounds.Size.X * 0.5f;
			const float RightCenterX =
				RightBounds.Position.X + RightBounds.Size.X * 0.5f;
			const bool LeftIsActuallyLeft = LeftCenterX <= RightCenterX;

			const bool LeftKicked = IsKickedBallSlime(Left);
			const bool RightKicked = IsKickedBallSlime(Right);
			if (LeftKicked != RightKicked) {
				NativeObjectRuntime& Victim =
					LeftKicked ? Right : Left;
				Victim.LifeState = ObjectLifeState::Defeated;
				Victim.Active = false;
				Victim.Velocity = {0.0f, 0.0f};
				continue;
			}

			Left.Direction = LeftIsActuallyLeft ? -1 : 1;
			Right.Direction = LeftIsActuallyLeft ? 1 : -1;
			Left.Velocity.X =
				static_cast<float>(Left.Direction) * Left.MoveSpeed;
			Right.Velocity.X =
				static_cast<float>(Right.Direction) * Right.MoveSpeed;

			// 同一frameで重なった分を半分ずつ戻して、翌frameの再反転を防ぐ。
			const float OverlapX = LeftIsActuallyLeft
				? LeftBounds.Position.X + LeftBounds.Size.X -
					RightBounds.Position.X
				: RightBounds.Position.X + RightBounds.Size.X -
					LeftBounds.Position.X;
			if (OverlapX > 0.0f) {
				const float Correction = OverlapX * 0.5f + 0.01f;
				Left.Position.X += LeftIsActuallyLeft
					? -Correction
					: Correction;
				Right.Position.X += LeftIsActuallyLeft
					? Correction
					: -Correction;
			}
		}
	}
}

std::vector<NativeObjectContact> NativeObjectSystem::FindContacts(
	WorldPosition Position,
	WorldPosition Size,
	float PlayerVerticalVelocity) const {
	std::vector<NativeObjectContact> Contacts;

	for (std::size_t Index = 0; Index < Objects_.size(); ++Index) {
		const NativeObjectRuntime& Object = Objects_[Index];
		if (!Object.Active ||
			!Object.ContactEnabled ||
			!Object.HitBounds().Intersects(Position, Size)) {
			continue;
		}

		NativeObjectContact Contact;
		Contact.ObjectIndex = Index;
		Contact.ObjectId = Object.Id;
		Contact.TypeId = Object.TypeId;
		Contact.ContactDamage = Object.ContactDamage;

		// WalkingEnemyは上から下降して浅く重なった接触だけを踏みつけとして扱う。
		// 横/下からの接触は従来どおりTouch（damage候補）のまま。
		if (Object.Stompable && PlayerVerticalVelocity > 0.0f) {
			const ObjectHitBounds Bounds = Object.HitBounds();
			const float PlayerBottom = Position.Y + Size.Y;
			const float OverlapFromTop = PlayerBottom - Bounds.Position.Y;
			constexpr float StompTolerance = 10.0f;
			if (OverlapFromTop > 0.0f && OverlapFromTop <= StompTolerance) {
				Contact.Kind = NativeObjectContactKind::Stomp;
				Contact.ContactDamage = 0;
			}
		}
		Contacts.push_back(Contact);
	}

	return Contacts;
}

bool NativeObjectSystem::HandleStomp(
	const std::string& ObjectId) {
	NativeObjectRuntime* Object = Find(ObjectId);
	if (Object == nullptr || !Object->Active) return false;

	if (Object->TypeId != "BallSlime") {
		return Deactivate(ObjectId);
	}

	if (Object->BehaviorState == BallSlimeShell) {
		// V1では復活直前(5秒以降)なら再度踏んでtimerを戻せる。
		if (Object->BehaviorTimer >= BallSlimeWakeFrames) {
			Object->BehaviorTimer = 0;
			Object->Stompable = false;
		}
		return true;
	}

	Object->BehaviorState = BallSlimeShell;
	Object->BehaviorTimer = 0;
	Object->MoveSpeed = BallSlimeWalkSpeed;
	Object->Velocity.X = 0.0f;
	Object->ContactDamage = 0;
	Object->Stompable = false;
	return true;
}

bool NativeObjectSystem::HandlePlayerTouch(
	const std::string& ObjectId,
	float PlayerCenterX) {
	NativeObjectRuntime* Object = Find(ObjectId);
	if (Object == nullptr ||
		!Object->Active ||
		Object->TypeId != "BallSlime" ||
		Object->BehaviorState != BallSlimeShell) {
		return false;
	}

	const ObjectHitBounds Bounds = Object->HitBounds();
	const float ObjectCenterX =
		Bounds.Position.X + Bounds.Size.X * 0.5f;
	Object->Direction =
		PlayerCenterX > ObjectCenterX ? -1 : 1;
	Object->BehaviorState = BallSlimeKicked;
	Object->BehaviorTimer = 0;
	Object->MoveSpeed = BallSlimeKickSpeed;
	Object->Velocity.X =
		static_cast<float>(Object->Direction) * Object->MoveSpeed;
	Object->ContactDamage = 0;
	Object->Stompable = false;
	return true;
}

bool NativeObjectSystem::Deactivate(const std::string& ObjectId) {
	NativeObjectRuntime* Object = Find(ObjectId);
	if (Object == nullptr || !Object->Active) return false;
	Object->LifeState = ObjectLifeState::Defeated;
	Object->Active = false;
	Object->Velocity = {0.0f, 0.0f};
	return true;
}

} // namespace uchinoko
