#include "SPRichText.h"

#include <iostream>
#include <string>

using space::SPRichText;
using space::SPRichTextRun;

static int failures = 0;

static void check(bool condition, const std::string &what) {
	if (condition)
		return;
	std::cerr << "FAILED: " << what << '\n';
	++failures;
}

// Every character is 10 wide and a line is 16 high; bold is 12 wide.
static space::sp_rectangle measure(const std::string &text, const space::SPFontStyle &font) {
	const int advance = font.weight >= 700 ? 12 : 10;
	return {0, 0, static_cast<int>(text.size()) * advance, 16, 0};
}

int main() {
	const space::SPFontStyle font("Tahoma", 13, 1, false, false);

	// Formatting and links.
	{
		const auto runs = SPRichText::parse(
			"By <link uri=\"bungalow:artist:1\">Queen</link>, <a href='https://example.com/?a=1&amp;b=2'>"
			"<b>site</b></a> <i>it</i><u>un</u>");
		check(runs.size() == 7, "run count");
		if (runs.size() == 7) {
			check(runs[0].text == "By " && runs[0].uri.empty(), "plain text run");
			check(runs[1].text == "Queen" && runs[1].uri == "bungalow:artist:1", "link run");
			check(runs[2].text == ", " && runs[2].uri.empty(), "text after a link is not a link");
			check(runs[3].text == "site" && runs[3].bold &&
					  runs[3].uri == "https://example.com/?a=1&b=2",
				  "bold run inside <a href>, entity in the attribute");
			check(runs[4].text == " " && !runs[4].bold, "bold ends");
			check(runs[5].text == "it" && runs[5].italic && !runs[5].underline, "italic run");
			check(runs[6].text == "un" && runs[6].underline && !runs[6].italic, "underline run");
		}
	}

	// Line breaks, images, entities, and things that are not tags.
	{
		const auto runs = SPRichText::parse(
			"a<br />b<br>c\nd <img src=\"cover.png\" uri=\"bungalow:release:9\" width=\"32\" /> "
			"1 &lt; 2 &amp; 3 &#65;&#x42; &bogus; <video> 4 < 5");
		check(runs.size() == 9, "run count with breaks and an image");
		if (runs.size() == 9) {
			check(runs[1].kind == SPRichTextRun::Kind::LineBreak, "<br />");
			check(runs[3].kind == SPRichTextRun::Kind::LineBreak, "<br>");
			check(runs[5].kind == SPRichTextRun::Kind::LineBreak, "newline");
			check(runs[7].kind == SPRichTextRun::Kind::Image && runs[7].src == "cover.png" &&
					  runs[7].uri == "bungalow:release:9" && runs[7].width == 32,
				  "image with a uri");
			check(runs[8].text == " 1 < 2 & 3 AB &bogus; <video> 4 < 5",
				  "entities decode; unknown entities and tags stay as written");
		}
	}

	// Escaping makes data safe to put into markup.
	{
		const std::string name = "<b>AC&DC</b> \"live\"";
		const auto runs = SPRichText::parse(SPRichText::link("bungalow:artist:<2>", name));
		check(runs.size() == 1 && runs[0].text == name && !runs[0].bold &&
				  runs[0].uri == "bungalow:artist:<2>",
			  "link() escapes its text and uri");
		check(SPRichText::plainText("<b>a</b><br/>b &amp; c") == "a\nb & c", "plainText");
	}

	// Layout and hit testing.
	{
		SPRichText text("ab<link uri=\"x\">cd</link><b>ef</b><br/><link uri=\"y\">gh</link>");
		check(text.hasLinks(), "hasLinks");
		text.layout(font, measure);
		check(text.getWidth() == 64 && text.getHeight() == 32, "size of two lines");
		check(text.uriAt(5, 5).empty(), "plain text is not a link");
		check(text.uriAt(20, 0) == "x" && text.uriAt(39, 15) == "x", "link on the first line");
		check(text.uriAt(40, 5).empty(), "text after the link");
		check(text.uriAt(0, 16) == "y" && text.uriAt(19, 31) == "y", "link on the second line");
		check(text.uriAt(20, 20).empty() && text.uriAt(5, 32).empty() && text.uriAt(-1, 5).empty(),
			  "outside the text");

		text.setMarkup("plain");
		check(!text.hasLinks() && text.getFragments().empty(), "new markup drops the old layout");
		text.layout(font, measure);
		check(text.getWidth() == 50 && text.getHeight() == 16, "size of one line");
	}

	// An image takes the line height unless it says otherwise.
	{
		SPRichText text("<img src=\"a.png\" uri=\"z\"/>x<img src=\"b.png\" width=\"40\" height=\"30\"/>");
		text.layout(font, measure);
		const auto &fragments = text.getFragments();
		check(fragments.size() == 3, "image fragments");
		if (fragments.size() == 3) {
			check(fragments[0].width == 16 && fragments[0].height == 16, "default image size");
			check(fragments[2].x == 26 && fragments[2].width == 40 && fragments[2].height == 30,
				  "explicit image size");
			check(fragments[1].y == 7, "text is centered on a taller line");
		}
		check(text.getHeight() == 30 && text.uriAt(8, 14) == "z" && text.uriAt(30, 10).empty(),
			  "image link");
	}

	if (failures == 0)
		std::cout << "SPRichText tests passed\n";
	return failures == 0 ? 0 : 1;
}
