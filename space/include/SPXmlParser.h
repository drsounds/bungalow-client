#ifndef SP_XML_PARSER_H
#define SP_XML_PARSER_H

#include <map>
#include <string>
#include <vector>

namespace space {
    struct SPXmlNode {
        std::string name;
        std::string text;
        std::map<std::string, std::string> attributes;
        std::vector<SPXmlNode> children;
        bool textNode = false;
    };

    bool parseSPXml(const std::string& xml, SPXmlNode& root,
                    std::string* error = nullptr);
}

#endif
