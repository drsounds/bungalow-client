#include "SPTemplate.h"

#include <lua.hpp>

#include "elua.h"

#include <memory>
#include <regex>
#include <stdexcept>

namespace space {
namespace {

class EluaContext {
public:
	EluaContext() { elua_init_context(&value); }

	~EluaContext() { elua_free_context(&value); }

	elua_context_t value{};
};

std::string convertTemplate(const std::string &source) {
	static const std::regex countLoop(
		R"(^\s*for\s+([A-Za-z_]\w*)\s*,\s*(.+?)\s+do\s*$)");
	std::string converted;
	size_t lineStart = 0;

	while (lineStart < source.size()) {
		const size_t lineEnd = source.find('\n', lineStart);
		const size_t lineLength = (lineEnd == std::string::npos)
									  ? source.size() - lineStart
									  : lineEnd - lineStart;
		const std::string line = source.substr(lineStart, lineLength);
		const size_t first = line.find_first_not_of(" \t\r");

		if (first != std::string::npos && line[first] == '%') {
			std::string code = line.substr(first + 1);
			std::smatch match;
			if (std::regex_match(code, match, countLoop)) {
				code = "for " + match[1].str() + "=1," + match[2].str() + " do";
			}
			converted += "<%";
			converted += code;
			converted += "%>";
		} else {
			size_t pos = 0;
			while (pos < line.size()) {
				const size_t open = line.find("${", pos);
				if (open == std::string::npos) {
					converted.append(line, pos);
					break;
				}
				const size_t close = line.find('}', open + 2);
				if (close == std::string::npos) {
					converted.append(line, pos);
					break;
				}
				converted.append(line, pos, open - pos);
				converted += "<%=";
				converted.append(line, open + 2, close - open - 2);
				converted += "%>";
				pos = close + 1;
			}
			if (lineEnd != std::string::npos)
				converted += '\n';
		}

		if (lineEnd == std::string::npos)
			break;
		lineStart = lineEnd + 1;
	}
	return converted;
}

void pushJson(lua_State *state, const boost::json::value &value) {
	switch (value.kind()) {
	case boost::json::kind::null:
		lua_pushnil(state);
		break;
	case boost::json::kind::bool_:
		lua_pushboolean(state, value.as_bool());
		break;
	case boost::json::kind::int64:
		lua_pushnumber(state, static_cast<lua_Number>(value.as_int64()));
		break;
	case boost::json::kind::uint64:
		lua_pushnumber(state, static_cast<lua_Number>(value.as_uint64()));
		break;
	case boost::json::kind::double_:
		lua_pushnumber(state, static_cast<lua_Number>(value.as_double()));
		break;
	case boost::json::kind::string: {
		const auto &text = value.as_string();
		lua_pushlstring(state, text.data(), text.size());
		break;
	}
	case boost::json::kind::array: {
		lua_createtable(state, static_cast<int>(value.as_array().size()), 0);
		lua_Integer index = 1;
		for (const auto &item : value.as_array()) {
			pushJson(state, item);
			lua_rawseti(state, -2, index++);
		}
		break;
	}
	case boost::json::kind::object: {
		lua_createtable(state, 0, static_cast<int>(value.as_object().size()));
		for (const auto &item : value.as_object()) {
			pushJson(state, item.value());
			lua_setfield(state, -2, item.key_c_str());
		}
		break;
	}
	}
}

void pushObject(lua_State *state, const boost::json::object &object) {
	pushJson(state, boost::json::value(object));
}

int appendPrint(lua_State *state) {
	auto *output =
		static_cast<std::string *>(lua_touserdata(state, lua_upvalueindex(1)));
	const int count = lua_gettop(state);
	for (int i = 1; i <= count; ++i) {
		if (i > 1)
			output->push_back('\t');
		if (lua_isnil(state, i))
			continue;
		if (lua_isboolean(state, i)) {
			output->append(lua_toboolean(state, i) ? "true" : "false");
			continue;
		}
		size_t size = 0;
		const char *text = lua_tolstring(state, i, &size);
		if (text)
			output->append(text, size);
	}
	return 0;
}

} // namespace

std::string SPTemplate::render(const std::string &source,
							   const std::string &method,
							   const std::string &uri,
							   const std::string &action,
							   const boost::json::object &data,
							   const boost::json::object &model) {
	using LuaState = std::unique_ptr<lua_State, decltype(&lua_close)>;
	LuaState state(luaL_newstate(), &lua_close);
	if (!state)
		throw std::runtime_error("Unable to create Lua state");

	luaL_openlibs(state.get());
	std::string output;
	lua_pushlightuserdata(state.get(), &output);
	lua_pushcclosure(state.get(), appendPrint, 1);
	lua_setglobal(state.get(), "print");

	lua_pushlstring(state.get(), method.data(), method.size());
	lua_setglobal(state.get(), "method");
	lua_pushlstring(state.get(), uri.data(), uri.size());
	lua_setglobal(state.get(), "uri");
	lua_pushlstring(state.get(), action.data(), action.size());
	lua_setglobal(state.get(), "action");
	pushObject(state.get(), data);
	lua_setglobal(state.get(), "data");
	pushObject(state.get(), model);
	lua_setglobal(state.get(), "model");

	const std::string converted = convertTemplate(source);
	EluaContext context;
	int status =
		elua_loadbuffer(state.get(), converted.data(), converted.size(),
						"spider-template", &context.value);
	if (status == 0)
		status = lua_pcall(state.get(), 0, 0, 0);

	if (status != 0) {
		const char *error = lua_tostring(state.get(), -1);
		const std::string message =
			error ? error : "Unknown Lua template error";
		throw std::runtime_error(message);
	}

	return output;
}

} // namespace space
