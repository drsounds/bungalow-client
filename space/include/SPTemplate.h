#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace space {

class SPTemplate {
public:
	static std::string render(const std::string &source,
							  const std::string &method = "GET",
							  const std::string &uri = "",
							  const std::string &action = "",
							  const nlohmann::json &data = nlohmann::json::object(),
							  const nlohmann::json &model = nlohmann::json::object());
};

} // namespace space
