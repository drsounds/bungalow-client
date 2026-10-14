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
	boost::json::object data;
	data["name"] = "Spacify";
	boost::json::object model;
	model["title"] = "Overview";

	const std::string source = "<view>\n"
							   "  <page title=\"${model.title}\">\n"
							   "    <text>${string.upper(data.name)}</text>\n"
							   "% for i,3 do\n"
							   "    <text>${i}</text>\n"
							   "% end\n"
							   "    <text>${method}:${uri}:${action}</text>\n"
							   "% if action == \"refresh\" then\n"
							   "    <text>refreshed</text>\n"
							   "% end\n"
							   "</view>";

	const std::string rendered = space::SPTemplate::render(
		source, "POST", "spacify:overview", "refresh", data, model);
	if (!contains(rendered, "<page title=\"Overview\">") ||
		!contains(rendered, "<text>SPACIFY</text>") ||
		!contains(rendered, "<text>1</text>") ||
		!contains(rendered, "<text>2</text>") ||
		!contains(rendered, "<text>3</text>") ||
		!contains(rendered, "<text>POST:spacify:overview:refresh</text>") ||
		!contains(rendered, "<text>refreshed</text>"))
		return 1;

	const std::string escaped =
		space::SPTemplate::render("<view><text>\\\"quoted\\\"</text></view>");
	if (escaped != "<view><text>\\\"quoted\\\"</text></view>") {
		std::cerr << "Literal backslashes and quotes were not preserved\n";
		return 1;
	}
	return 0;
}
