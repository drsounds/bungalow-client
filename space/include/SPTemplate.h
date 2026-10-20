#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct lua_State;

namespace space {

class SPTemplate {
public:
	/// With modelGlobals, each field of an object model is also a template
	/// variable, so ${name} reads model.name.
	static std::string render(const std::string &source,
							  const std::string &method = "GET",
							  const std::string &uri = "",
							  const std::string &action = "",
							  const nlohmann::json &data = nlohmann::json::object(),
							  const nlohmann::json &model = nlohmann::json::object(),
							  bool modelGlobals = false);

	/// Pushes value onto the Lua stack (arrays become 1-based tables).
	static void pushJson(lua_State *state, const nlohmann::json &value);
};

} // namespace space
