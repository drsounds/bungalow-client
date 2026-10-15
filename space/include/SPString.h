#pragma once

#include <string>
#include <vector>

namespace space {

inline bool startsWith(const std::string &value, const std::string &prefix) {
	return value.compare(0, prefix.size(), prefix) == 0;
}

inline bool endsWith(const std::string &value, const std::string &suffix) {
	return value.size() >= suffix.size() &&
		   value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::vector<std::string> split(const std::string &value, char delimiter) {
	std::vector<std::string> parts;
	std::size_t start = 0;
	std::size_t end;
	while ((end = value.find(delimiter, start)) != std::string::npos) {
		parts.emplace_back(value, start, end - start);
		start = end + 1;
	}
	parts.emplace_back(value, start);
	return parts;
}

} // namespace space
