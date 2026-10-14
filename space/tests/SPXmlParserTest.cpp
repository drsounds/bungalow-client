#include "SPXmlParser.h"

#include <cassert>
#include <string>

int main()
{
    space::SPXmlNode root;
    std::string error;
    assert(space::parseSPXml(
        "<view id='main'><text>hello &amp; world</text><panel active='true'/></view>",
        root, &error));
    assert(error.empty());
    assert(root.name == "view");
    assert(root.attributes.at("id") == "main");
    assert(root.children.size() == 2);
    assert(root.children[0].name == "text");
    assert(root.children[0].text == "hello & world");
    assert(root.children[0].children.size() == 1);
    assert(root.children[0].children[0].textNode);
    assert(root.children[0].children[0].text == "hello & world");
    assert(root.children[1].attributes.at("active") == "true");

    assert(!space::parseSPXml("<view><broken></view>", root, &error));
    assert(!error.empty());
    assert(!space::parseSPXml("<!DOCTYPE view [<!ENTITY x 'x'>]><view>&x;</view>",
                              root, &error));
}
