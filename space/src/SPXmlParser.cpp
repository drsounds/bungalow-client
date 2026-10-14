#include "SPXmlParser.h"

#include <expat.h>

#include <limits>
#include <memory>
#include <type_traits>

namespace space {
    namespace {
        struct ParseContext {
            SPXmlNode root;
            std::vector<SPXmlNode*> stack;
            XML_Parser parser = nullptr;
        };

        void startElement(void* userData, const XML_Char* name,
                          const XML_Char** attributes)
        {
            auto& context = *static_cast<ParseContext*>(userData);
            SPXmlNode node;
            node.name = name;
            for (const XML_Char** attribute = attributes;
                 *attribute != nullptr; attribute += 2)
                node.attributes.emplace(attribute[0], attribute[1]);

            SPXmlNode* added = nullptr;
            if (context.stack.empty()) {
                context.root = std::move(node);
                added = &context.root;
            } else {
                context.stack.back()->children.push_back(std::move(node));
                added = &context.stack.back()->children.back();
            }
            context.stack.push_back(added);
        }

        void endElement(void* userData, const XML_Char*)
        {
            auto& stack = static_cast<ParseContext*>(userData)->stack;
            if (!stack.empty())
                stack.pop_back();
        }

        void characterData(void* userData, const XML_Char* text, int length)
        {
            auto& stack = static_cast<ParseContext*>(userData)->stack;
            if (stack.empty() || length == 0)
                return;

            auto* parent = stack.back();
            parent->text.append(text, static_cast<std::size_t>(length));
            if (parent->children.empty() || !parent->children.back().textNode) {
                SPXmlNode textNode;
                textNode.textNode = true;
                parent->children.push_back(std::move(textNode));
            }
            parent->children.back().text.append(text, static_cast<std::size_t>(length));
        }

        void startDoctype(void* userData, const XML_Char*, const XML_Char*,
                          const XML_Char*, int)
        {
            auto& context = *static_cast<ParseContext*>(userData);
            XML_StopParser(context.parser, XML_FALSE);
        }
    }

    bool parseSPXml(const std::string& xml, SPXmlNode& root,
                    std::string* error)
    {
        struct ParserDeleter {
            void operator()(XML_Parser parser) const
            {
                if (parser)
                    XML_ParserFree(parser);
            }
        };

        ParseContext context;
        std::unique_ptr<std::remove_pointer_t<XML_Parser>, ParserDeleter> parser(
            XML_ParserCreate(nullptr));
        if (!parser) {
            if (error)
                *error = "Could not allocate XML parser";
            return false;
        }

        context.parser = parser.get();
        XML_SetUserData(parser.get(), &context);
        XML_SetElementHandler(parser.get(), startElement, endElement);
        XML_SetCharacterDataHandler(parser.get(), characterData);
        XML_SetStartDoctypeDeclHandler(parser.get(), startDoctype);
        XML_SetParamEntityParsing(parser.get(), XML_PARAM_ENTITY_PARSING_NEVER);

        if (xml.size() > static_cast<std::size_t>(
                             (std::numeric_limits<int>::max)())) {
            if (error)
                *error = "XML document is too large";
            return false;
        }
        const auto status = XML_Parse(parser.get(), xml.data(),
                                      static_cast<int>(xml.size()), XML_TRUE);
        if (status != XML_STATUS_OK || context.stack.size() != 0 ||
            context.root.name.empty()) {
            if (error) {
                const XML_Error code = XML_GetErrorCode(parser.get());
                *error = code == XML_ERROR_NONE
                    ? "XML document has no root element"
                    : XML_ErrorString(code);
            }
            return false;
        }

        root = std::move(context.root);
        if (error)
            error->clear();
        return true;
    }
}
