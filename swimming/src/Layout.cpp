#include "swimming/Layout.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

#include <yoga/Yoga.h>

#include "swimming/Html.h"

namespace swimming {
    namespace {
        /// A box whose position is read back from its Yoga node once laid out.
        struct Built {
            YGNodeRef node;
            LayoutBox box;
            std::vector<Built> kids;
        };

        using Edge = YGEdge;
        const Edge kEdges[] = {YGEdgeTop, YGEdgeRight, YGEdgeBottom, YGEdgeLeft};
        const char* const kSides[] = {"top", "right", "bottom", "left"};

        std::optional<std::string> get(const Decls& d, const std::string& key) {
            const auto it = d.find(key);
            if (it == d.end()) return std::nullopt;
            return it->second;
        }

        std::optional<Len> getLen(const Decls& d, const std::string& key) {
            const auto value = get(d, key);
            return value ? parseLen(*value) : std::nullopt;
        }

        std::optional<float> getPx(const Decls& d, const std::string& key) {
            const auto len = getLen(d, key);
            if (len && len->kind == Len::Px) return len->value;
            return std::nullopt;
        }

        std::optional<float> getNumber(const Decls& d, const std::string& key) {
            const auto value = get(d, key);
            if (!value) return std::nullopt;
            char* end = nullptr;
            const float v = std::strtof(value->c_str(), &end);
            if (end == value->c_str()) return std::nullopt;
            return v;
        }

        /// No font stack yet: approximates an advance of 0.55em per character.
        std::pair<float, float> textSize(const std::string& text, float fontSize) {
            size_t lines = 1, longest = 0, current = 0;
            for (size_t i = 0; i < text.size(); ++i) {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                if (c == '\n') {
                    ++lines;
                    longest = std::max(longest, current);
                    current = 0;
                } else if ((c & 0xC0) != 0x80) { // count UTF-8 code points
                    ++current;
                }
            }
            longest = std::max(longest, current);
            return {static_cast<float>(longest) * fontSize * 0.55f, static_cast<float>(lines) * fontSize * 1.2f};
        }

        Decls computedDecls(const Node& n) {
            Decls d = parseDecls(uaStyle(n.kind));
            merge(d, parseDecls(n.style));
            return d;
        }

        bool isInline(const Node& n, const Decls& d) {
            if (n.kind == "#text") return true;
            const auto display = get(d, "display");
            if (display) return *display == "inline" || *display == "inline-block" || *display == "inline-flex";
            return defaultDisplay(n.kind) == DefaultDisplay::Inline;
        }

        std::optional<YGAlign> align(const std::string& s) {
            if (s == "flex-start" || s == "start") return YGAlignFlexStart;
            if (s == "flex-end" || s == "end") return YGAlignFlexEnd;
            if (s == "center") return YGAlignCenter;
            if (s == "stretch") return YGAlignStretch;
            if (s == "baseline") return YGAlignBaseline;
            return std::nullopt;
        }

        std::optional<YGJustify> justify(const std::string& s) {
            if (s == "flex-start" || s == "start") return YGJustifyFlexStart;
            if (s == "flex-end" || s == "end") return YGJustifyFlexEnd;
            if (s == "center") return YGJustifyCenter;
            if (s == "space-between") return YGJustifySpaceBetween;
            if (s == "space-around") return YGJustifySpaceAround;
            if (s == "space-evenly") return YGJustifySpaceEvenly;
            return std::nullopt;
        }

        YGFlexDirection direction(const std::optional<std::string>& s) {
            if (s == "row") return YGFlexDirectionRow;
            if (s == "row-reverse") return YGFlexDirectionRowReverse;
            if (s == "column-reverse") return YGFlexDirectionColumnReverse;
            return YGFlexDirectionColumn;
        }

        void setWidth(YGNodeRef n, const Len& l) {
            if (l.kind == Len::Px) YGNodeStyleSetWidth(n, l.value);
            else if (l.kind == Len::Pct) YGNodeStyleSetWidthPercent(n, l.value * 100);
            else YGNodeStyleSetWidthAuto(n);
        }
        void setHeight(YGNodeRef n, const Len& l) {
            if (l.kind == Len::Px) YGNodeStyleSetHeight(n, l.value);
            else if (l.kind == Len::Pct) YGNodeStyleSetHeightPercent(n, l.value * 100);
            else YGNodeStyleSetHeightAuto(n);
        }

        /// Box styles; auto min/max sizes and insets are Yoga's defaults, so they are left unset.
        void applyBox(YGNodeRef n, const Decls& d, float borderWidth) {
            setWidth(n, getLen(d, "width").value_or(Len{}));
            setHeight(n, getLen(d, "height").value_or(Len{}));
            if (auto l = getLen(d, "min-width"); l && l->kind != Len::Auto)
                l->kind == Len::Px ? YGNodeStyleSetMinWidth(n, l->value) : YGNodeStyleSetMinWidthPercent(n, l->value * 100);
            if (auto l = getLen(d, "min-height"); l && l->kind != Len::Auto)
                l->kind == Len::Px ? YGNodeStyleSetMinHeight(n, l->value) : YGNodeStyleSetMinHeightPercent(n, l->value * 100);
            if (auto l = getLen(d, "max-width"); l && l->kind != Len::Auto)
                l->kind == Len::Px ? YGNodeStyleSetMaxWidth(n, l->value) : YGNodeStyleSetMaxWidthPercent(n, l->value * 100);
            if (auto l = getLen(d, "max-height"); l && l->kind != Len::Auto)
                l->kind == Len::Px ? YGNodeStyleSetMaxHeight(n, l->value) : YGNodeStyleSetMaxHeightPercent(n, l->value * 100);

            for (int i = 0; i < 4; ++i) {
                const std::string side = kSides[i];
                const Edge edge = kEdges[i];
                const Len margin = getLen(d, "margin-" + side).value_or(Len::px(0));
                if (margin.kind == Len::Px) YGNodeStyleSetMargin(n, edge, margin.value);
                else if (margin.kind == Len::Pct) YGNodeStyleSetMarginPercent(n, edge, margin.value * 100);
                else YGNodeStyleSetMarginAuto(n, edge);

                const Len padding = getLen(d, "padding-" + side).value_or(Len::px(0));
                if (padding.kind == Len::Pct) YGNodeStyleSetPaddingPercent(n, edge, padding.value * 100);
                else YGNodeStyleSetPadding(n, edge, padding.kind == Len::Px ? padding.value : 0);

                const Len border = getLen(d, "border-" + side + "-width").value_or(Len::px(borderWidth));
                YGNodeStyleSetBorder(n, edge, border.kind == Len::Px ? border.value : 0);

                const auto inset = getLen(d, side);
                if (inset && inset->kind == Len::Px) YGNodeStyleSetPosition(n, edge, inset->value);
                else if (inset && inset->kind == Len::Pct) YGNodeStyleSetPositionPercent(n, edge, inset->value * 100);
            }
        }

        void applyFlex(YGNodeRef n, const Decls& d) {
            YGNodeStyleSetDisplay(n, YGDisplayFlex);
            YGNodeStyleSetFlexDirection(n, direction(get(d, "flex-direction")));
            YGNodeStyleSetFlexWrap(n, get(d, "flex-wrap") == "wrap" ? YGWrapWrap : YGWrapNoWrap);
            YGNodeStyleSetAlignContent(n, YGAlignStretch);
            const auto jc = get(d, "justify-content");
            YGNodeStyleSetJustifyContent(n, jc ? justify(*jc).value_or(YGJustifyFlexStart) : YGJustifyFlexStart);
            const auto ai = get(d, "align-items");
            YGNodeStyleSetAlignItems(n, ai ? align(*ai).value_or(YGAlignStretch) : YGAlignStretch);
            if (const auto as = get(d, "align-self"))
                if (const auto a = align(*as)) YGNodeStyleSetAlignSelf(n, *a);
            if (const auto gap = getPx(d, "gap")) YGNodeStyleSetGap(n, YGGutterAll, *gap);

            std::optional<float> grow = getNumber(d, "flex-grow");
            std::optional<float> shrink = getNumber(d, "flex-shrink");
            std::optional<Len> basis = getLen(d, "flex-basis");
            if (const auto flex = get(d, "flex")) {
                std::istringstream parts(*flex);
                std::vector<std::string> p{std::istream_iterator<std::string>(parts), {}};
                char* end = nullptr;
                if (!p.empty()) {
                    const float g = std::strtof(p[0].c_str(), &end);
                    if (end != p[0].c_str()) {
                        grow = g;
                        if (p.size() == 1) {
                            shrink = 1.0f;
                            basis = Len::pct(0);
                        }
                    }
                }
                if (p.size() > 1) {
                    const float s = std::strtof(p[1].c_str(), &end);
                    if (end != p[1].c_str()) shrink = s;
                }
                if (p.size() > 2)
                    if (const auto b = parseLen(p[2])) basis = *b;
            }
            YGNodeStyleSetFlexGrow(n, grow.value_or(0));
            YGNodeStyleSetFlexShrink(n, shrink.value_or(1));
            if (!basis || basis->kind == Len::Auto) YGNodeStyleSetFlexBasisAuto(n);
            else if (basis->kind == Len::Px) YGNodeStyleSetFlexBasis(n, basis->value);
            else YGNodeStyleSetFlexBasisPercent(n, basis->value * 100);

            YGNodeStyleSetPositionType(n, get(d, "position") == "absolute" ? YGPositionTypeAbsolute : YGPositionTypeRelative);
        }

        class Builder {
        public:
            Builder(const NodeMap& nodes, YGConfigRef config) : m_nodes(nodes), m_config(config) {}

            std::optional<Built> build(std::uint64_t id, const Node& v, const Computed& parent, const std::string& extraStyle = {}) {
                if (v.kind == "#text") {
                    const std::string text = v.text.value_or("");
                    const auto [w, h] = textSize(text, parent.fontSize);
                    YGNodeRef node = YGNodeNewWithConfig(m_config);
                    YGNodeStyleSetWidth(node, w);
                    YGNodeStyleSetHeight(node, h);
                    YGNodeStyleSetFlexShrink(node, 0);
                    Built built{node, {}, {}};
                    built.box.tag = "#text";
                    built.box.text = text;
                    built.box.computed = parent;
                    return built;
                }

                Decls d = computedDecls(v);
                if (!extraStyle.empty()) merge(d, parseDecls(extraStyle));
                const auto display = get(d, "display");
                const bool hidden = display ? *display == "none" : defaultDisplay(v.kind) == DefaultDisplay::None;
                if (hidden) return std::nullopt;

                Computed c;
                const auto color = get(d, "color");
                c.color = color ? parseColor(*color).value_or(parent.color) : parent.color;
                c.fontSize = getPx(d, "font-size").value_or(parent.fontSize);
                if (const auto weight = get(d, "font-weight"))
                    c.bold = *weight == "bold" || std::atoi(weight->c_str()) >= 600;
                else
                    c.bold = parent.bold;
                if (const auto bg = get(d, "background-color")) c.background = parseColor(*bg);
                auto borderColor = get(d, "border-color");
                if (!borderColor) borderColor = get(d, "border-top-color");
                c.borderColor = borderColor ? parseColor(*borderColor).value_or(c.color) : c.color;
                c.borderWidth = getPx(d, "border-top-width").value_or(0);
                c.radius = getPx(d, "border-radius").value_or(0);
                c.opacity = getNumber(d, "opacity").value_or(1);

                // Children: consecutive inline-level runs are grouped into anonymous wrapping rows.
                std::vector<Built> kids;
                std::vector<Built> run;
                const bool flexParent = display == "flex" || display == "inline-flex";
                for (std::uint64_t childId : v.children) {
                    const auto it = m_nodes.find(childId);
                    if (it == m_nodes.end()) continue;
                    const Node& child = it->second;
                    const bool inlineChild = !flexParent && isInline(child, child.kind == "#text" ? Decls{} : computedDecls(child));
                    auto built = build(childId, child, c);
                    if (!built) continue;
                    if (inlineChild) {
                        run.push_back(std::move(*built));
                    } else {
                        flush(run, kids, c);
                        kids.push_back(std::move(*built));
                    }
                }
                flush(run, kids, c);

                YGNodeRef node = YGNodeNewWithConfig(m_config);
                applyFlex(node, d);
                applyBox(node, d, c.borderWidth);
                attach(node, kids);
                Built built{node, {}, std::move(kids)};
                built.box.id = id;
                built.box.tag = v.kind;
                built.box.computed = c;
                return built;
            }

        private:
            void attach(YGNodeRef node, const std::vector<Built>& kids) {
                for (size_t i = 0; i < kids.size(); ++i)
                    YGNodeInsertChild(node, kids[i].node, i);
            }

            void flush(std::vector<Built>& run, std::vector<Built>& kids, const Computed& c) {
                if (run.empty()) return;
                YGNodeRef node = YGNodeNewWithConfig(m_config);
                YGNodeStyleSetFlexDirection(node, YGFlexDirectionRow);
                YGNodeStyleSetFlexWrap(node, YGWrapWrap);
                YGNodeStyleSetAlignItems(node, YGAlignFlexStart);
                YGNodeStyleSetAlignContent(node, YGAlignStretch);
                YGNodeStyleSetFlexShrink(node, 0);
                attach(node, run);
                Built anon{node, {}, std::move(run)};
                anon.box.tag = "#anon";
                anon.box.computed = c;
                kids.push_back(std::move(anon));
                run.clear();
            }

            const NodeMap& m_nodes;
            YGConfigRef m_config;
        };

        LayoutBox collect(Built& b) {
            LayoutBox box = std::move(b.box);
            box.x = YGNodeLayoutGetLeft(b.node);
            box.y = YGNodeLayoutGetTop(b.node);
            box.w = YGNodeLayoutGetWidth(b.node);
            box.h = YGNodeLayoutGetHeight(b.node);
            box.children.reserve(b.kids.size());
            for (Built& kid : b.kids)
                box.children.push_back(collect(kid));
            return box;
        }

        std::optional<std::uint64_t> hit(const LayoutBox& b, float x, float y, float ox, float oy) {
            const float bx = ox + b.x, by = oy + b.y;
            if (x < bx || y < by || x > bx + b.w || y > by + b.h) return std::nullopt;
            for (auto it = b.children.rbegin(); it != b.children.rend(); ++it)
                if (auto id = hit(*it, x, y, bx, by)) return id;
            return b.id;
        }

        nlohmann::json summarize(const LayoutBox& b) {
            nlohmann::json children = nlohmann::json::array();
            for (const LayoutBox& c : b.children) children.push_back(summarize(c));
            return {{"id", b.id ? nlohmann::json(*b.id) : nlohmann::json()}, {"type", b.tag},
                    {"x", b.x}, {"y", b.y}, {"width", b.w}, {"height", b.h}, {"children", children}};
        }
    }

    nlohmann::json LaidOut::summary() const {
        return summarize(root);
    }

    std::optional<std::uint64_t> LaidOut::hitTest(float x, float y) const {
        return hit(root, x, y, 0, 0);
    }

    LaidOut layout(const NodeMap& nodes, std::uint64_t root, float width, float height) {
        YGConfigRef config = YGConfigNew();
        Builder builder(nodes, config);
        const auto it = nodes.find(root);
        std::optional<Built> built;
        if (it != nodes.end()) {
            std::ostringstream size;
            size << "width:" << width << "px;height:" << height << "px";
            built = builder.build(root, it->second, Computed{}, size.str());
        }
        LaidOut out;
        if (built) {
            YGNodeCalculateLayout(built->node, width, height, YGDirectionLTR);
            YGNodeRef rootNode = built->node;
            out.root = collect(*built);
            YGNodeFreeRecursive(rootNode);
        }
        YGConfigFree(config);
        return out;
    }
}
