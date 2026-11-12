#ifndef SP_RICH_TEXT_H
#define SP_RICH_TEXT_H

#include "SPFontStyle.h"
#include "libspace.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace space {
    class SPColor;
    class SPGraphicsContext;
    class SPImage;

    /// One stretch of a label that is drawn the same way throughout.
    struct SPRichTextRun {
        enum class Kind { Text, Image, LineBreak };
        Kind kind = Kind::Text;
        std::string text;      ///< Text: what to draw, entities already decoded
        std::string src;       ///< Image: what SPGraphicsContext::loadImage() takes
        int width = 0;         ///< Image: 0 follows the height, or the line height
        int height = 0;        ///< Image: 0 follows the width, or the line height
        std::string uri;       ///< where a click goes, "" when the run is not a link
        bool bold = false;
        bool italic = false;
        bool underline = false;
    };

    /// Where a run ended up, relative to the label's top left corner.
    struct SPRichTextFragment {
        int run = 0;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    /**
     * A label written in a small subset of HTML:
     *
     *   <link uri="…">text</link>, <a href="…">text</a>   a clickable link
     *   <b>, <strong>, <i>, <em>, <u>                      bold, italic, underline
     *   <br />                                             a line break
     *   <img src="…" uri="…" width="…" height="…" />       an image; uri makes it a link
     *
     * and the entities &amp; &lt; &gt; &quot; &apos; &nbsp; &#N; &#xN;.
     * Anything else that looks like a tag is drawn as it is written. Lines do
     * not wrap; a line ends at <br /> or a newline.
     *
     * Text that comes from data has to go through escape() (or link()) before
     * it is put into markup, or a name such as "<b>" would be read as a tag.
     */
    class SPRichText {
    public:
        using Measure = std::function<sp_rectangle(const std::string&, const SPFontStyle&)>;

        static std::vector<SPRichTextRun> parse(const std::string& markup);
        /// text with & < > " ' replaced by entities, safe to put into markup.
        static std::string escape(const std::string& text);
        /// <link uri="uri">text</link>, with both escaped.
        static std::string link(const std::string& uri, const std::string& text);
        /// As link(), around content that already is markup.
        static std::string wrapLink(const std::string& uri, const std::string& markup);
        /// The label's text without its formatting; line breaks become newlines.
        static std::string plainText(const std::string& markup);

        SPRichText() = default;
        explicit SPRichText(const std::string& markup) { setMarkup(markup); }

        /// Parses markup. Does nothing when it is what the label already holds.
        void setMarkup(const std::string& markup);
        const std::string& getMarkup() const { return m_markup; }
        const std::vector<SPRichTextRun>& getRuns() const { return m_runs; }
        bool empty() const { return m_runs.empty(); }
        bool hasLinks() const { return m_hasLinks; }
        std::string getPlainText() const;

        /// Places the runs for font. The result is kept until the markup or
        /// the font changes, so this is cheap to call before every paint.
        void layout(const SPFontStyle& font, const Measure& measure);
        const std::vector<SPRichTextFragment>& getFragments() const { return m_fragments; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

        /// The link at a point relative to the label's top left corner, or "".
        /// Valid after layout().
        std::string uriAt(int x, int y) const;

        /// Lays the label out for font and draws it with its top left corner
        /// at (x, y). Links are drawn in linkColor; those to hoverUri, and
        /// <u> runs, are underlined.
        void paint(SPGraphicsContext* g, const SPFontStyle& font, int x, int y,
                   const SPColor& color, const SPColor& linkColor,
                   const std::string& hoverUri = std::string());

        /// The font a run is drawn in, given the label's font.
        static SPFontStyle fontFor(const SPRichTextRun& run, const SPFontStyle& base);

    private:
        std::string m_markup;
        std::vector<SPRichTextRun> m_runs;
        std::vector<SPRichTextFragment> m_fragments;
        bool m_hasLinks = false;
        int m_width = 0;
        int m_height = 0;
        bool m_laidOut = false;
        std::string m_layoutFont; ///< the font the fragments were placed for
        std::map<std::string, SPImage*> m_images;
    };
}

#endif
