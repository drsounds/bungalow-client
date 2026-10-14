#include "SPTemplate.h"

#include <lauxlib.h>
#include <lualib.h>

#include <memory>
#include <regex>
#include <stdexcept>
#include <string_view>

namespace space {
namespace {

std::string trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string_view::npos)
        return {};
    const auto last = text.find_last_not_of(" \t\r\n\f\v");
    return std::string(text.substr(first, last - first + 1));
}

std::string quoteLua(std::string_view text) {
    std::string quoted;
    quoted.reserve(text.size() + 2);
    quoted.push_back('"');
    for (unsigned char c : text) {
        switch (c) {
            case '\\': quoted += "\\\\"; break;
            case '"':  quoted += "\\\""; break;
            case '\n': quoted += "\\n"; break;
            case '\r': quoted += "\\r"; break;
            case '\t': quoted += "\\t"; break;
            default:
                if (c < 32 || c == 127) {
                    quoted.push_back('\\');
                    quoted.push_back(static_cast<char>('0' + c / 100));
                    quoted.push_back(static_cast<char>('0' + (c / 10) % 10));
                    quoted.push_back(static_cast<char>('0' + c % 10));
                } else {
                    quoted.push_back(static_cast<char>(c));
                }
        }
    }
    quoted.push_back('"');
    return quoted;
}

void emitLiteral(std::string& output, std::string_view text) {
    if (!text.empty())
        output += " __emit(" + quoteLua(text) + ")";
}

std::string compileTextLine(std::string_view line) {
    std::string output;
    size_t position = 0;
    while (position < line.size()) {
        const size_t open = line.find("${", position);
        if (open == std::string_view::npos) {
            emitLiteral(output, line.substr(position));
            break;
        }
        const size_t close = line.find('}', open + 2);
        if (close == std::string_view::npos) {
            emitLiteral(output, line.substr(position));
            break;
        }
        emitLiteral(output, line.substr(position, open - position));
        const std::string expression = trim(line.substr(open + 2, close - open - 2));
        output += " __emit(__str(" + expression + "))";
        position = close + 1;
    }
    emitLiteral(output, "\n");
    return output;
}

std::string compileCodeLine(std::string code) {
    static const std::regex countFor(
        R"(^for\s+([A-Za-z_]\w*)\s*,\s*(.+?)\s+do$)");
    std::smatch match;
    if (std::regex_match(code, match, countFor))
        return "for " + match[1].str() + "=1," + trim(match[2].str()) + " do";
    return code;
}

std::string compile(std::string_view source) {
    std::string lua =
        "local __out = {}\n"
        "local function __emit(s) __out[#__out + 1] = s end\n"
        "local function __str(v) if v == nil then return \"\" end return tostring(v) end\n";

    size_t start = 0;
    while (true) {
        const size_t newline = source.find('\n', start);
        const std::string_view line = source.substr(
            start, newline == std::string_view::npos ? source.size() - start : newline - start);
        const auto first = line.find_first_not_of(" \t\r\n\f\v");
        if (first != std::string_view::npos && line[first] == '%') {
            lua += compileCodeLine(trim(line.substr(first + 1)));
            lua.push_back('\n');
        } else {
            lua += compileTextLine(line);
            lua.push_back('\n');
        }
        if (newline == std::string_view::npos)
            break;
        start = newline + 1;
    }
    lua += "return table.concat(__out)\n";
    return lua;
}

using LuaState = std::unique_ptr<lua_State, decltype(&lua_close)>;

void setString(lua_State* state, const char* name, const std::string& value) {
    lua_pushlstring(state, value.data(), value.size());
    lua_setglobal(state, name);
}

void pushJson(lua_State* state, const boost::json::value& value) {
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
            lua_pushnumber(state, value.as_double());
            break;
        case boost::json::kind::string: {
            const auto& text = value.as_string();
            lua_pushlstring(state, text.data(), text.size());
            break;
        }
        case boost::json::kind::object:
            lua_createtable(state, 0, static_cast<int>(value.as_object().size()));
            for (const auto& entry : value.as_object()) {
                const auto& key = entry.key();
                lua_pushlstring(state, key.data(), key.size());
                pushJson(state, entry.value());
                lua_rawset(state, -3);
            }
            break;
        case boost::json::kind::array: {
            const auto& array = value.as_array();
            lua_createtable(state, static_cast<int>(array.size()), 0);
            lua_Integer index = 1;
            for (const auto& item : array) {
                pushJson(state, item);
                lua_rawseti(state, -2, index++);
            }
            break;
        }
    }
}

void setObject(lua_State* state, const char* name, const boost::json::object& value) {
    pushJson(state, value);
    lua_setglobal(state, name);
}

std::string luaError(lua_State* state) {
    const char* message = lua_tostring(state, -1);
    return message ? message : "unknown Lua error";
}

}

std::string SPTemplate::render(const std::string& source,
                               const SPTemplateRequest& request,
                               const boost::json::object& model) {
    LuaState state(luaL_newstate(), lua_close);
    if (!state)
        throw std::runtime_error("Could not create Lua state");
    luaL_openlibs(state.get());

    setString(state.get(), "method", request.method);
    setString(state.get(), "uri", request.uri);
    setString(state.get(), "action", request.action);
    setObject(state.get(), "data", request.data);
    setObject(state.get(), "model", model);

    const std::string lua = compile(source);
    if (luaL_loadbuffer(state.get(), lua.data(), lua.size(), "spider-template") != 0)
        throw std::runtime_error(luaError(state.get()));
    if (lua_pcall(state.get(), 0, 1, 0) != 0)
        throw std::runtime_error(luaError(state.get()));

    size_t resultSize = 0;
    const char* result = lua_tolstring(state.get(), -1, &resultSize);
    if (!result)
        return {};
    return std::string(result, resultSize);
}

}
