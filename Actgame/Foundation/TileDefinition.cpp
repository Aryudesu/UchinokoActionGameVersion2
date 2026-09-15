#include "TileDefinition.h"

#include <utility>

namespace uchinoko {

Result<bool> TileCatalog::Register(TileDefinition Definition) {
	if (Find(Definition.Id) != nullptr) {
		return Result<bool>::Failure("Tile ID " + std::to_string(Definition.Id) + " is already registered");
	}
	Definitions_.push_back(std::move(Definition));
	return Result<bool>::Success(true);
}

const TileDefinition* TileCatalog::Find(int Id) const {
	for (const TileDefinition& Definition : Definitions_) {
		if (Definition.Id == Id) return &Definition;
	}
	return nullptr;
}

} // namespace uchinoko
