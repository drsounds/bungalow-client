#include "SPRichText.h"

#include "SPColor.h"
#include "SPGraphicsContext.h"

namespace space {

void SPRichText::paint(SPGraphicsContext* g, const SPFontStyle& font, int x, int y,
                       const SPColor& color, const SPColor& linkColor,
                       const std::string& hoverUri) {
    if (g == nullptr || m_runs.empty())
        return;
    this->layout(font, [g](const std::string& text, const SPFontStyle& style) {
        return g->measureString(text, style);
    });

    for (const auto& fragment : m_fragments) {
        const SPRichTextRun& run = m_runs[static_cast<size_t>(fragment.run)];
        const int left = x + fragment.x;
        const int top = y + fragment.y;
        const bool link = !run.uri.empty();
        const bool hovered = link && run.uri == hoverUri;

        if (run.kind == SPRichTextRun::Kind::Image) {
            auto cached = m_images.find(run.src);
            if (cached == m_images.end())
                cached = m_images.emplace(run.src, run.src.empty() ? nullptr : g->loadImage(run.src)).first;
            if (cached->second != nullptr)
                g->drawImage(cached->second, left, top, fragment.width, fragment.height);
            if (hovered)
                g->drawRectangle(left, top, fragment.width, fragment.height, linkColor);
            continue;
        }

        const SPColor& textColor = link ? linkColor : color;
        g->drawString(run.text, fontFor(run, font), textColor, left, top, fragment.width, fragment.height);
        if (run.underline || hovered)
            g->fillRectangle(left, top + fragment.height - 2, fragment.width, 1, textColor);
    }
}

} // namespace space
