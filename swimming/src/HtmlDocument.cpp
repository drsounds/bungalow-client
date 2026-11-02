#include "swimming/HtmlDocument.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>
#include <unordered_map>

namespace swimming {
    namespace {
        const std::set<std::string> kVoid = {
            "area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr",
        };
        const std::set<std::string> kRawText = {"script", "style", "textarea", "title", "xmp", "noscript"};

        std::string lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        void appendUtf8(std::string& out, unsigned long cp) {
            if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
            if (cp < 0x80) {
                out += static_cast<char>(cp);
            } else if (cp < 0x800) {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }

        /// Decodes numeric and the common named character references.
        std::string decodeEntities(const std::string& s) {
            static const std::unordered_map<std::string, unsigned long> named = {
                {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", 0xA0},
                {"copy", 0xA9}, {"reg", 0xAE}, {"hellip", 0x2026}, {"mdash", 0x2014}, {"ndash", 0x2013},
                {"laquo", 0xAB}, {"raquo", 0xBB}, {"middot", 0xB7}, {"times", 0xD7},
            };
            std::string out;
            out.reserve(s.size());
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] != '&') {
                    out += s[i];
                    continue;
                }
                const size_t semi = s.find(';', i + 1);
                if (semi == std::string::npos || semi - i > 32) {
                    out += '&';
                    continue;
                }
                const std::string ref = s.substr(i + 1, semi - i - 1);
                if (ref.size() > 1 && ref[0] == '#') {
                    const bool hex = ref[1] == 'x' || ref[1] == 'X';
                    const std::string digits = ref.substr(hex ? 2 : 1);
                    char* end = nullptr;
                    const unsigned long cp = std::strtoul(digits.c_str(), &end, hex ? 16 : 10);
                    if (!digits.empty() && *end == '\0') {
                        appendUtf8(out, cp);
                        i = semi;
                        continue;
                    }
                } else if (const auto it = named.find(ref); it != named.end()) {
                    appendUtf8(out, it->second);
                    i = semi;
                    continue;
                }
                out += '&';
            }
            return out;
        }

        class Parser {
        public:
            explicit Parser(const std::string& html) : m_s(html) {}

            std::unique_ptr<HtmlNode> parse() {
                auto document = std::make_unique<HtmlNode>();
                m_open.push_back(document.get());
                while (m_i < m_s.size()) {
                    if (m_s[m_i] == '<') tag();
                    else text();
                }
                return document;
            }

        private:
            HtmlNode& current() { return *m_open.back(); }

            void text() {
                const size_t next = m_s.find('<', m_i + 1);
                const size_t end = next == std::string::npos ? m_s.size() : next;
                addText(decodeEntities(m_s.substr(m_i, end - m_i)));
                m_i = end;
            }

            void addText(const std::string& text) {
                if (text.empty()) return;
                auto& children = current().children;
                if (!children.empty() && children.back()->isText()) {
                    children.back()->text += text;
                    return;
                }
                auto node = std::make_unique<HtmlNode>();
                node->text = text;
                children.push_back(std::move(node));
            }

            void skipTo(const std::string& terminator) {
                const size_t end = m_s.find(terminator, m_i);
                m_i = end == std::string::npos ? m_s.size() : end + terminator.size();
            }

            void tag() {
                if (m_s.compare(m_i, 4, "<!--") == 0) {
                    m_i += 4;
                    skipTo("-->");
                } else if (m_i + 1 < m_s.size() && (m_s[m_i + 1] == '!' || m_s[m_i + 1] == '?')) {
                    skipTo(">"); // doctype, CDATA-ish, processing instructions
                } else if (m_i + 1 < m_s.size() && m_s[m_i + 1] == '/') {
                    endTag();
                } else if (m_i + 1 < m_s.size() && std::isalpha(static_cast<unsigned char>(m_s[m_i + 1]))) {
                    startTag();
                } else {
                    addText("<");
                    ++m_i;
                }
            }

            std::string name() {
                const size_t begin = m_i;
                while (m_i < m_s.size() && !std::isspace(static_cast<unsigned char>(m_s[m_i])) && m_s[m_i] != '>' &&
                       m_s[m_i] != '/')
                    ++m_i;
                return lower(m_s.substr(begin, m_i - begin));
            }

            void skipSpace() {
                while (m_i < m_s.size() && std::isspace(static_cast<unsigned char>(m_s[m_i]))) ++m_i;
            }

            void endTag() {
                m_i += 2;
                const std::string tagName = name();
                skipTo(">");
                for (size_t depth = m_open.size(); depth > 1; --depth) {
                    if (m_open[depth - 1]->name == tagName) {
                        m_open.resize(depth - 1);
                        return;
                    }
                }
            }

            void startTag() {
                ++m_i;
                auto node = std::make_unique<HtmlNode>();
                node->name = name();
                bool selfClosing = false;
                while (m_i < m_s.size()) {
                    skipSpace();
                    if (m_i >= m_s.size()) break;
                    if (m_s[m_i] == '>') {
                        ++m_i;
                        break;
                    }
                    if (m_s[m_i] == '/') {
                        selfClosing = true;
                        ++m_i;
                        continue;
                    }
                    attribute(*node);
                }

                HtmlNode* element = node.get();
                current().children.push_back(std::move(node));
                if (kRawText.count(element->name)) {
                    const std::string close = "</" + element->name;
                    size_t end = m_i;
                    while ((end = m_s.find("</", end)) != std::string::npos &&
                           lower(m_s.substr(end, close.size())) != close)
                        end += 2;
                    if (end == std::string::npos) end = m_s.size();
                    std::string raw = m_s.substr(m_i, end - m_i);
                    element->text = element->name == "script" || element->name == "style" ? raw : decodeEntities(raw);
                    m_i = end;
                    if (m_i < m_s.size()) skipTo(">");
                } else if (!selfClosing && !kVoid.count(element->name)) {
                    m_open.push_back(element);
                }
            }

            void attribute(HtmlNode& node) {
                const size_t begin = m_i;
                while (m_i < m_s.size() && !std::isspace(static_cast<unsigned char>(m_s[m_i])) && m_s[m_i] != '=' &&
                       m_s[m_i] != '>' && !(m_s[m_i] == '/' && m_i + 1 < m_s.size() && m_s[m_i + 1] == '>'))
                    ++m_i;
                std::string key = lower(m_s.substr(begin, m_i - begin));
                if (key.empty()) {
                    ++m_i; // a stray character; skip it
                    return;
                }
                skipSpace();
                std::string value;
                if (m_i < m_s.size() && m_s[m_i] == '=') {
                    ++m_i;
                    skipSpace();
                    if (m_i < m_s.size() && (m_s[m_i] == '"' || m_s[m_i] == '\'')) {
                        const char quote = m_s[m_i++];
                        const size_t end = m_s.find(quote, m_i);
                        const size_t stop = end == std::string::npos ? m_s.size() : end;
                        value = m_s.substr(m_i, stop - m_i);
                        m_i = end == std::string::npos ? m_s.size() : end + 1;
                    } else {
                        const size_t start = m_i;
                        while (m_i < m_s.size() && !std::isspace(static_cast<unsigned char>(m_s[m_i])) && m_s[m_i] != '>')
                            ++m_i;
                        value = m_s.substr(start, m_i - start);
                    }
                }
                for (const auto& existing : node.attributes)
                    if (existing.first == key) return; // the first occurrence wins
                node.attributes.emplace_back(std::move(key), decodeEntities(value));
            }

            const std::string& m_s;
            size_t m_i = 0;
            std::vector<HtmlNode*> m_open;
        };
    }

    std::optional<std::string> HtmlNode::attribute(const std::string& key) const {
        for (const auto& [k, v] : attributes)
            if (k == key) return v;
        return std::nullopt;
    }

    std::string HtmlNode::textContent() const {
        std::string out = text;
        for (const auto& child : children)
            out += child->textContent();
        return out;
    }

    std::unique_ptr<HtmlNode> parseHtml(const std::string& html) {
        return Parser(html).parse();
    }
}
