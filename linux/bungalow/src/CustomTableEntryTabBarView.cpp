#include "CustomTableEntryTabBarView.h"
#include "../../space/src/SPColor.h"
#include "../../space/src/SPHBoxElement.h"
#include "../../space/src/SPHeaderElement.h"
#include "../../space/src/SPImageElement.h"
#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTextElement.h"
#include "../../space/src/SPTitleElement.h"
#include "../../space/src/SPVBoxElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTabBarElement.h"
#include "CustomTableEntryView.h"

#include "SPEntityResolver.h"

#include <regex>
#include <string>

namespace space {
namespace views {

// ── URI helpers
// ───────────────────────────────────────────────────────────────

static bool parseEntryUri(const std::string &uri, std::string &tableName,
						  std::string &rowUuid) {
	static const std::string kPrefix = "bungalow:table:";
	if (uri.rfind(kPrefix, 0) != 0)
		return false;

	std::string rest = uri.substr(kPrefix.size()); // "<tableName>:<uuid>"
	size_t colon = rest.rfind(':');
	if (colon == std::string::npos || colon == 0)
		return false;

	std::string id = rest.substr(colon + 1);
	if (id.empty())
		return false;

	tableName = rest.substr(0, colon);
	rowUuid = id;
	return !tableName.empty();
}

// ── Construction
// ──────────────────────────────────────────────────────────────

CustomTableEntryTabBarView::CustomTableEntryTabBarView() :
	SPTabBarViewElement() {}

CustomTableEntryTabBarView::CustomTableEntryTabBarView(SPElement *parent,
													   sqlite3 *db) :
	SPTabBarViewElement(parent), m_db(db) {
	this->customTableEntryView = new CustomTableEntryView(this, db);
	this->tabStack->appendChild(this->customTableEntryView);
	this->tabBar->addTab("overview", "Overview", "");
}

CustomTableEntryTabBarView::~CustomTableEntryTabBarView() {}

// ── URI routing
// ───────────────────────────────────────────────────────────────

bool CustomTableEntryTabBarView::acceptsUri(const std::string &uri) {
	// Match "bungalow:table:<name>:<uuid>" — any non-empty trailing segment
	std::string t, id;
	return parseEntryUri(uri, t, id);
}

void CustomTableEntryTabBarView::navigate(const std::string &uri) {
	std::string tableName, rowUuid;
	if (!parseEntryUri(uri, tableName, rowUuid))
		return;

	m_tableName = tableName;
	m_rowId = rowUuid;
	this->setUri(uri);
	this->customTableEntryView->navigate(uri);
	// rebuild(); // Rebuild should be in the child view or managed there
}
} // namespace views
} // namespace space
