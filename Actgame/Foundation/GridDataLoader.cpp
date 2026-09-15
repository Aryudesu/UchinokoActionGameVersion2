#include "GridDataLoader.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace uchinoko {
namespace {

std::string Trim(const std::string& Value) {
	const std::string Whitespace = " \t\r\n";
	const std::size_t Begin = Value.find_first_not_of(Whitespace);
	if (Begin == std::string::npos) return "";
	const std::size_t End = Value.find_last_not_of(Whitespace);
	return Value.substr(Begin, End - Begin + 1);
}

Result<int> ParseInteger(const std::string& Value, int Line, int Column) {
	const std::string Trimmed = Trim(Value);
	if (Trimmed.empty()) {
		return Result<int>::Failure(
			"Empty value at line " + std::to_string(Line) +
			", column " + std::to_string(Column));
	}

	errno = 0;
	char* End = nullptr;
	const long Parsed = std::strtol(Trimmed.c_str(), &End, 10);
	if (errno == ERANGE || Parsed < INT_MIN || Parsed > INT_MAX ||
		End == Trimmed.c_str() || *End != '\0') {
		return Result<int>::Failure(
			"Invalid integer '" + Trimmed + "' at line " +
			std::to_string(Line) + ", column " + std::to_string(Column));
	}
	return Result<int>::Success(static_cast<int>(Parsed));
}

} // namespace

Result<IntegerGrid> GridDataLoader::Load(const std::string& FileName) {
	std::ifstream File(FileName, std::ios::binary);
	if (!File) {
		return Result<IntegerGrid>::Failure("Could not open grid file: " + FileName);
	}
	std::ostringstream Buffer;
	Buffer << File.rdbuf();
	Result<IntegerGrid> Parsed = Parse(Buffer.str());
	if (Parsed.IsFailure()) {
		return Result<IntegerGrid>::Failure(FileName + ": " + Parsed.Error());
	}
	return Parsed;
}

Result<IntegerGrid> GridDataLoader::Parse(const std::string& Text) {
	IntegerGrid Grid;
	std::istringstream Lines(Text);
	std::string Line;
	int LineNumber = 0;
	std::size_t ExpectedWidth = 0;

	while (std::getline(Lines, Line)) {
		++LineNumber;
		if (LineNumber == 1 && Line.size() >= 3 &&
			static_cast<unsigned char>(Line[0]) == 0xEF &&
			static_cast<unsigned char>(Line[1]) == 0xBB &&
			static_cast<unsigned char>(Line[2]) == 0xBF) {
			Line.erase(0, 3);
		}
		if (Trim(Line).empty()) continue;

		std::vector<int> Row;
		std::istringstream Cells(Line);
		std::string Cell;
		int ColumnNumber = 0;
		while (std::getline(Cells, Cell, ',')) {
			++ColumnNumber;
			Result<int> Value = ParseInteger(Cell, LineNumber, ColumnNumber);
			if (Value.IsFailure()) return Result<IntegerGrid>::Failure(Value.Error());
			Row.push_back(Value.Value());
		}

		if (Grid.empty()) {
			ExpectedWidth = Row.size();
			if (ExpectedWidth == 0) {
				return Result<IntegerGrid>::Failure("Grid has no columns");
			}
		} else if (Row.size() != ExpectedWidth) {
			return Result<IntegerGrid>::Failure(
				"Row width mismatch at line " + std::to_string(LineNumber) +
				": expected " + std::to_string(ExpectedWidth) +
				", got " + std::to_string(Row.size()));
		}
		Grid.push_back(std::move(Row));
	}

	if (Grid.empty()) return Result<IntegerGrid>::Failure("Grid has no rows");
	return Result<IntegerGrid>::Success(std::move(Grid));
}

} // namespace uchinoko
