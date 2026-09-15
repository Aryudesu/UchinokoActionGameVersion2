#pragma once

#include <string>
#include <utility>

namespace uchinoko {

template <class T>
class Result {
public:
	static Result Success(T Value) {
		return Result(true, std::move(Value), "");
	}

	static Result Failure(std::string Error) {
		return Result(false, T(), std::move(Error));
	}

	bool IsSuccess() const { return Success_; }
	bool IsFailure() const { return !Success_; }
	const T& Value() const { return Value_; }
	T& Value() { return Value_; }
	const std::string& Error() const { return Error_; }

private:
	Result(bool Success, T Value, std::string Error)
		: Success_(Success), Value_(std::move(Value)), Error_(std::move(Error)) {}

	bool Success_;
	T Value_;
	std::string Error_;
};

} // namespace uchinoko
