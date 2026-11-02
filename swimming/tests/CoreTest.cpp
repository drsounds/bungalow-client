#include "swimming/Engine.h"
#include "swimming/HtmlDocument.h"
#include "swimming/Render.h"
#include "swimming/Style.h"

#include <iostream>
#include <string>
#include <vector>

using namespace swimming;

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        ++failures;
    }
}

std::uint64_t div(Engine& e, const std::string& style) {
    return e.createNode("div", style, "{}");
}

void laysOutCssWithYoga() {
    Engine e;
    const auto row = div(e, "display:flex;flex-direction:row;padding:10px");
    const auto a = div(e, "width:50px;height:20px");
    const auto b = div(e, "width:30px;height:20px;margin-left:5px");
    e.appendChild(row, a);
    e.appendChild(row, b);
    e.appendChild(kRoot, row);
    e.computeLayout(200, 100);
    const LayoutBox& r = e.layout()->root.children[0];
    check(r.children[0].x == 10 && r.children[0].y == 10 && r.children[0].w == 50 && r.children[0].h == 20,
          "padding offsets the first flex item");
    check(r.children[1].x == 65, "margin-left offsets the second flex item");
}

void everyHtmlElementRendersAndHeadIsHidden() {
    Engine e;
    const auto main = e.createNode("main", "", "{}");
    const auto h1 = e.createNode("h1", "", "{}");
    e.appendChild(h1, e.createText("Hi"));
    e.appendChild(main, h1);
    for (const char* tag : {"script", "table", "video"})
        e.appendChild(main, e.createNode(tag, "", "{}"));
    e.appendChild(kRoot, main);
    e.computeLayout(100, 100);
    const auto& kids = e.layout()->root.children[0].children;
    std::vector<std::string> tags;
    for (const auto& k : kids) tags.push_back(k.tag);
    check(tags == std::vector<std::string>{"h1", "table", "#anon"}, "script is hidden, video is inline");
    check(!kids.empty() && kids[0].h > 30, "h1 has its UA font size");
}

void insertBeforeRemoveAndHitTest() {
    Engine e;
    const auto a = div(e, "width:50px;height:20px");
    const auto b = div(e, "width:50px;height:20px");
    e.appendChild(kRoot, b);
    e.insertBefore(kRoot, a, b);
    e.computeLayout(200, 100);
    check(e.hitTest(5, 5) == a, "hit test finds the inserted node");
    check(e.hitTest(5, 25) == b, "hit test finds the node after it");
    check(!e.hitTest(150, 90), "the root is never hit");
    const size_t before = e.nodeCount();
    const auto inner = div(e, "");
    e.appendChild(a, inner);
    e.removeChild(kRoot, a);
    check(e.nodeCount() == before - 1, "removing a node frees its subtree");
    check(e.parent(b) == kRoot, "siblings keep their parent");
}

void parsesInlineCss() {
    const Decls d = parseDecls("Width: 10px; color: red !important; ; bogus; background: url(a;b)");
    check(d.at("width") == "10px" && d.at("color") == "red", "names are lowercased and !important dropped");
    check(d.at("background") == "url(a;b)", "semicolons inside parentheses do not split");

    check(parseColor("red") == Rgba{255, 0, 0, 255}, "named colour");
    check(parseColor("#0284c7") == Rgba{2, 132, 199, 255}, "#rrggbb");
    check(parseColor("#f008") == Rgba{255, 0, 0, 136}, "#rgba");
    check(parseColor("rgba(10, 20, 30, 0.5)") == Rgba{10, 20, 30, 128}, "rgba()");
    check(parseColor("rgb(10 20 30 / 50%)") == Rgba{10, 20, 30, 128}, "space-separated rgb()");
    check(parseColor("hsl(120, 100%, 50%)") == Rgba{0, 255, 0, 255}, "hsl()");
    check(!parseColor("currentcolor"), "currentcolor has no fixed value");

    check(parseLen("2rem") == Len::px(32) && parseLen("1.5em") == Len::px(24), "em and rem use a 16px font");
    check(parseLen("50%") == Len::pct(0.5f) && parseLen("auto") == Len{} && parseLen("0") == Len::px(0), "lengths");

    Decls merged;
    merge(merged, parseDecls("margin:16px 40px;border-width:2px"));
    check(merged.at("margin-left") == "40px" && merged.at("margin-top") == "16px", "margin shorthand expands");
    check(merged.at("border-bottom-width") == "2px", "border-width shorthand expands");
}

void paintsBackgroundsAndBorders() {
    Engine e;
    const auto box = div(e, "width:40px;height:20px;background-color:#ff0000;border-width:2px;border-color:blue");
    e.appendChild(kRoot, box);
    e.computeLayout(100, 50);
    const Pixmap p = paint(*e.layout(), 100, 50);
    check(p.pixel(20, 10) == Rgba{255, 0, 0, 255}, "background fills the box");
    check(p.pixel(0, 10)[2] > 200 && p.pixel(0, 10)[0] < 100, "the border is stroked on the edge");
    check(p.pixel(80, 40) == Rgba{255, 255, 255, 255}, "the page is white");
}

void parsesHtml() {
    const auto doc = parseHtml(
        "<!doctype html><html><head><title>T &amp; t</title><script>if (a < b) x();</script></head>"
        "<body><!-- note --><main style=\"width:1px\" hidden><p>one<br>two &#x263A;</p><img src=x /></main></body></html>");
    const HtmlNode* html = doc->children[0].get();
    check(html->name == "html" && html->children.size() == 2, "html holds head and body");
    const HtmlNode& head = *html->children[0];
    check(head.children[0]->text == "T & t", "title text is decoded");
    check(head.children[1]->text == "if (a < b) x();", "script text is raw");
    const HtmlNode& main = *html->children[1]->children[0];
    check(main.name == "main" && main.attribute("style") == "width:1px" && main.attribute("hidden") == "", "attributes");
    check(main.children.size() == 2 && main.children[1]->name == "img", "void elements take no children");
    check(main.children[0]->textContent() == "onetwo \xE2\x98\xBA", "text content with a numeric reference");
}

} // namespace

int main() {
    laysOutCssWithYoga();
    everyHtmlElementRendersAndHeadIsHidden();
    insertBeforeRemoveAndHitTest();
    parsesInlineCss();
    paintsBackgroundsAndBorders();
    parsesHtml();
    return failures == 0 ? 0 : 1;
}
