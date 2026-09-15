#include "AssetPaths.h"

#include <algorithm>
#include <utility>

namespace uchinoko {

AssetPaths::AssetPaths(std::string DataRoot)
	: DataRoot_(Normalize(std::move(DataRoot))) {
	while (DataRoot_.size() > 1 && DataRoot_.back() == '/') DataRoot_.pop_back();
}

std::string AssetPaths::Data(const std::string& RelativePath) const {
	return Join(DataRoot_, RelativePath);
}

std::string AssetPaths::Image(const std::string& FileName) const {
	return Data(Join("img", FileName));
}

std::string AssetPaths::SoundEffect(const std::string& FileName) const {
	return Data(Join("SE", FileName));
}

std::string AssetPaths::Bgm(const std::string& FileName) const {
	return Data(Join("BGM", FileName));
}

std::string AssetPaths::Stage(int StageNumber, const std::string& FileName) const {
	return Data(Join(Join("stage", std::to_string(StageNumber)), FileName));
}

std::string AssetPaths::Normalize(std::string Path) {
	std::replace(Path.begin(), Path.end(), '\\', '/');
	return Path;
}

std::string AssetPaths::Join(const std::string& Left, const std::string& Right) {
	std::string NormalizedLeft = Normalize(Left);
	std::string NormalizedRight = Normalize(Right);
	while (!NormalizedRight.empty() && NormalizedRight.front() == '/') {
		NormalizedRight.erase(NormalizedRight.begin());
	}
	if (NormalizedLeft.empty()) return NormalizedRight;
	if (NormalizedRight.empty()) return NormalizedLeft;
	if (NormalizedLeft.back() != '/') NormalizedLeft += '/';
	return NormalizedLeft + NormalizedRight;
}

} // namespace uchinoko
