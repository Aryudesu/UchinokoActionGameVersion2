#include "ProjectileSystem.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {

namespace {

constexpr float Pi = 3.14159265358979323846f;
constexpr float SpiralAngularSpeed = 0.02f;

bool IntersectsCircleRectangle(
	WorldPosition Center,
	float Radius,
	WorldPosition RectanglePosition,
	WorldPosition RectangleSize) {
	const float NearestX =
		(std::max)(
			RectanglePosition.X,
			(std::min)(
				Center.X,
				RectanglePosition.X + RectangleSize.X));
	const float NearestY =
		(std::max)(
			RectanglePosition.Y,
			(std::min)(
				Center.Y,
				RectanglePosition.Y + RectangleSize.Y));
	const float DX = Center.X - NearestX;
	const float DY = Center.Y - NearestY;
	return DX * DX + DY * DY <= Radius * Radius;
}

bool ShapeBlocksProjectile(CollisionShape Shape) {
	return Shape != CollisionShape::None;
}

} // namespace

void ProjectileSystem::Reset() {
	Projectiles_.clear();
}

void ProjectileSystem::Spawn(
	const ProjectileSpawnRequest& Request) {
	ProjectileRuntime Projectile;
	Projectile.Position = Request.Position;
	Projectile.Origin = Request.Position;
	Projectile.Velocity = Request.Velocity;
	Projectile.Motion = Request.Motion;
	Projectile.Speed = Request.Speed;
	Projectile.Angle = Request.Angle;
	Projectile.Gravity = Request.Gravity;
	Projectile.TerrainResponse = Request.TerrainResponse;
	Projectile.SplitCount = Request.SplitCount;
	Projectile.SplitSpeed = Request.SplitSpeed;
	Projectile.Damage = Request.Damage;
	Projectile.LifetimeFrames = Request.LifetimeFrames;
	Projectile.HitRadius = Request.Radius;
	Projectile.CollidesWithTerrain = Request.CollidesWithTerrain;

	for (ProjectileRuntime& Existing : Projectiles_) {
		if (Existing.Active) continue;
		Existing = Projectile;
		return;
	}
	Projectiles_.push_back(Projectile);
}

void ProjectileSystem::SpawnAll(
	const std::vector<ProjectileSpawnRequest>& Requests) {
	for (const ProjectileSpawnRequest& Request : Requests) {
		Spawn(Request);
	}
}

bool ProjectileSystem::TouchesTerrain(
	const ProjectileRuntime& Projectile,
	const TileMap& Map,
	const TileCatalog& Catalog) {
	const int Column = static_cast<int>(
		std::floor(Projectile.Position.X / Map.TileWidth()));
	const int Row = static_cast<int>(
		std::floor(Projectile.Position.Y / Map.TileHeight()));
	const int* Id = Map.TryGet({Column, Row});
	if (Id == nullptr) return true;

	const TileDefinition* Definition = Catalog.Find(*Id);
	return Definition != nullptr &&
		ShapeBlocksProjectile(Definition->Collision);
}

void ProjectileSystem::Update(
	const TileMap& Map,
	const TileCatalog& Catalog) {
	std::vector<ProjectileSpawnRequest> DeferredSpawns;

	for (ProjectileRuntime& Projectile : Projectiles_) {
		if (!Projectile.Active) continue;

		++Projectile.AgeFrames;
		if (Projectile.AgeFrames > Projectile.LifetimeFrames) {
			Projectile.Active = false;
			continue;
		}

		if (Projectile.Motion == ProjectileMotion::Straight ||
			Projectile.Motion == ProjectileMotion::Ballistic) {
			if (Projectile.Motion == ProjectileMotion::Ballistic) {
				Projectile.Velocity.Y += Projectile.Gravity;
			}

			const WorldPosition Previous = Projectile.Position;

			if (Projectile.CollidesWithTerrain &&
				Projectile.TerrainResponse ==
					ProjectileTerrainResponse::Bounce) {
				Projectile.Position.X += Projectile.Velocity.X;
				if (TouchesTerrain(Projectile, Map, Catalog)) {
					Projectile.Position.X = Previous.X;
					Projectile.Velocity.X *= -1.0f;
				}

				Projectile.Position.Y += Projectile.Velocity.Y;
				if (TouchesTerrain(Projectile, Map, Catalog)) {
					Projectile.Position.Y = Previous.Y;
					Projectile.Velocity.Y *= -1.0f;
				}
			} else {
				Projectile.Position.X += Projectile.Velocity.X;
				Projectile.Position.Y += Projectile.Velocity.Y;
			}
		} else {
			const float AngularDelta =
				Projectile.Motion == ProjectileMotion::SpiralClockwise
					? SpiralAngularSpeed
					: -SpiralAngularSpeed;
			Projectile.Angle += AngularDelta;
			if (Projectile.Angle > Pi * 2.0f) {
				Projectile.Angle -= Pi * 2.0f;
			} else if (Projectile.Angle < -Pi * 2.0f) {
				Projectile.Angle += Pi * 2.0f;
			}
			Projectile.RadiusFromOrigin += Projectile.Speed;
			Projectile.Position.X =
				Projectile.Origin.X +
				std::cos(Projectile.Angle) *
					Projectile.RadiusFromOrigin;
			Projectile.Position.Y =
				Projectile.Origin.Y -
				std::sin(Projectile.Angle) *
					Projectile.RadiusFromOrigin;
		}

		const float WorldWidth =
			static_cast<float>(Map.Width() * Map.TileWidth());
		const float WorldHeight =
			static_cast<float>(Map.Height() * Map.TileHeight());
		if (Projectile.Position.X < -Projectile.HitRadius ||
			Projectile.Position.Y < -Projectile.HitRadius ||
			Projectile.Position.X > WorldWidth + Projectile.HitRadius ||
			Projectile.Position.Y > WorldHeight + Projectile.HitRadius) {
			Projectile.Active = false;
			continue;
		}

		if (!Projectile.CollidesWithTerrain ||
			Projectile.TerrainResponse ==
				ProjectileTerrainResponse::Bounce) {
			continue;
		}

		if (!TouchesTerrain(Projectile, Map, Catalog)) continue;

		if (Projectile.TerrainResponse ==
			ProjectileTerrainResponse::Split) {
			const int Count = (std::max)(1, Projectile.SplitCount);
			for (int Index = 0; Index < Count; ++Index) {
				const float Angle =
					Pi * 2.0f * static_cast<float>(Index) /
					static_cast<float>(Count);

				ProjectileSpawnRequest Child;
				Child.Position = Projectile.Position;
				Child.Velocity = {
					std::cos(Angle) * Projectile.SplitSpeed,
					std::sin(Angle) * Projectile.SplitSpeed
				};
				Child.Motion = ProjectileMotion::Straight;
				Child.Damage = Projectile.Damage;
				Child.LifetimeFrames = Projectile.LifetimeFrames;
				Child.Radius = Projectile.HitRadius;
				// HSP eshootf=6相当。分裂後はterrainを無視して直進。
				Child.CollidesWithTerrain = false;
				DeferredSpawns.push_back(Child);
			}
		}

		Projectile.Active = false;
	}

	SpawnAll(DeferredSpawns);
}

std::vector<ProjectileContact> ProjectileSystem::FindContacts(
	WorldPosition Position,
	WorldPosition Size) const {
	std::vector<ProjectileContact> Contacts;
	for (std::size_t Index = 0; Index < Projectiles_.size(); ++Index) {
		const ProjectileRuntime& Projectile = Projectiles_[Index];
		if (!Projectile.Active || Projectile.Damage <= 0) continue;
		if (!IntersectsCircleRectangle(
			Projectile.Position,
			Projectile.HitRadius,
			Position,
			Size)) {
			continue;
		}

		ProjectileContact Contact;
		Contact.ProjectileIndex = Index;
		Contact.Damage = Projectile.Damage;
		Contact.SourceCenterX = Projectile.Position.X;
		Contacts.push_back(Contact);
	}
	return Contacts;
}

bool ProjectileSystem::Deactivate(
	std::size_t ProjectileIndex) {
	if (ProjectileIndex >= Projectiles_.size()) return false;
	ProjectileRuntime& Projectile = Projectiles_[ProjectileIndex];
	if (!Projectile.Active) return false;
	Projectile.Active = false;
	return true;
}

} // namespace uchinoko
