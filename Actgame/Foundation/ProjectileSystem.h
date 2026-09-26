#pragma once

#include "Coordinates.h"
#include "TileDefinition.h"
#include "TileMap.h"

#include <cstddef>
#include <vector>

namespace uchinoko {

enum class ProjectileMotion {
	Straight,
	SpiralClockwise,
	SpiralCounterClockwise,
	Ballistic
};

enum class ProjectileTerrainResponse {
	Deactivate,
	Bounce,
	Split
};

struct ProjectileSpawnRequest {
	WorldPosition Position;
	WorldPosition Velocity;
	ProjectileMotion Motion = ProjectileMotion::Straight;
	float Speed = 0.0f;
	float Angle = 0.0f;
	float Gravity = 0.0f;
	ProjectileTerrainResponse TerrainResponse =
		ProjectileTerrainResponse::Deactivate;
	int SplitCount = 8;
	float SplitSpeed = 6.0f;
	int Damage = 1;
	int LifetimeFrames = 360;
	float Radius = 5.0f;
	bool CollidesWithTerrain = true;
};

struct ProjectileRuntime {
	WorldPosition Position;
	WorldPosition Origin;
	WorldPosition Velocity;
	ProjectileMotion Motion = ProjectileMotion::Straight;
	float Speed = 0.0f;
	float Angle = 0.0f;
	float RadiusFromOrigin = 0.0f;
	float Gravity = 0.0f;
	ProjectileTerrainResponse TerrainResponse =
		ProjectileTerrainResponse::Deactivate;
	int SplitCount = 8;
	float SplitSpeed = 6.0f;
	float HitRadius = 5.0f;
	int Damage = 1;
	int AgeFrames = 0;
	int LifetimeFrames = 360;
	bool CollidesWithTerrain = true;
	bool Active = true;
};

struct ProjectileContact {
	std::size_t ProjectileIndex = 0;
	int Damage = 0;
	float SourceCenterX = 0.0f;
};

class ProjectileSystem {
public:
	void Reset();
	void Spawn(const ProjectileSpawnRequest& Request);
	void SpawnAll(const std::vector<ProjectileSpawnRequest>& Requests);

	void Update(
		const TileMap& Map,
		const TileCatalog& Catalog);

	std::vector<ProjectileContact> FindContacts(
		WorldPosition Position,
		WorldPosition Size) const;

	bool Deactivate(std::size_t ProjectileIndex);

	const std::vector<ProjectileRuntime>& Projectiles() const {
		return Projectiles_;
	}

private:
	static bool TouchesTerrain(
		const ProjectileRuntime& Projectile,
		const TileMap& Map,
		const TileCatalog& Catalog);

	std::vector<ProjectileRuntime> Projectiles_;
};

} // namespace uchinoko
