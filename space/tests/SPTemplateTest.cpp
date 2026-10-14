#include "SPTemplate.h"

#include <stdexcept>

int main() {
    space::SPTemplateRequest request;
    request.method = "POST";
    request.uri = "spacify:overview";
    request.action = "refresh";
    request.data = boost::json::parse(R"({"name":"Ada"})").as_object();

    const boost::json::object model =
        boost::json::parse(R"({"title":"Overview","nested":{"value":42}})").as_object();
    const std::string output = space::SPTemplate::render(
        "<view>\n"
        "  <page title=\"${model.title}\">${method}:${uri}:${action}:${data.name}:${model.nested.value}</page>\n"
        "  % for i,3 do\n"
        "  <text>${i}</text>\n"
        "  % end\n"
        "</view>",
        request,
        model);

    if (output.find("<page title=\"Overview\">POST:spacify:overview:refresh:Ada:42</page>") ==
            std::string::npos ||
        output.find("<text>1</text>") == std::string::npos ||
        output.find("<text>2</text>") == std::string::npos ||
        output.find("<text>3</text>") == std::string::npos)
        return 1;

    if (space::SPTemplate::render("${string.upper('lua')}", request) != "LUA\n")
        return 2;

    try {
        space::SPTemplate::render("${missing.function()}");
    } catch (const std::runtime_error&) {
        return 0;
    }
    return 3;
}
