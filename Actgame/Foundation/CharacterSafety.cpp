#include "CharacterSafety.h"

#include "TileDefinition.h"
#include "TileMap.h"

#include <algorithm>
#include <cmath>

namespace uchinoko {
namespace {

constexpr float Epsilon = 0.001f;

struct Candidate {
	WorldPosition Position;
	float Distance = 0.0f;
};

} // namespace

bool CharacterSafety::OverlapsTile(
	const CharacterBody& Body, TilePosition Tile,
	int TileWidth, int TileHeight) {
	const float Left = static_cast<float>(Tile.Column * TileWidth);
	const float Top = static_cast<float>(Tile.Row * TileHeight);
	const float Right = Left + TileWidth;
	const float Bottom = Top + TileHeight;

	return Body.Position.X < Right - Epsilon &&
		Body.Position.X + Body.Width > Left + Epsilon &&
		Body.Position.Y < Bottom - Epsilon &&
		Body.Position.Y + Body.Height > Top + Epsilon;
}

bool CharacterSafety::OverlapsSolid(
	const CharacterBody& Body,
	const TileMap& Map, const TileCatalog& Catalog) {
	const int FirstColumn = static_cast<int>(std::floor(
		(Body.Position.X + Epsilon) / Map.TileWidth()));
	const int LastColumn = static_cast<int>(std::floor(
		(Body.Position.X + Body.Width - Epsilon) / Map.TileWidth()));
	const int FirstRow = static_cast<int>(std::floor(
		(Body.Position.Y + Epsilon) / Map.TileHeight()));
	const int LastRow = static_cast<int>(std::floor(
		(Body.Position.Y + Body.Height - Epsilon) / Map.TileHeight()));

	for (int Row = FirstRow; Row <= LastRow; ++Row) {
		for (int Column = FirstColumn; Column <= LastColumn; ++Column) {
			const int* Id = Map.TryGet({Column, Row});
			const TileDefinition* Definition =
				Id == nullptr ? nullptr : Catalog.Find(*Id);
			if (Definition != nullptr &&
				Definition->Collision == CollisionShape::Solid) {
				return true;
			}
		}
	}
	return false;
}

CharacterSafetyResult CharacterSafety::ResolveActivatedSolids(
	CharacterBody& Body,
	const TileMap& Map, const TileCatalog& Catalog,
	const std::vector<TilePosition>& ActivatedSolidTiles) {
	CharacterSafetyResult Result;

	// 複数ブロックが同時に現れるケースもあるため、押し出し後に再評価する。
	for (std::size_t Pass = 0;
		Pass <= ActivatedSolidTiles.size(); ++Pass) {
		bool FoundOverlap = false;

		for (std::size_t Index = 0; Index < ActivatedSolidTiles.size(); ++Index) {
			const TilePosition Tile = ActivatedSolidTiles[Index];
			if (!OverlapsTile(Body, Tile, Map.TileWidth(), Map.TileHeight())) continue;
			FoundOverlap = true;

			const float Left = static_cast<float>(Tile.Column * Map.TileWidth());
			const float Top = static_cast<float>(Tile.Row * Map.TileHeight());
			const float Right = Left + Map.TileWidth();
			const float Bottom = Top + Map.TileHeight();

			Candidate Candidates[4];
			Candidates[0].Position = {Left - Body.Width, Body.Position.Y};
			Candidates[1].Position = {Right, Body.Position.Y};
			Candidates[2].Position = {Body.Position.X, Top - Body.Height};
			Candidates[3].Position = {Body.Position.X, Bottom};

			for (int CandidateIndex = 0; CandidateIndex < 4; ++CandidateIndex) {
				const float Dx = Candidates[CandidateIndex].Position.X - Body.Position.X;
				const float Dy = Candidates[CandidateIndex].Position.Y - Body.Position.Y;
				Candidates[CandidateIndex].Distance = std::fabs(Dx) + std::fabs(Dy);
			}
			std::sort(
				Candidates, Candidates + 4,
				[](const Candidate& LeftCandidate, const Candidate& RightCandidate) {
					return LeftCandidate.Distance < RightCandidate.Distance;
				});

			bool Escaped = false;
			for (int CandidateIndex = 0; CandidateIndex < 4; ++CandidateIndex) {
				CharacterBody Moved = Body;
				Moved.Position = Candidates[CandidateIndex].Position;
				if (OverlapsSolid(Moved, Map, Catalog)) continue;

				Body.Position = Moved.Position;
				Body.Velocity = {0.0f, 0.0f};
				Body.Grounded = false;
				Result.Repositioned = true;
				Escaped = true;
				break;
			}

			if (!Escaped) {
				Result.Crushed = true;
				TileEffect Death;
				Death.Type = TileEffectType::InstantDeath;
				Death.Position = Tile;
				Death.Value = 1;
				Result.Effects.push_back(Death);
				return Result;
			}

			break;
		}

		if (!FoundOverlap) return Result;
	}

	// 安全側: 同時出現が多く収束しなかった場合は圧死とする。
	Result.Crushed = true;
	TileEffect Death;
	Death.Type = TileEffectType::InstantDeath;
	Death.Value = 1;
	Result.Effects.push_back(Death);
	return Result;
}

} // namespace uchinoko
