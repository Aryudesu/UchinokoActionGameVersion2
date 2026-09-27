#include "NativeObjectRuntime.h"

#include "TerrainCollision.h"

#include <algorithm>
#include <cmath>
#include <utility>

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

constexpr float ProjectilePi = 3.14159265358979323846f;
constexpr int HspShooterDefaultIntervalFrames = 101;

constexpr int PikachiiWaiting = 0;
constexpr int PikachiiJumping = 1;
constexpr int PikachiiFired = 2;
constexpr float PikachiiTriggerDistance = 32.0f * 8.0f;
constexpr int PikachiiContinueTriggerFrames = 20;
constexpr int PikachiiJumpFrame = 75;
// HSP版は -jump*2 (-18) を設定した直後に gravity(+0.4) を加え、
// abs(vy)>maxVspeed(9) なら -9 へclampする。
// V2共通vertical resolverは上昇側をclampしないため、同じ1frame目の
// 結果(-9.0)になるよう、gravity適用前のseedを -9.4 とする。
constexpr float PikachiiJumpSpeed = 9.4f;
constexpr float PikachiiProjectileSpeed = 5.0f;

constexpr float ChikorarashiTriggerDistance = 32.0f * 10.0f;
constexpr int ChikorarashiCycleFrames = 150;
constexpr int ChikorarashiFirstShotAfter = 50;
constexpr int ChikorarashiShotModulo = 25;
constexpr int ChikorarashiShotRemainder = 24;
constexpr float ChikorarashiProjectileGravity = 0.4f / 3.0f;

constexpr int FlyingHorizontal = 1;
constexpr int FlyingHorizontalOscillation = 2;
constexpr int FlyingVertical = 3;
constexpr int FlyingVerticalOscillation = 4;
constexpr float FlyingOscillationPhaseStep = 0.1f;
constexpr float FlyingOscillationPhaseLimit = 6.28f * 2.0f;
constexpr float FlyingOscillationSpeed = 5.0f;

constexpr int KameenWaiting = 0;
constexpr int KameenChasing = 1;
constexpr float KameenTriggerDistance = 32.0f * 8.0f;
constexpr float KameenAcceleration = 0.2f;
constexpr float KameenMaxSpeed = 8.0f;

constexpr int FishHorizontal = 1;
constexpr int FishHorizontalRange = 2;
constexpr int FishVerticalRange = 3;
constexpr float FishDefaultSpeed = 1.0f;
constexpr float FishRange = 32.0f * 3.0f;

constexpr int WallCrawlerCounterClockwise = 1;
constexpr int WallCrawlerClockwise = 2;
constexpr float WallCrawlerSpeed = 1.0f;

constexpr int TransformingWalkerOriginal = 0;
constexpr int TransformingWalkerChanged = 1;

constexpr int SeaAnemoneRightFan = 1;
constexpr int SeaAnemoneLeftFan = 2;
constexpr float SeaAnemoneTriggerDistance = 32.0f * 6.0f;
constexpr int SeaAnemoneChargeFrames = 160;
constexpr int SeaAnemoneShotIntervalFrames = 10;
constexpr int SeaAnemoneShotCount = 8;
constexpr float SeaAnemoneProjectileSpeed = 6.0f;
constexpr float SeaAnemoneProjectileGravity = 0.4f / 3.0f;
constexpr float SeaAnemoneProjectileMaxFallSpeed = 8.0f / 2.0f;

constexpr int MaririJumpTimer = 100;
constexpr float MaririMoveSpeed = 3.0f;
constexpr float MaririJumpSpeed = 9.0f;

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
		Object.TypeId == "BallSlime" ||
		Object.TypeId == "StationaryShooter" ||
		Object.TypeId == "Pikachii" ||
		Object.TypeId == "Chikorarashi" ||
		Object.TypeId == "FlyingEnemy" ||
		Object.TypeId == "Kameen" ||
		Object.TypeId == "FishEnemy" ||
		Object.TypeId == "WallCrawler" ||
		Object.TypeId == "SeaAnemone" ||
		Object.TypeId == "Mariri";
}

bool IsEnemyCollisionParticipant(const NativeObjectRuntime& Object) {
	if (Object.TypeId == "WalkingEnemy" ||
		Object.TypeId == "StationaryShooter" ||
		Object.TypeId == "Pikachii" ||
		Object.TypeId == "Chikorarashi" ||
		Object.TypeId == "FlyingEnemy" ||
		Object.TypeId == "FishEnemy" ||
		Object.TypeId == "WallCrawler" ||
		Object.TypeId == "SeaAnemone" ||
		Object.TypeId == "Mariri") {
		return true;
	}
	if (Object.TypeId == "Kameen") {
		return Object.BehaviorState == KameenChasing;
	}
	if (Object.TypeId == "CarrotMan") {
		return Object.BehaviorState != CarrotHidden;
	}
	return IsBallSlime(Object);
}

bool IsWalkingCollisionEnemy(const NativeObjectRuntime& Object) {
	if (Object.TypeId == "WalkingEnemy" ||
		Object.TypeId == "TransformingWalker") return true;
	if (Object.TypeId == "CarrotMan") {
		return Object.BehaviorState != CarrotHidden;
	}
	if (IsBallSlime(Object)) {
		return Object.BehaviorState != BallSlimeShell;
	}
	return false;
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

struct CardinalDirection {
	int X = 0;
	int Y = 0;
};

CardinalDirection WallCrawlerDirection(
	const NativeObjectRuntime& Object) {
	if (Object.Velocity.X < 0.0f) return {-1, 0};
	if (Object.Velocity.X > 0.0f) return {1, 0};
	if (Object.Velocity.Y < 0.0f) return {0, -1};
	if (Object.Velocity.Y > 0.0f) return {0, 1};
	return {0, 0};
}

CardinalDirection RotateClockwise(
	CardinalDirection Direction) {
	return {-Direction.Y, Direction.X};
}

CardinalDirection RotateCounterClockwise(
	CardinalDirection Direction) {
	return {Direction.Y, -Direction.X};
}

WorldPosition ToWallCrawlerVelocity(
	CardinalDirection Direction) {
	return {
		static_cast<float>(Direction.X) * WallCrawlerSpeed,
		static_cast<float>(Direction.Y) * WallCrawlerSpeed
	};
}

bool IsWallCrawlerBlocked(
	const TileMap& Map,
	const TileCatalog& Catalog,
	int Column,
	int Row) {
	if (Column < 0 || Column >= Map.Width() ||
		Row < 0 || Row >= Map.Height()) {
		return false;
	}

	const TilePosition Tile = {Column, Row};
	const int* Id = Map.TryGet(Tile);
	const TileDefinition* Definition =
		Id == nullptr ? nullptr : Catalog.Find(*Id);
	if (Definition == nullptr ||
		Definition->Collision == CollisionShape::None) {
		return false;
	}

	const WorldPosition Center = {
		(static_cast<float>(Column) + 0.5f) * Map.TileWidth(),
		(static_cast<float>(Row) + 0.5f) * Map.TileHeight()
	};
	return TerrainCollision::ContainsSolidPoint(
		Definition->Collision,
		Tile,
		Center,
		Map.TileWidth(),
		Map.TileHeight());
}

bool IsWallCrawlerBlocked(
	const TileMap& Map,
	const TileCatalog& Catalog,
	int Column,
	int Row,
	CardinalDirection Direction) {
	return IsWallCrawlerBlocked(
		Map,
		Catalog,
		Column + Direction.X,
		Row + Direction.Y);
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

bool ResolveFlyingEnemyVertical(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	bool MovingDown) {
	const ObjectHitBounds Bounds = Object.HitBounds();
	const float WorldHeight =
		static_cast<float>(Map.Height() * Map.TileHeight());

	if (Bounds.Position.Y < 0.0f) {
		Object.Position.Y -= Bounds.Position.Y;
		return true;
	}
	if (Bounds.Position.Y + Bounds.Size.Y > WorldHeight) {
		Object.Position.Y -=
			Bounds.Position.Y + Bounds.Size.Y - WorldHeight;
		return true;
	}

	const float ProbeY =
		MovingDown
			? Bounds.Position.Y + Bounds.Size.Y - 0.01f
			: Bounds.Position.Y + 0.01f;
	const int Row =
		static_cast<int>(std::floor(ProbeY / Map.TileHeight()));
	if (Row < 0 || Row >= Map.Height()) return false;

	const int FirstColumn = (std::max)(
		0,
		static_cast<int>(
			std::floor(Bounds.Position.X / Map.TileWidth())));
	const int LastColumn = (std::min)(
		Map.Width() - 1,
		static_cast<int>(
			std::floor(
				(Bounds.Position.X + Bounds.Size.X - 0.01f) /
				Map.TileWidth())));

	for (int Column = FirstColumn; Column <= LastColumn; ++Column) {
		const TilePosition Tile = {Column, Row};
		const int* Id = Map.TryGet(Tile);
		const TileDefinition* Definition =
			Id == nullptr ? nullptr : Catalog.Find(*Id);
		if (Definition == nullptr) continue;

		const float TileLeft =
			static_cast<float>(Column * Map.TileWidth());
		const float TileRight =
			static_cast<float>((Column + 1) * Map.TileWidth());
		const float ProbeX = (std::clamp)(
			Bounds.Position.X + Bounds.Size.X * 0.5f,
			TileLeft + 0.01f,
			TileRight - 0.01f);

		if (!TerrainCollision::ContainsSolidPoint(
			Definition->Collision,
			Tile,
			{ProbeX, ProbeY},
			Map.TileWidth(),
			Map.TileHeight())) {
			continue;
		}

		if (MovingDown) {
			const float TileTop =
				static_cast<float>(Row * Map.TileHeight());
			Object.Position.Y =
				TileTop -
				Object.HitboxOffset.Y -
				Object.HitboxSize.Y;
		} else {
			const float TileBottom =
				static_cast<float>((Row + 1) * Map.TileHeight());
			Object.Position.Y =
				TileBottom - Object.HitboxOffset.Y;
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
	} else if (Spawn.TypeId == "TransformingWalker") {
		// HSP enemyf=26/27:
		// movement before stomp is identical to enemyf=1/2 respectively.
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = true;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = 1;
		Runtime.MoveSpeed = 2.0f;
		Runtime.Gravity = 0.5f;
		Runtime.MaxFallSpeed = 12.0f;
		Runtime.BehaviorState = TransformingWalkerOriginal;
	} else if (Spawn.TypeId == "FlyingEnemy") {
		// HSP enemyf=4: gravityを使わず一定高度を横移動する飛行Enemy。
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = FlyingHorizontal;
		Runtime.MoveSpeed = 2.0f;
		Runtime.Gravity = 0.0f;
		Runtime.MaxFallSpeed = 0.0f;
	} else if (Spawn.TypeId == "Mariri") {
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = true;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.MoveSpeed = MaririMoveSpeed;
		Runtime.Gravity = 0.4f;
		Runtime.MaxFallSpeed = 9.0f;
		Runtime.BehaviorTimer = 0;
	} else if (Spawn.TypeId == "SeaAnemone") {
		Runtime.HitboxOffset = {0.0f, 0.0f};
		Runtime.HitboxSize = {32.0f, 32.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = true;
		Runtime.Stompable = false;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = SeaAnemoneRightFan;
		Runtime.MoveSpeed = 0.0f;
		Runtime.Gravity = 0.0f;
		Runtime.MaxFallSpeed = 0.0f;
		Runtime.BehaviorTimer = 0;
		Runtime.BehaviorPhase = 0.0f;
	} else if (Spawn.TypeId == "WallCrawler") {
		Runtime.HitboxOffset = {0.0f, 0.0f};
		Runtime.HitboxSize = {32.0f, 32.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = true;
		Runtime.Stompable = false;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = WallCrawlerCounterClockwise;
		Runtime.MoveSpeed = WallCrawlerSpeed;
		Runtime.Gravity = 0.0f;
		Runtime.MaxFallSpeed = 0.0f;
		Runtime.Velocity = {-WallCrawlerSpeed, 0.0f};
	} else if (Spawn.TypeId == "FishEnemy") {
		Runtime.HitboxOffset = {0.0f, 0.0f};
		Runtime.HitboxSize = {32.0f, 32.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = true;
		Runtime.Stompable = false;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Variant = FishHorizontal;
		Runtime.MoveSpeed = FishDefaultSpeed;
		Runtime.Gravity = 0.0f;
		Runtime.MaxFallSpeed = 0.0f;
		Runtime.BehaviorState = -1;
	} else if (Spawn.TypeId == "Kameen") {
		Runtime.HitboxOffset = {0.0f, 0.0f};
		Runtime.HitboxSize = {32.0f, 32.0f};
		Runtime.ContactDamage = 1;
		Runtime.ContactEnabled = false;
		Runtime.Stompable = false;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Gravity = 0.0f;
		Runtime.MaxFallSpeed = 0.0f;
		Runtime.BehaviorState = KameenWaiting;
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
	} else if (Spawn.TypeId == "Pikachii") {
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.Gravity = 0.4f;
		Runtime.MaxFallSpeed = 9.0f;
		Runtime.BehaviorState = PikachiiWaiting;
		Runtime.BehaviorTimer = 0;
	} else if (Spawn.TypeId == "Chikorarashi") {
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.BehaviorTimer = 0;
		unsigned int Seed = 2166136261u;
		for (const char Ch : Spawn.Id) {
			Seed ^= static_cast<unsigned char>(Ch);
			Seed *= 16777619u;
		}
		Runtime.RandomState = Seed == 0u ? 1u : Seed;
	} else if (Spawn.TypeId == "StationaryShooter") {
		Runtime.HitboxOffset = {8.0f, 1.0f};
		Runtime.HitboxSize = {16.0f, 31.0f};
		Runtime.ContactDamage = 1;
		Runtime.Stompable = true;
		Runtime.Direction = -1;
		Runtime.InitialDirection = -1;
		Runtime.AttackPattern = "radial12";
		Runtime.AttackIntervalFrames = HspShooterDefaultIntervalFrames;
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

	if (Spawn.TypeId == "SeaAnemone") {
		if (!TryReadInteger(
			Spawn.Properties,
			"variant",
			Runtime.Variant,
			Error,
			Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}
		if (Runtime.Variant != SeaAnemoneRightFan &&
			Runtime.Variant != SeaAnemoneLeftFan) {
			return Result<NativeObjectRuntime>::Failure(
				"SeaAnemone variant must be 1 or 2: " +
					Spawn.Id);
		}
	}

	if (Spawn.TypeId == "WallCrawler") {
		if (!TryReadInteger(
			Spawn.Properties,
			"variant",
			Runtime.Variant,
			Error,
			Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant != WallCrawlerCounterClockwise &&
			Runtime.Variant != WallCrawlerClockwise) {
			return Result<NativeObjectRuntime>::Failure(
				"WallCrawler variant must be 1 or 2: " +
					Spawn.Id);
		}

		Runtime.Velocity =
			Runtime.Variant == WallCrawlerCounterClockwise
				? WorldPosition{-WallCrawlerSpeed, 0.0f}
				: WorldPosition{WallCrawlerSpeed, 0.0f};
	}

	if (Spawn.TypeId == "FishEnemy") {
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
				Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant != FishHorizontal &&
			Runtime.Variant != FishHorizontalRange &&
			Runtime.Variant != FishVerticalRange) {
			return Result<NativeObjectRuntime>::Failure(
				"FishEnemy variant must be 1, 2 or 3: " +
					Spawn.Id);
		}

		std::string Direction =
			Runtime.Variant == FishVerticalRange
				? "down"
				: "left";
		if (!TryReadString(
			Spawn.Properties,
			"direction",
			Direction,
			Error,
			Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant == FishVerticalRange) {
			if (Direction == "up") {
				Runtime.Direction = -1;
			} else if (Direction == "down") {
				Runtime.Direction = 1;
			} else {
				return Result<NativeObjectRuntime>::Failure(
					"Vertical FishEnemy direction must be up or down: " +
						Spawn.Id);
			}
			Runtime.InitialDirection = Runtime.Direction;
			Runtime.BehaviorState = Runtime.Direction;
		} else {
			if (Direction == "left") {
				Runtime.Direction = -1;
			} else if (Direction == "right") {
				Runtime.Direction = 1;
			} else {
				return Result<NativeObjectRuntime>::Failure(
					"Horizontal FishEnemy direction must be left or right: " +
						Spawn.Id);
			}
			Runtime.InitialDirection = Runtime.Direction;
		}

		if (Runtime.MoveSpeed <= 0.0f) {
			return Result<NativeObjectRuntime>::Failure(
				"FishEnemy speed must be positive: " + Spawn.Id);
		}
	}

	if (Spawn.TypeId == "FlyingEnemy") {
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
				Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant != FlyingHorizontal &&
			Runtime.Variant != FlyingHorizontalOscillation &&
			Runtime.Variant != FlyingVertical &&
			Runtime.Variant != FlyingVerticalOscillation) {
			return Result<NativeObjectRuntime>::Failure(
				"FlyingEnemy variant must be 1, 2, 3 or 4: " +
					Spawn.Id);
		}

		std::string Direction =
			Runtime.Variant == FlyingVertical
				? "down"
				: (Runtime.Direction < 0 ? "left" : "right");
		if (!TryReadString(
			Spawn.Properties,
			"direction",
			Direction,
			Error,
			Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		if (Runtime.Variant == FlyingVertical) {
			if (Direction == "up") {
				Runtime.Direction = -1;
			} else if (Direction == "down") {
				Runtime.Direction = 1;
			} else {
				return Result<NativeObjectRuntime>::Failure(
					"Vertical FlyingEnemy direction must be up or down: " +
						Spawn.Id);
			}
		} else {
			if (Direction == "left") {
				Runtime.Direction = -1;
			} else if (Direction == "right") {
				Runtime.Direction = 1;
			} else {
				return Result<NativeObjectRuntime>::Failure(
					"Horizontal FlyingEnemy direction must be left or right: " +
						Spawn.Id);
			}
		}
		Runtime.InitialDirection = Runtime.Direction;

		if (Runtime.MoveSpeed <= 0.0f) {
			return Result<NativeObjectRuntime>::Failure(
				"FlyingEnemy speed must be positive: " + Spawn.Id);
		}
	}

	if (Spawn.TypeId == "WalkingEnemy" ||
		Spawn.TypeId == "TransformingWalker" ||
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

	if (Spawn.TypeId == "StationaryShooter") {
		if (!TryReadString(
			Spawn.Properties,
			"pattern",
			Runtime.AttackPattern,
			Error,
			Spawn.Id) ||
			!TryReadInteger(
				Spawn.Properties,
				"intervalFrames",
				Runtime.AttackIntervalFrames,
				Error,
				Spawn.Id)) {
			return Result<NativeObjectRuntime>::Failure(Error);
		}

		const bool KnownPattern =
			Runtime.AttackPattern == "radial12" ||
			Runtime.AttackPattern == "spiralCW12" ||
			Runtime.AttackPattern == "spiralCCW12" ||
			Runtime.AttackPattern == "dualSpiral24" ||
			Runtime.AttackPattern == "bounce4" ||
			Runtime.AttackPattern == "splitDown";
		if (!KnownPattern) {
			return Result<NativeObjectRuntime>::Failure(
				"StationaryShooter pattern is invalid: " + Spawn.Id);
		}
		if (Runtime.AttackIntervalFrames <= 0) {
			return Result<NativeObjectRuntime>::Failure(
				"StationaryShooter intervalFrames must be positive: " +
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
	PendingProjectileSpawns_.clear();

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
	Object.Acceleration = {0.0f, 0.0f};
	Object.Grounded = false;

	if (Object.TypeId == "TransformingWalker") {
		Object.BehaviorState = TransformingWalkerOriginal;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
		Object.MoveSpeed = 2.0f;
		Object.Gravity = 0.5f;
		Object.MaxFallSpeed = 12.0f;
	} else if (Object.TypeId == "Mariri") {
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
		Object.MoveSpeed = MaririMoveSpeed;
		Object.Gravity = 0.4f;
		Object.MaxFallSpeed = 9.0f;
	} else if (Object.TypeId == "SeaAnemone") {
		Object.BehaviorTimer = 0;
		Object.BehaviorPhase = 0.0f;
		Object.ContactEnabled = true;
		Object.Stompable = false;
		Object.ContactDamage = 1;
	} else if (Object.TypeId == "WallCrawler") {
		Object.ContactEnabled = true;
		Object.Stompable = false;
		Object.ContactDamage = 1;
		Object.Velocity =
			Object.Variant == WallCrawlerCounterClockwise
				? WorldPosition{-WallCrawlerSpeed, 0.0f}
				: WorldPosition{WallCrawlerSpeed, 0.0f};
	} else if (Object.TypeId == "FishEnemy") {
		Object.ContactEnabled = true;
		Object.Stompable = false;
		Object.ContactDamage = 1;
		if (Object.Variant == FishVerticalRange) {
			Object.BehaviorState = Object.InitialDirection;
		}
	} else if (Object.TypeId == "Kameen") {
		Object.BehaviorState = KameenWaiting;
		Object.ContactEnabled = false;
		Object.Stompable = false;
		Object.ContactDamage = 1;
	} else if (Object.TypeId == "CarrotMan") {
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
	} else if (Object.TypeId == "StationaryShooter") {
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
	} else if (Object.TypeId == "Pikachii") {
		Object.BehaviorState = PikachiiWaiting;
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
	} else if (Object.TypeId == "Chikorarashi") {
		Object.BehaviorTimer = 0;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
	} else if (Object.TypeId == "FlyingEnemy") {
		Object.BehaviorPhase = 0.0f;
		Object.ContactEnabled = true;
		Object.Stompable = true;
		Object.ContactDamage = 1;
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

			// Pikachiiは大ジャンプ中にCamera上端を大きく越える。
			// 攻撃cycle中まで通常Enemyのoff-camera dormancyを適用すると、
			// 頂点へ到達する前にspawnへresetされ「上へ消える」ため、
			// 着地してWAITへ戻るまでは更新を継続する。
			if (Object.TypeId == "Pikachii" &&
				Object.BehaviorState != PikachiiWaiting) {
				continue;
			}

			// HSP enemyf=29のKameenは起動後、画面外へ出ても追尾を継続する。
			// CHASE中に共通Camera lifecycleでDormantへ戻すと、
			// 画面端を越えた瞬間にspawnへresetされてしまうため除外する。
			if (Object.TypeId == "Kameen" &&
				Object.BehaviorState == KameenChasing) {
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

void NativeObjectSystem::UpdateFlyingEnemy(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active || Object.MoveSpeed <= 0.0f) return;

	if (Object.Variant == FlyingVertical) {
		Object.Velocity.X = 0.0f;
		Object.Velocity.Y =
			static_cast<float>(Object.Direction) * Object.MoveSpeed;
		Object.Position.Y += Object.Velocity.Y;

		const bool MovingDown = Object.Velocity.Y > 0.0f;
		if (ResolveFlyingEnemyVertical(
			Object, Map, Catalog, MovingDown)) {
			Object.Direction *= -1;
			Object.Velocity.Y =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
		}
	} else if (Object.Variant == FlyingVerticalOscillation) {
		// HSP enemyf=7:
		// rad += 0.1
		// y += 5 * cos(rad * 0.5)
		// 向きは移動方向ではなくPlayerの左右位置へ向ける。
		Object.BehaviorPhase += FlyingOscillationPhaseStep;
		if (Object.BehaviorPhase > FlyingOscillationPhaseLimit) {
			Object.BehaviorPhase = 0.0f;
		}

		const float Angle = Object.BehaviorPhase * 0.5f;
		Object.Velocity.X = 0.0f;
		Object.Velocity.Y =
			FlyingOscillationSpeed * std::cos(Angle);
		Object.Position.Y += Object.Velocity.Y;

		if (PlayerPosition.X < Object.Position.X) {
			Object.Direction = -1;
		} else if (PlayerPosition.X > Object.Position.X) {
			Object.Direction = 1;
		}

		const bool MovingDown = Object.Velocity.Y > 0.0f;
		if (ResolveFlyingEnemyVertical(
			Object, Map, Catalog, MovingDown)) {
			// HSP版もterrain hit時にはenemydireを反転する。
			// 次frameにはPlayer位置で再設定される。
			Object.Direction *= -1;
		}
	} else {
		Object.Velocity.Y = 0.0f;

		if (Object.Variant == FlyingHorizontalOscillation) {
			Object.BehaviorPhase += FlyingOscillationPhaseStep;
			if (Object.BehaviorPhase > FlyingOscillationPhaseLimit) {
				Object.BehaviorPhase = 0.0f;
			}

			const float Angle = Object.BehaviorPhase * 0.5f;
			Object.Velocity.X =
				FlyingOscillationSpeed * std::cos(Angle);
			Object.Direction =
				Angle <= 1.57f || Angle > 4.71f ? 1 : -1;
			Object.Position.X += Object.Velocity.X;
		} else {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
			Object.Position.X += Object.Velocity.X;
		}

		const bool HitWall =
			ResolveWalkingEnemySide(Object, Map, Catalog);
		if (HitWall && Object.Variant == FlyingHorizontal) {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * Object.MoveSpeed;
		}
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateWallCrawler(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	if (!Object.Active) return;

	Object.Position.X += Object.Velocity.X;
	Object.Position.Y += Object.Velocity.Y;

	const int X = static_cast<int>(Object.Position.X);
	const int Y = static_cast<int>(Object.Position.Y);
	if (X % Map.TileWidth() != 0 ||
		Y % Map.TileHeight() != 0) {
		return;
	}

	const int Column = X / Map.TileWidth();
	const int Row = Y / Map.TileHeight();
	const CardinalDirection Forward =
		WallCrawlerDirection(Object);
	if (Forward.X == 0 && Forward.Y == 0) return;

	// 壁に片手を付けたまま進むwall followerとして扱う。
	// CCWは進行方向の反時計側、CWは時計側を「壁側」とする。
	const bool CounterClockwise =
		Object.Variant == WallCrawlerCounterClockwise;
	const CardinalDirection WallSide =
		CounterClockwise
			? RotateCounterClockwise(Forward)
			: RotateClockwise(Forward);
	const CardinalDirection AwayFromWall =
		CounterClockwise
			? RotateClockwise(Forward)
			: RotateCounterClockwise(Forward);

	bool Turned = false;

	// 外角: 壁側のtileが空いたら、その方向へ回り込む。
	if (!IsWallCrawlerBlocked(
		Map, Catalog, Column, Row, WallSide)) {
		Object.Velocity =
			ToWallCrawlerVelocity(WallSide);
		Turned = true;
	}
	// 内角: 正面が塞がれたら、壁から離れる側へ90度turnする。
	else if (IsWallCrawlerBlocked(
		Map, Catalog, Column, Row, Forward)) {
		Object.Velocity =
			ToWallCrawlerVelocity(AwayFromWall);
		Turned = true;
	}

	// HSP enemyf=38/39はturnした同じframeに、
	// 新しい方向へさらに1px進む。
	if (Turned) {
		Object.Position.X += Object.Velocity.X;
		Object.Position.Y += Object.Velocity.Y;
	}

	if (Object.Velocity.X < 0.0f) {
		Object.Direction = -1;
	} else if (Object.Velocity.X > 0.0f) {
		Object.Direction = 1;
	}
}

void NativeObjectSystem::UpdateFishEnemy(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active || Object.MoveSpeed <= 0.0f) return;

	if (Object.Variant == FishVerticalRange) {
		const int MoveDirection =
			Object.BehaviorState < 0 ? -1 : 1;
		Object.Velocity.X = 0.0f;
		Object.Velocity.Y =
			static_cast<float>(MoveDirection) * Object.MoveSpeed;
		Object.Position.Y += Object.Velocity.Y;

		const bool ExceededRange =
			std::fabs(Object.Position.Y - Object.InitialPosition.Y) >
			FishRange;
		if (ExceededRange) {
			Object.BehaviorState *= -1;
			Object.Velocity.Y =
				static_cast<float>(
					Object.BehaviorState < 0 ? -1 : 1) *
				Object.MoveSpeed;
		} else {
			const bool MovingDown = Object.Velocity.Y > 0.0f;
			if (ResolveFlyingEnemyVertical(
				Object, Map, Catalog, MovingDown)) {
				Object.BehaviorState *= -1;
				Object.Velocity.Y =
					static_cast<float>(
						Object.BehaviorState < 0 ? -1 : 1) *
					Object.MoveSpeed;
			}
		}

		if (PlayerPosition.X < Object.Position.X) {
			Object.Direction = -1;
		} else if (PlayerPosition.X > Object.Position.X) {
			Object.Direction = 1;
		}
		return;
	}

	Object.Velocity.Y = 0.0f;
	Object.Velocity.X =
		static_cast<float>(Object.Direction) * Object.MoveSpeed;
	Object.Position.X += Object.Velocity.X;

	bool ExceededRange = false;
	if (Object.Variant == FishHorizontalRange) {
		ExceededRange =
			std::fabs(Object.Position.X - Object.InitialPosition.X) >
			FishRange;
		if (ExceededRange) {
			Object.Direction *= -1;
			Object.Velocity.X =
				static_cast<float>(Object.Direction) *
				Object.MoveSpeed;
		}
	}

	if (!ExceededRange) {
		if (ResolveWalkingEnemySide(Object, Map, Catalog)) {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) *
				Object.MoveSpeed;
		}
	}
}

void NativeObjectSystem::UpdateMariri(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	// HSP enemyf=34:
	// enemyvxは水平速度ではなく「次のjumpまでのtimer」として使われる。
	// timer>=100の間だけ空中で3px/frame横移動する。
	if (Object.BehaviorTimer >= MaririJumpTimer) {
		Object.Velocity.X =
			static_cast<float>(Object.Direction) * MaririMoveSpeed;
		Object.Position.X += Object.Velocity.X;
		if (ResolveWalkingEnemySide(Object, Map, Catalog)) {
			Object.Velocity.X =
				static_cast<float>(Object.Direction) * MaririMoveSpeed;
		}
	} else {
		Object.Velocity.X = 0.0f;
	}

	++Object.BehaviorTimer;

	// HSPはtimerが100になったframeに -jump(-9) を設定してから
	// gravity(+0.4)を適用するため、最初の実移動量は -8.6。
	if (Object.BehaviorTimer == MaririJumpTimer) {
		Object.Velocity.Y = -MaririJumpSpeed;
		Object.Grounded = false;
	}

	ResolveWalkingEnemyVertical(Object, Map, Catalog);

	// 着地した時点でjump cycle終了。
	// HSPではtimer>100のときだけ0へ戻し、次の100frame待機へ入る。
	if (Object.Grounded &&
		Object.BehaviorTimer > MaririJumpTimer) {
		Object.BehaviorTimer = 0;
		Object.Velocity.X = 0.0f;
		Object.Direction =
			Object.Position.X >= PlayerPosition.X ? -1 : 1;
	}

	// 待機中は常にPlayerの方向を向き、
	// jump開始時のDirectionがそのまま空中水平移動方向になる。
	if (Object.BehaviorTimer < MaririJumpTimer) {
		Object.Direction =
			Object.Position.X >= PlayerPosition.X ? -1 : 1;
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateKameen(
	NativeObjectRuntime& Object,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	const float DeltaX = PlayerPosition.X - Object.Position.X;
	const float DeltaY = PlayerPosition.Y - Object.Position.Y;
	const float DistanceSquared =
		DeltaX * DeltaX + DeltaY * DeltaY;

	if (Object.BehaviorState == KameenWaiting) {
		Object.Velocity = {0.0f, 0.0f};
		Object.Acceleration = {0.0f, 0.0f};
		Object.ContactEnabled = false;

		if (DistanceSquared <
			KameenTriggerDistance * KameenTriggerDistance) {
			Object.BehaviorState = KameenChasing;
		}
		return;
	}

	Object.ContactEnabled = true;

	// HSP enemyf=29をそのまま寄せる。
	// Playerが右/下の場合だけatan由来のcos/sin成分を使い、
	// 左/上の場合は各axisへ直接 -0.2 を入れる。
	if (PlayerPosition.X > Object.Position.X) {
		const float Angle = std::atan2(
			PlayerPosition.Y - Object.Position.Y,
			PlayerPosition.X - Object.Position.X);
		Object.Acceleration.X =
			KameenAcceleration * std::cos(Angle);
	} else if (PlayerPosition.X < Object.Position.X) {
		Object.Acceleration.X = -KameenAcceleration;
	}

	if (PlayerPosition.Y > Object.Position.Y) {
		const float Angle = std::atan2(
			PlayerPosition.Y - Object.Position.Y,
			PlayerPosition.X - Object.Position.X);
		Object.Acceleration.Y =
			KameenAcceleration * std::sin(Angle);
	} else if (PlayerPosition.Y < Object.Position.Y) {
		Object.Acceleration.Y = -KameenAcceleration;
	}

	Object.Velocity.X = (std::clamp)(
		Object.Velocity.X + Object.Acceleration.X,
		-KameenMaxSpeed,
		KameenMaxSpeed);
	Object.Velocity.Y = (std::clamp)(
		Object.Velocity.Y + Object.Acceleration.Y,
		-KameenMaxSpeed,
		KameenMaxSpeed);

	// HSPは int(enemyv + enemyPos) を毎frame代入する。
	Object.Position.X = static_cast<float>(
		static_cast<int>(Object.Position.X + Object.Velocity.X));
	Object.Position.Y = static_cast<float>(
		static_cast<int>(Object.Position.Y + Object.Velocity.Y));

	if (Object.Velocity.X < 0.0f) {
		Object.Direction = -1;
	} else if (Object.Velocity.X > 0.0f) {
		Object.Direction = 1;
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

void NativeObjectSystem::EmitStationaryShooterPattern(
	const NativeObjectRuntime& Object) {
	const WorldPosition Origin = {
		Object.Position.X + 16.0f,
		Object.Position.Y + 16.0f
	};

	const auto EmitRing = [this, Origin](
		ProjectileMotion Motion,
		float Speed) {
		for (int Index = 0; Index < 12; ++Index) {
			const float Angle =
				ProjectilePi * 2.0f *
				static_cast<float>(Index) / 12.0f;
			ProjectileSpawnRequest Request;
			Request.Position = Origin;
			Request.Motion = Motion;
			Request.Speed = Speed;
			Request.Angle = Angle;
			Request.Damage = 1;
			Request.LifetimeFrames = 360;
			Request.Radius = 5.0f;
			Request.CollidesWithTerrain = true;

			if (Motion == ProjectileMotion::Straight) {
				Request.Velocity = {
					std::cos(Angle) * Speed,
					std::sin(Angle) * Speed
				};
			}
			PendingProjectileSpawns_.push_back(Request);
		}
	};

	if (Object.AttackPattern == "radial12") {
		EmitRing(ProjectileMotion::Straight, 5.0f);
	} else if (Object.AttackPattern == "spiralCW12") {
		EmitRing(ProjectileMotion::SpiralClockwise, 3.0f);
	} else if (Object.AttackPattern == "spiralCCW12") {
		EmitRing(ProjectileMotion::SpiralCounterClockwise, 3.0f);
	} else if (Object.AttackPattern == "dualSpiral24") {
		EmitRing(ProjectileMotion::SpiralClockwise, 3.0f);
		EmitRing(ProjectileMotion::SpiralCounterClockwise, 3.0f);
	} else if (Object.AttackPattern == "bounce4") {
		for (int Index = 0; Index < 4; ++Index) {
			const float Angle =
				ProjectilePi * 0.25f +
				ProjectilePi * 0.5f * static_cast<float>(Index);
			ProjectileSpawnRequest Request;
			Request.Position = Origin;
			Request.Velocity = {
				std::cos(Angle) * 4.0f,
				std::sin(Angle) * 4.0f
			};
			Request.Motion = ProjectileMotion::Straight;
			Request.TerrainResponse =
				ProjectileTerrainResponse::Bounce;
			Request.Damage = 1;
			Request.LifetimeFrames = 480;
			Request.Radius = 5.0f;
			Request.CollidesWithTerrain = true;
			PendingProjectileSpawns_.push_back(Request);
		}
	} else if (Object.AttackPattern == "splitDown") {
		ProjectileSpawnRequest Request;
		Request.Position = Origin;
		Request.Velocity = {0.0f, 5.0f};
		Request.Motion = ProjectileMotion::Straight;
		Request.TerrainResponse =
			ProjectileTerrainResponse::Split;
		Request.SplitCount = 8;
		Request.SplitSpeed = 6.0f;
		Request.Damage = 1;
		Request.LifetimeFrames = 360;
		Request.Radius = 5.0f;
		Request.CollidesWithTerrain = true;
		PendingProjectileSpawns_.push_back(Request);
	}
}

void NativeObjectSystem::UpdatePikachii(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	// HSP enemyf=31/32 は左右向きを別IDで表していたが、
	// V2ではDirectionだけで保持する。
	Object.Direction =
		PlayerPosition.X >= Object.Position.X ? 1 : -1;

	const bool Triggered =
		std::fabs(PlayerPosition.X - Object.Position.X) <
			PikachiiTriggerDistance ||
		Object.BehaviorTimer > PikachiiContinueTriggerFrames;
	if (Triggered) {
		++Object.BehaviorTimer;
	}

	if (Object.BehaviorState == PikachiiWaiting &&
		Object.BehaviorTimer == PikachiiJumpFrame) {
		Object.BehaviorState = PikachiiJumping;
		Object.Velocity.Y = -PikachiiJumpSpeed;
		Object.Grounded = false;
	}

	ResolveWalkingEnemyVertical(Object, Map, Catalog);

	if (Object.BehaviorState == PikachiiJumping &&
		std::fabs(Object.Velocity.Y) < 0.21f &&
		Object.BehaviorTimer > PikachiiJumpFrame) {
		const WorldPosition Origin = {
			Object.Position.X + 16.0f,
			Object.Position.Y + 16.0f
		};
		const WorldPosition Target = {
			PlayerPosition.X + 16.0f,
			PlayerPosition.Y + 16.0f
		};
		const float DX = Target.X - Origin.X;
		const float DY = Target.Y - Origin.Y;
		const float Length = std::sqrt(DX * DX + DY * DY);

		ProjectileSpawnRequest Request;
		Request.Position = Origin;
		Request.Motion = ProjectileMotion::Straight;
		Request.Speed = PikachiiProjectileSpeed;
		Request.Damage = 1;
		Request.LifetimeFrames = 360;
		Request.Radius = 5.0f;
		Request.CollidesWithTerrain = true;
		if (Length > 0.001f) {
			Request.Velocity = {
				DX / Length * PikachiiProjectileSpeed,
				DY / Length * PikachiiProjectileSpeed
			};
		} else {
			Request.Velocity = {
				static_cast<float>(Object.Direction) *
					PikachiiProjectileSpeed,
				0.0f
			};
		}
		PendingProjectileSpawns_.push_back(Request);
		Object.BehaviorState = PikachiiFired;
	}

	if (Object.Grounded &&
		Object.BehaviorTimer > PikachiiJumpFrame) {
		Object.BehaviorState = PikachiiWaiting;
		Object.BehaviorTimer = 0;
		Object.Velocity.Y = 0.0f;
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateSeaAnemone(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	// HSP enemyf=40/41:
	// Playerが横6tiles以内なら160frame charge。
	// charge完了後はPlayerが離れても8発撃ち切るまでcycleを継続する。
	if (std::fabs(Object.Position.X - PlayerPosition.X) <
			SeaAnemoneTriggerDistance ||
		Object.BehaviorTimer > SeaAnemoneChargeFrames) {
		++Object.BehaviorTimer;
	} else {
		Object.BehaviorTimer = 0;
	}

	// HSPではenemyvyを0..159で回しており、実移動には使っていない。
	// animation用phaseとして保持する。
	Object.BehaviorPhase += 1.0f;
	if (Object.BehaviorPhase >= 160.0f) {
		Object.BehaviorPhase = 0.0f;
	}

	if (Object.BehaviorTimer > SeaAnemoneChargeFrames &&
		Object.BehaviorTimer % SeaAnemoneShotIntervalFrames == 0) {
		const int Step =
			(Object.BehaviorTimer - SeaAnemoneChargeFrames) /
				SeaAnemoneShotIntervalFrames +
			1;
		const float Angle =
			3.14f * 2.0f *
			static_cast<float>(Step) / 24.0f;
		const float HorizontalSign =
			Object.Variant == SeaAnemoneRightFan ? 1.0f : -1.0f;

		ProjectileSpawnRequest Request;
		Request.Position = Object.Position;
		Request.Velocity = {
			HorizontalSign * std::cos(Angle) *
				SeaAnemoneProjectileSpeed,
			-std::sin(Angle) * SeaAnemoneProjectileSpeed
		};
		Request.Motion = ProjectileMotion::Ballistic;
		Request.Gravity = SeaAnemoneProjectileGravity;
		Request.MaxFallSpeed = SeaAnemoneProjectileMaxFallSpeed;
		Request.Damage = 1;
		Request.LifetimeFrames = 480;
		Request.Radius = 6.0f;
		Request.CollidesWithTerrain = false;
		PendingProjectileSpawns_.push_back(Request);
	}

	if ((Object.BehaviorTimer - SeaAnemoneChargeFrames) /
			SeaAnemoneShotIntervalFrames ==
		SeaAnemoneShotCount) {
		Object.BehaviorTimer = 0;
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateChikorarashi(
	NativeObjectRuntime& Object,
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	if (!Object.Active) return;

	Object.Direction =
		PlayerPosition.X >= Object.Position.X ? 1 : -1;

	if (std::fabs(PlayerPosition.X - Object.Position.X) <
		ChikorarashiTriggerDistance) {
		++Object.BehaviorTimer;
	}
	if (Object.BehaviorTimer > ChikorarashiCycleFrames) {
		Object.BehaviorTimer = 0;
	}

	if (Object.BehaviorTimer > ChikorarashiFirstShotAfter &&
		Object.BehaviorTimer % ChikorarashiShotModulo ==
			ChikorarashiShotRemainder) {
		auto NextRandom = [&Object]() {
			unsigned int X = Object.RandomState;
			X ^= X << 13;
			X ^= X >> 17;
			X ^= X << 5;
			Object.RandomState = X == 0u ? 1u : X;
			return Object.RandomState;
		};

		const float SpeedX =
			static_cast<float>(NextRandom() % 3u + 1u) *
			static_cast<float>(Object.Direction);
		const float SpeedY =
			-static_cast<float>(NextRandom() % 4u + 4u);

		ProjectileSpawnRequest Request;
		Request.Position = {
			Object.Position.X + 16.0f,
			Object.Position.Y + 16.0f
		};
		Request.Velocity = {SpeedX, SpeedY};
		Request.Motion = ProjectileMotion::Ballistic;
		Request.Gravity = ChikorarashiProjectileGravity;
		Request.Damage = 1;
		Request.LifetimeFrames = 480;
		Request.Radius = 6.0f;
		// HSP eshootf=8 はterrain判定を持たず、地面を貫通する。
		Request.CollidesWithTerrain = false;
		PendingProjectileSpawns_.push_back(Request);
	}

	if (TouchesEnemyDamageTerrain(Object, Map, Catalog)) {
		Object.LifeState = ObjectLifeState::Defeated;
		Object.Active = false;
		Object.Velocity = {0.0f, 0.0f};
	}
}

void NativeObjectSystem::UpdateStationaryShooter(
	NativeObjectRuntime& Object) {
	if (!Object.Active) return;

	++Object.BehaviorTimer;
	if (Object.BehaviorTimer <= Object.AttackIntervalFrames) return;

	Object.BehaviorTimer = 0;
	EmitStationaryShooterPattern(Object);
}

std::vector<ProjectileSpawnRequest>
NativeObjectSystem::TakeProjectileSpawns() {
	std::vector<ProjectileSpawnRequest> Result =
		std::move(PendingProjectileSpawns_);
	PendingProjectileSpawns_.clear();
	return Result;
}

void NativeObjectSystem::Update(
	const TileMap& Map,
	const TileCatalog& Catalog,
	WorldPosition PlayerPosition) {
	for (NativeObjectRuntime& Object : Objects_) {
		if (!Object.Active) continue;
		if (Object.TypeId == "WalkingEnemy" ||
			Object.TypeId == "TransformingWalker") {
			UpdateWalkingEnemy(Object, Map, Catalog);
		} else if (Object.TypeId == "FlyingEnemy") {
			UpdateFlyingEnemy(
				Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "WallCrawler") {
			UpdateWallCrawler(Object, Map, Catalog);
		} else if (Object.TypeId == "FishEnemy") {
			UpdateFishEnemy(
				Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "Mariri") {
			UpdateMariri(
				Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "Kameen") {
			UpdateKameen(Object, PlayerPosition);
		} else if (Object.TypeId == "CarrotMan") {
			UpdateCarrotMan(Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "BallSlime") {
			UpdateBallSlime(Object, Map, Catalog);
		} else if (Object.TypeId == "Pikachii") {
			UpdatePikachii(Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "StationaryShooter") {
			UpdateStationaryShooter(Object);
		} else if (Object.TypeId == "SeaAnemone") {
			UpdateSeaAnemone(
				Object, Map, Catalog, PlayerPosition);
		} else if (Object.TypeId == "Chikorarashi") {
			UpdateChikorarashi(
				Object, Map, Catalog, PlayerPosition);
		}
	}

	// 歩行中Enemy同士が横からぶつかった場合は、互いに反転させる。
	// 地形解決後のHitBoundsで判定し、縦方向の重なりがある組だけを対象にする。
	for (std::size_t LeftIndex = 0; LeftIndex < Objects_.size(); ++LeftIndex) {
		NativeObjectRuntime& Left = Objects_[LeftIndex];
		if (!Left.Active || !IsEnemyCollisionParticipant(Left)) continue;

		for (std::size_t RightIndex = LeftIndex + 1;
			RightIndex < Objects_.size();
			++RightIndex) {
			NativeObjectRuntime& Right = Objects_[RightIndex];
			if (!Right.Active || !IsEnemyCollisionParticipant(Right)) continue;

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

			// 停止Shellは通常の押し返し対象にしない。
			// Kicked Shellからの攻撃判定だけは上で処理済み。
			if (!IsWalkingCollisionEnemy(Left) ||
				!IsWalkingCollisionEnemy(Right)) {
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

	if (Object->TypeId == "TransformingWalker") {
		if (Object->BehaviorState == TransformingWalkerOriginal) {
			// HSP enemyf=26 -> 1, enemyf=27 -> 2.
			// movement variant is already the destination WalkingEnemy variant,
			// so the first stomp only switches visual/behavior state.
			Object->BehaviorState = TransformingWalkerChanged;
			Object->ContactEnabled = true;
			Object->Stompable = true;
			Object->ContactDamage = 1;
			return true;
		}
		return Deactivate(ObjectId);
	}

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
		Object->TypeId != "BallSlime") {
		return false;
	}

	const ObjectHitBounds Bounds = Object->HitBounds();
	const float ObjectCenterX =
		Bounds.Position.X + Bounds.Size.X * 0.5f;

	if (Object->BehaviorState == BallSlimeWalking) {
		// V1では通常歩行中にPlayerへ横接触すると進行方向を反転する。
		if ((PlayerCenterX > ObjectCenterX && Object->Direction > 0) ||
			(PlayerCenterX <= ObjectCenterX && Object->Direction < 0)) {
			Object->Direction *= -1;
		}
		return false;
	}
	if (Object->BehaviorState != BallSlimeShell) {
		return false;
	}

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
