#pragma once

#include <string>

namespace uchinoko {

class AssetPaths {
public:
	explicit AssetPaths(std::string DataRoot = "dat");

	const std::string& DataRoot() const { return DataRoot_; }
	std::string Data(const std::string& RelativePath) const;
	std::string Image(const std::string& FileName) const;
	std::string SoundEffect(const std::string& FileName) const;
	std::string Bgm(const std::string& FileName) const;
	std::string Stage(int StageNumber, const std::string& FileName) const;

private:
	static std::string Normalize(std::string Path);
	static std::string Join(const std::string& Left, const std::string& Right);

	std::string DataRoot_;
};

} // namespace uchinoko
