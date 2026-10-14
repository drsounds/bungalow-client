#include "SPTemplate.h"

#include <cassert>
#include <string>

int main() {
    const std::string source = R"(
<view>
  <page title="${model.title}">
    <text>${method} ${uri} ${action} ${data.name}</text>
    <text>${data.missing}</text>
    % for i,3 do
    <text>${i}</text>
    % end
    % if data.enabled then
    <text>${model.nested.label}</text>
    % end
  </page>
</view>)";
    const boost::json::object data = {
        {"name", "Lua"},
        {"enabled", true}
    };
    const boost::json::object model = {
        {"title", "Overview"},
        {"nested", {{"label", "nested value"}}}
    };

    const auto markup = space::SPTemplate::render(
        source, "POST", "spacify:start", "refresh", data, model);

    assert(markup.find("title=\"Overview\"") != std::string::npos);
    assert(markup.find("POST spacify:start refresh Lua") != std::string::npos);
    assert(markup.find("1</text>") != std::string::npos);
    assert(markup.find("2</text>") != std::string::npos);
    assert(markup.find("3</text>") != std::string::npos);
    assert(markup.find("nested value") != std::string::npos);
    assert(markup.find("nil") == std::string::npos);
}
