#include "SPTemplate.h"

#include <iostream>
#include <string>

static bool contains(const std::string &text, const std::string &value) {
	if (text.find(value) != std::string::npos)
		return true;
	std::cerr << "Rendered template is missing: " << value << '\n';
	return false;
}

int main() {
	const std::string source = R"(
<view>
  <page title="${model.title}">
    <text>${method} ${uri} ${action} ${data.name}</text>
    <text>${data.missing}</text>
    <text>${data.items[1]}</text>
    <text>${data.items[2]}</text>
    % for i,3 do
    <text>${i}</text>
    % end
    % if data.enabled then
    <text>${model.nested.label}</text>
    % end
    % if action == "refresh" then
    <text>refreshed</text>
    % end
    <text>${string.upper(data.name)}</text>
  </page>
</view>)";

	const nlohmann::json data = {
		{"name", "Lua"}, {"enabled", true}, {"items", {1, "two"}}};
	const nlohmann::json model = {{"title", "Overview"},
								  {"nested", {{"label", "nested value"}}}};

	const auto rendered = space::SPTemplate::render(
		source, "POST", "spacify:start", "refresh", data, model);
	if (!contains(rendered, "title=\"Overview\"") ||
		!contains(rendered, "POST spacify:start refresh Lua") ||
		!contains(rendered, "<text>1</text>") ||
		!contains(rendered, "<text>two</text>") ||
		!contains(rendered, "1</text>") || !contains(rendered, "2</text>") ||
		!contains(rendered, "3</text>") ||
		!contains(rendered, "nested value") ||
		!contains(rendered, "<text>refreshed</text>") ||
		!contains(rendered, "<text>LUA</text>") ||
		rendered.find("nil") != std::string::npos) {
		return 1;
	}

	const std::string escaped =
		space::SPTemplate::render("<view><text>\\\"quoted\\\"</text></view>");
	if (escaped != "<view><text>\\\"quoted\\\"</text></view>") {
		std::cerr << "Literal backslashes and quotes were not preserved\n";
		return 1;
	}

	const std::string entity = space::SPTemplate::render(
		"<text>${escape(model.entity.name)}${escape(model.missing)}</text>", "GET",
		"spacify:artist:1", "", nlohmann::json::object(),
		{{"entity", {{"name", "Simon & <Garfunkel>"}}}});
	if (entity != "<text>Simon &amp; &lt;Garfunkel&gt;</text>") {
		std::cerr << "escape() did not produce safe XUL: " << entity << '\n';
		return 1;
	}
	return 0;
}
