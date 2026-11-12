//
// Created by Alexander Forselius on 2025-12-11.
//

#include "SPClassicSkin.h"

#include "SPGraphicsContext.h"
#include "SPTableDataSource.h"
#include "libspace.h"

#include "../../bungalow/src/BungalowApp.h"
#include "../../bungalow/src/models/SPPlayable.h"
#include "SPAppHeaderElement.h"
#include "SPButtonElement.h"
#include "SPColor.h"

#include <cmath>
#include <iostream>

using namespace space;

bool SPClassicSkin::drawElement(SPGraphicsContext *g, SPElement *elm,
								const string &state,
								const sp_rectangle &bounds) {
	bool result = false;
	SPColor bgColor = elm->getBackgroundColor();
	SPColor fgColor = elm->getForegroundColor();
	SPColor shadowColor = this->m_shadowColor;
	std::optional<SPColor> tintColor = elm->getTintColor();
	if (tintColor.has_value()) {
		bgColor = bgColor.tint(tintColor.value());
		fgColor = fgColor.tint(tintColor.value());
		shadowColor = tintColor.value().lighten(-0);
	}
	if (elm->getType() == "input") {
		result = true;
	}
	if (elm->getType() == "splitter") {
		result = true;
		bgColor = shadowColor;
		g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
						 bgColor);
	}
	if (elm->getType() == "button" || elm->getType() == "nowplayingbarheader" ||
		elm->getType() == "nowplayingbartitle") {
		if (elm->getType() == "nowplayingbarheader" ||
			elm->getType() == "nowplayingbartitle") {
			cout << "Now Playing Bar element: " << elm->getType() << endl;
		}
		result = true;
		int kR = 25;
		if (elm->getType() == "nowplayingbarheader" ||
			elm->getType() == "nowplayingbartitle") {
			kR = 0;
		}
		SPColor bgColor = elm->getBackgroundColor();
		if (elm->getStringAttribute("variant", "default") == "primary") {
			bgColor = tintColor.value().alpha(1);
		} else {
			bgColor = bgColor.tint(tintColor.value()).alpha(1);
		}
		if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
			bgColor = bgColor.lighten(10);
		if (state == "active")
			bgColor = bgColor.lighten(-0.15f);
		else if (state == "hover")
			bgColor = bgColor.lighten(0.05f);

		SPGradient grad(bgColor.lighten(0.50f), bgColor.lighten(-0.40f), 0);
		if (state == "active")
			grad = SPGradient(grad.getEnd(), grad.getStart(), 0);
		g->fillRoundedLinearGradientRectangle(bounds.x, bounds.y, bounds.width,
											  bounds.height, kR, 0, grad);

		g->drawRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
								kR, bgColor.lighten(-0.5f));

		if (!elm->getInnerText().empty()) {
			SPGraphicsContext *gc = elm->createGraphics();
			if (g == nullptr) {
				cerr << "GraphicsContext is NULL" << endl;
				return false;
			}
			space::SPFontStyle fontStyle = elm->getFontStyle();
			string text = elm->getInnerText();
			space::sp_rectangle tSize = gc->measureString(text, fontStyle);
			int x = (elm->getWidth() / 2) - (tSize.width / 2) +
					elm->getAbsoluteBounds().x;
			int y = (elm->getHeight() / 2) - (tSize.height / 2) +
					elm->getAbsoluteBounds().y;

			gc->drawString(text, fontStyle, SPColor(10, 10, 10, 255), x, y,
						   tSize.width, tSize.height);
		}
	}
	if (elm->getType() == "table") {
	}
	if (elm->getType() == "appheader") {
		result = true;
		// Re-get fresh background color (same pattern as button) to avoid
		// double-tinting from the outer bgColor computation at the top.
		bgColor = bgColor.lighten(2);
		SPGradient gradient(bgColor.lighten(0.1), bgColor.lighten(-0.1), 0);
		g->drawLinearGradient({bounds.x, bounds.y, bounds.width, bounds.height},
							  gradient);
	}
	if (elm->getType() == "appfooter") {
		result = true;
		cout << "appfooter" << endl;
		SPGradient gradient(bgColor.lighten(0.2), bgColor.lighten(0.1), 0);
		g->drawLinearGradient({bounds.x, bounds.y, bounds.width, bounds.height},
							  gradient);
	}
	if (elm->getType() == "treeview") {
		if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK) {
			bgColor = bgColor.lighten(0.28);
		}
		result = true;
		auto *treeView = dynamic_cast<SPTreeViewElement *>(elm);
		auto width = treeView->getWidth();
		auto height = treeView->getHeight();
		SPGradient gradient(bgColor.lighten(0.5), bgColor.lighten(0), 0);
		if (state == "active") {
			gradient =
				SPGradient(SPColor(0, 0, 0, 1), SPColor(10, 10, 10, 0), 0);
		}
		g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
						 bgColor);
	}
	if (elm->getType() == "viewstack") {
		result = true;
		g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
						 bgColor);
	}
	return result;
}

void SPClassicSkin::drawTableRow(SPGraphicsContext *g, SPTableElement *table,
								 int i, int x, int y, int width, int height,
								 int row) {
	if (table->isRowSelected(row)) {
		g->fillRectangle(x, y, width, height, SPColor(166, 218, 251, 255));
	} else {
		if (row % 2 == 0) {
			SPColor bgColor = table->getBackgroundColor();
			if (auto tintColor = table->getTintColor(); tintColor)
				bgColor = bgColor.tint(
					tintColor.value_or(SPColor(200, 200, 200, 255)));
			// Semi-transparent darker tint for even rows
			SPColor stripeColor = bgColor.lighten(-0.2).alpha(100);
			if (!table->gradientView) {
				g->fillRectangle(x, y, width, height, stripeColor);
			}
		}
		auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
		if (app != nullptr) {
			if (app->getCurrentPlayable() != nullptr && table->dataSource) {
				auto *dataSource = table->dataSource;
				if (row < dataSource->getRowCount()) {
					if (dataSource->getRowUri(row) ==
						app->getCurrentPlayable()->uri) {
						g->fillRectangle(x, y, width, height,
										 SPColor(0, 0, 0, 255));
					}
				}
			}
		}
	}
}

void SPClassicSkin::drawTableHeader(SPGraphicsContext *g, SPTableElement *table,
									int x, int y, int width, int height) {
	SPColor bgColor = table->getBackgroundColor();
	if (auto tintColor = table->getTintColor(); tintColor.has_value())
		bgColor = bgColor.tint(tintColor.value());
	if (table->getColorScheme() == SP_COLOR_SCHEME_DARK)
		bgColor = bgColor.lighten(2);

	int colCount =
		(table->dataSource && table->dataSource->getColumnCount() > 0)
			? table->dataSource->getColumnCount()
			: 1;
	int colWidth = width / colCount;

	// Draw each column header cell like a button (default gradient + gloss)
	for (int col = 0; col < colCount; ++col) {
		int cx = x + col * colWidth;
		int cw = (col == colCount - 1) ? (width - col * colWidth) : colWidth;

		// Base button gradient: lighter top → darker bottom
		SPGradient gradient(bgColor.lighten(0.1), bgColor.lighten(-0.2), 0);
		g->drawLinearGradient({cx, y, cw, height}, gradient);

		// Column divider (shadow line on right edge, light line inset)
		SPColor divider = bgColor.lighten(-1.5);
		SPColor dividerLight = bgColor.lighten(1.3);
		if (col < colCount - 1) {
			g->drawLine(cx + cw - 1, y, cx + cw - 1, y + height - 1, divider);
			// g->drawLine(cx + cw,     y, cx + cw,     y + height - 1,
			// dividerLight);
		}
	}

	// Bottom border — same shadow line as button border
	SPColor borderColor = bgColor.lighten(-0.5);
	g->drawLine(x, y + height - 1, x + width, y + height - 1, borderColor);
}

void SPClassicSkin::drawTabBarToolButton(SPGraphicsContext *g,
										 SPTabBarElement *tabBar,
										 SPTabToolButton *btn, int x, int y,
										 int width, int height) {
	SPColor bgColor = tabBar->getBackgroundColor();
	std::optional<SPColor> tintColor = tabBar->getTintColor();
	if (tintColor.has_value())
		bgColor = bgColor.tint(tintColor.value());
	if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK)
		bgColor = bgColor.lighten(2);

	// Subtle inset button appearance matching the tab bar style
	SPGradient grad(bgColor.lighten(0.05f), bgColor.lighten(-0.15f), 0);
	g->drawLinearGradient({x + 1, y + 2, width - 2, height - 4}, grad);

	// Border
	SPColor border = this->m_shadowColor;
	if (tintColor.has_value())
		border = tintColor.value().lighten(-0.3f);
	g->drawRoundedRectangle(x + 1, y + 2, width - 2, height - 4, 20, border);

	// Vertical divider on the left
	g->drawLine(x, y, x, y + height - 1, border);
}

void SPClassicSkin::drawTabBarTab(SPGraphicsContext *g, SPTabBarElement *tabBar,
								  SPTab *tab, int index, bool active, int x,
								  int y, int width, int height) {
	SPColor bgColor = tabBar->getBackgroundColor();
	SPColor activeBgColor = bgColor; // preserve original for active tab
	SPColor lColor = this->m_lightColor;
	SPColor shadowColor = this->m_shadowColor;
	std::optional<SPColor> tintColor = tabBar->getTintColor();
	if (tintColor.has_value()) {
		bgColor = bgColor.tint(tintColor.value());
		activeBgColor = bgColor;
		shadowColor = tintColor.value().lighten(-0);
		lColor = lColor.tint(tintColor.value()).lighten(0.5);
	}
	if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK) {
		bgColor = bgColor.lighten(2);
		shadowColor = shadowColor.lighten(-1.7);
	}
	if (active) {
		g->fillRectangle(x, y, width, height, activeBgColor);
		if (index > 0) {
			g->drawLine(x, y, x, y + height - 1, shadowColor);
		} else {

			g->drawLine(x + width - 1, y, x + width - 1, y + height - 1,
						shadowColor);
		}
	} else {
		// g->drawLine(x, y + 1, x + width, y + 1, lColor);
		// g->drawLine(x, y + 1, x, y + height - 2, lColor);
		// g->drawLine(x + width + 1, y + 1, x + width + 1, y + height - 2,
		// lColor);
	}
	g->drawLine(x + width, y, x + width, y + height - 1, shadowColor);
}

void SPClassicSkin::drawTabBar(SPGraphicsContext *g, SPTabBarElement *tabBar,
							   int x, int y, int width, int height) {
	SPColor lColor = this->m_lightColor;
	SPColor shadowColor = this->m_shadowColor;
	SPColor bgColor = tabBar->getBackgroundColor();
	std::optional<SPColor> tintColor = tabBar->getTintColor();
	if (tintColor.has_value()) {
		bgColor = bgColor.tint(tintColor.value());
		lColor = lColor.tint(tintColor.value()).lighten(0.5);
		shadowColor = tintColor.value().lighten(-0);
	}
	if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK) {
		bgColor = bgColor.lighten(2);
		shadowColor = shadowColor.lighten(-0.5);
	}
	SPGradient gradient(bgColor.lighten(0.1), bgColor.lighten(-0.1), 0);
	g->drawLinearGradient(sp_rectangle{x, y, width, height}, gradient);

	// g->drawLine(x, y, x + width, y,shadowColor);
	// g->drawLine(x, y + 1, x + width, y + 1, lColor);
	g->drawLine(x, y + height - 1, x + width, y + height - 1, shadowColor);
}

void SPClassicSkin::drawTreeItem(SPGraphicsContext *g,
								 SPTreeViewElement *treeView, SPTreeItem *item,
								 int itemHeight, int x, int y, int width,
								 int height) {
	if (item->isSelected()) {
		g->fillRectangle(x, y, treeView->getWidth(), itemHeight,
						 SPColor(166, 218, 251, 255));
	}
	if (item->isHighlighted()) {
		g->fillRectangle(x, y, treeView->getWidth(), itemHeight,
						 SPColor(166, 218, 251, 128));
	}
}

void SPClassicSkin::drawSectionHeader(SPGraphicsContext *g,
									  SPTreeViewElement *treeView,
									  const string &title, int x, int y,
									  int width, int height) {
	// Muted label — slightly smaller font, uppercase appearance via color
	SPFontStyle sectionStyle(treeView->getFontStyle().getFamily(),
							 treeView->getFontStyle().getHeight() - 1,
							 treeView->getFontStyle().getWeight(), false,
							 false);
	SPColor labelColor(128, 128, 128, 200);
	g->drawString(title, sectionStyle, labelColor, x + 8, y + 4, width - 8,
				  height);
}

SPClassicSkin::SPClassicSkin() {}
