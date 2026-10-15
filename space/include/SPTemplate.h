#pragma once

#include <boost/json.hpp>
#include <string>

namespace space {

class SPTemplate {
public:
	static std::string render(const std::string &source,
							  const std::string &method = "GET",
							  const std::string &uri = "",
							  const std::string &action = "",
							  const boost::json::object &data = {},
							  const boost::json::object &model = {});
};

} // namespace space
