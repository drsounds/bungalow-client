#include "swimming/Loader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>

#include "swimming/HtmlDocument.h"

namespace swimming {
    namespace fs = std::filesystem;

    namespace {
        std::string lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        bool startsWith(const std::string& s, const std::string& prefix) {
            return s.compare(0, prefix.size(), prefix) == 0;
        }

        bool isHttpUrl(const std::string& source) {
            const std::string s = lower(source);
            return startsWith(s, "http://") || startsWith(s, "https://");
        }

        bool hasScheme(const std::string& s) {
            if (s.empty() || !std::isalpha(static_cast<unsigned char>(s[0]))) return false;
            for (char c : s) {
                if (c == ':') return true;
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '+' && c != '-' && c != '.') return false;
            }
            return false;
        }

        std::string stripQuery(const std::string& s) {
            return s.substr(0, s.find_first_of("?#"));
        }

        std::string readFile(const fs::path& path) {
            std::ifstream in(path, std::ios::binary);
            if (!in) throw std::runtime_error("cannot read " + path.u8string());
            std::ostringstream content;
            content << in.rdbuf();
            return content.str();
        }

        bool looksLikeHtml(const std::string& content) {
            const size_t start = content.find_first_not_of(" \t\r\n\xEF\xBB\xBF");
            if (start == std::string::npos) return false;
            const std::string head = lower(content.substr(start, 14));
            return startsWith(head, "<!doctype html") || startsWith(head, "<html");
        }

        bool hasHtmlExtension(const std::string& path) {
            const std::string p = lower(stripQuery(path));
            auto ends = [&](const std::string& ext) { return p.size() >= ext.size() && p.compare(p.size() - ext.size(), ext.size(), ext) == 0; };
            return ends(".html") || ends(".htm");
        }

        /// Removes "." and ".." segments from an absolute URL path.
        std::string removeDotSegments(const std::string& path) {
            std::vector<std::string> out;
            std::string segment;
            std::istringstream parts(path);
            std::vector<std::string> segments;
            while (std::getline(parts, segment, '/')) segments.push_back(segment);
            if (!path.empty() && path.back() == '/') segments.emplace_back();
            for (size_t i = 1; i < segments.size(); ++i) {
                const std::string& s = segments[i];
                const bool last = i + 1 == segments.size();
                if (s == "..") {
                    if (!out.empty()) out.pop_back();
                    if (last) out.emplace_back();
                } else if (s == ".") {
                    if (last) out.emplace_back();
                } else {
                    out.push_back(s);
                }
            }
            std::string result;
            for (const std::string& s : out) result += "/" + s;
            return result.empty() ? "/" : result;
        }

        /// The <script> elements a page runs, in document order.
        void collectScripts(const HtmlNode& node, std::vector<const HtmlNode*>& out) {
            for (const auto& child : node.children) {
                if (child->name == "script") out.push_back(child.get());
                collectScripts(*child, out);
            }
        }

        const HtmlNode* findElement(const HtmlNode& node, const std::string& name) {
            for (const auto& child : node.children) {
                if (child->name == name) return child.get();
                if (const HtmlNode* found = findElement(*child, name)) return found;
            }
            return nullptr;
        }

        void appendHtmlNode(Engine& engine, const HtmlNode& node, std::uint64_t parent) {
            for (const auto& child : node.children) {
                if (child->isText()) {
                    if (child->text.find_first_not_of(" \t\r\n") != std::string::npos)
                        engine.appendChild(parent, engine.createText(child->text));
                    continue;
                }
                if (child->name == "script" || child->name == "style" || child->name == "head") continue;
                nlohmann::json props = nlohmann::json::object();
                for (const auto& [key, value] : child->attributes) props[key] = value;
                const std::string propsJson = props.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
                const std::uint64_t id = engine.createNode(child->name, child->attribute("style").value_or(""), propsJson);
                engine.appendChild(parent, id);
                appendHtmlNode(engine, *child, id);
            }
        }

        /// Adds the page's body to the root of the layout tree.
        void mountHtml(Engine& engine, const HtmlNode& document) {
            const HtmlNode* body = findElement(document, "body");
            if (!body) body = findElement(document, "html");
            appendHtmlNode(engine, body ? *body : document, kRoot);
            engine.computeLayout(engine.width(), engine.height());
        }

        bool isClassicScript(const HtmlNode& script) {
            const auto type = script.attribute("type");
            if (!type) return true;
            const std::string t = lower(*type);
            const size_t begin = t.find_first_not_of(" \t");
            const std::string kind = begin == std::string::npos ? "" : t.substr(begin, t.find_last_not_of(" \t") - begin + 1);
            return kind.empty() || kind == "text/javascript" || kind == "application/javascript" ||
                   kind == "text/ecmascript" || kind == "application/ecmascript";
        }

        /// Runs the page's classic scripts; external ones come from baseUrl (with fetch) or basePath.
        void runScripts(Bridge& bridge, const HtmlNode& document, const std::optional<std::string>& baseUrl,
                        const fs::path& basePath, const Fetch& fetch) {
            std::vector<const HtmlNode*> scripts;
            collectScripts(document, scripts);
            for (const HtmlNode* script : scripts) {
                if (!isClassicScript(*script)) continue;
                if (const auto src = script->attribute("src")) {
                    if (baseUrl) {
                        const std::string url = resolveUrl(*baseUrl, *src);
                        bridge.eval(fetch(url), url);
                    } else {
                        std::string relative = stripQuery(*src);
                        fs::path path = fs::u8path(relative);
                        if (!relative.empty() && (relative[0] == '/' || relative[0] == '\\'))
                            path = basePath / fs::u8path(relative.substr(relative.find_first_not_of("/\\")));
                        else if (!path.is_absolute())
                            path = basePath / path;
                        bridge.eval(readFile(path), path.u8string());
                    }
                } else if (script->text.find_first_not_of(" \t\r\n") != std::string::npos) {
                    bridge.eval(script->text, "<inline script>");
                }
            }
        }

        void runContent(Bridge& bridge, const std::string& content, bool html, const std::string& name,
                        const std::optional<std::string>& baseUrl, const fs::path& basePath, const Fetch& fetch) {
            if (!html) {
                bridge.eval(content, name);
                return;
            }
            const auto document = parseHtml(content);
            mountHtml(bridge.engine(), *document);
            runScripts(bridge, *document, baseUrl, basePath, fetch);
        }
    }

    std::string resolveUrl(const std::string& base, const std::string& reference) {
        if (hasScheme(reference)) return reference;
        const size_t schemeEnd = base.find("://");
        if (schemeEnd == std::string::npos) return reference;
        const std::string scheme = base.substr(0, schemeEnd);
        const size_t authorityEnd = base.find_first_of("/?#", schemeEnd + 3);
        const std::string origin = base.substr(0, authorityEnd);
        if (startsWith(reference, "//")) return scheme + ":" + reference;
        const std::string rest = authorityEnd == std::string::npos ? "/" : base.substr(authorityEnd);
        const std::string beforeHash = rest.substr(0, rest.find('#'));
        const std::string path = beforeHash.substr(0, beforeHash.find('?'));
        const std::string query = beforeHash.substr(path.size());
        if (reference.empty()) return origin + (path.empty() ? "/" : path) + query;
        if (reference[0] == '#') return origin + (path.empty() ? "/" : path) + query + reference;
        if (reference[0] == '?') return origin + (path.empty() ? "/" : path) + reference;

        const size_t refPathEnd = reference.find_first_of("?#");
        const std::string refPath = reference.substr(0, refPathEnd);
        const std::string refRest = refPathEnd == std::string::npos ? "" : reference.substr(refPathEnd);
        const std::string merged = refPath[0] == '/' ? refPath : path.substr(0, path.rfind('/') + 1) + refPath;
        return origin + removeDotSegments(merged.empty() || merged[0] != '/' ? "/" + merged : merged) + refRest;
    }

    void runSource(Bridge& bridge, const std::string& source, const Fetch& fetch) {
        if (isHttpUrl(source)) {
            if (!fetch) throw std::runtime_error("cannot load " + source + ": HTTP is not available");
            const std::string content = fetch(source);
            runContent(bridge, content, hasHtmlExtension(source) || looksLikeHtml(content), source, source, {}, fetch);
            return;
        }
        const fs::path path = fs::u8path(source);
        const std::string content = readFile(path);
        runContent(bridge, content, hasHtmlExtension(source), path.u8string(), std::nullopt, path.parent_path(), fetch);
    }
}
