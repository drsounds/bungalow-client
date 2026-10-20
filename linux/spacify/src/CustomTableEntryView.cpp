#include "CustomTableEntryView.h"

#include "../../space/src/SPColor.h"
#include "../../space/src/SPHBoxElement.h"
#include "../../space/src/SPHeaderElement.h"
#include "../../space/src/SPImageElement.h"
#include "../../space/src/SPTextElement.h"
#include "../../space/src/SPTitleElement.h"
#include "../../space/src/SPVBoxElement.h"

#include "SPEntityResolver.h"

#include <regex>
#include <string>

namespace space {
namespace views {

// ── URI helpers
// ───────────────────────────────────────────────────────────────

// "bungalow:table:<tableName>:<rowId>"
// Extracts tableName and rowId from the URI.
// Returns true on success.
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

CustomTableEntryView::CustomTableEntryView() :
	SPViewElement() {}

CustomTableEntryView::CustomTableEntryView(SPElement *parent, sqlite3 *db) :
	SPViewElement(parent), m_db(db) {
	this->clipView = false;

	// Hero header — will be populated in rebuild()
	m_header = new SPHeaderElement(this);
	m_header->setHeight(140);
	this->appendChild(m_header);

	// Body — flexible scrollable area for field rows
	m_body = new SPVBoxElement(this);
	m_body->setFlex(1);
	m_body->setPadding({12, 16, 12, 16});
	m_body->setGap(0);
	this->appendChild(m_body);
}

CustomTableEntryView::~CustomTableEntryView() {}

// ── URI routing
// ───────────────────────────────────────────────────────────────

bool CustomTableEntryView::acceptsUri(const std::string &uri) {
	// Match "bungalow:table:<name>:<uuid>" — any non-empty trailing segment
	std::string t, id;
	return parseEntryUri(uri, t, id);
}

void CustomTableEntryView::navigate(const std::string &uri) {
	std::string tableName, rowUuid;
	if (!parseEntryUri(uri, tableName, rowUuid))
		return;

	m_tableName = tableName;
	m_rowId = rowUuid;
	this->setUri(uri);
	rebuild();
}

// ── Data loading
// ──────────────────────────────────────────────────────────────

bool CustomTableEntryView::loadData(CustomTableDef &outDef,
									std::vector<std::string> &outValues) {
	if (!m_db || m_tableName.empty() || m_rowId.empty())
		return false;

	// Load column definitions
	{
		sqlite3_stmt *stmt = nullptr;
		if (sqlite3_prepare_v2(m_db,
							   "SELECT id, name, columns, display_name, "
							   "noun_singular, noun_plural, description"
							   " FROM db_tables WHERE name = ? LIMIT 1;",
							   -1, &stmt, nullptr) != SQLITE_OK)
			return false;
		sqlite3_bind_text(stmt, 1, m_tableName.c_str(), -1, SQLITE_TRANSIENT);
		bool found = false;
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			auto txt = [&](int col) -> std::string {
				const unsigned char *t = sqlite3_column_text(stmt, col);
				return t ? reinterpret_cast<const char *>(t) : "";
			};
			outDef.id = sqlite3_column_int(stmt, 0);
			outDef.name = txt(1);
			outDef.columns = Database::decodeColumnsPublic(txt(2));
			outDef.displayName = txt(3);
			outDef.nounSingular = txt(4);
			outDef.nounPlural = txt(5);
			outDef.description = txt(6);
			found = true;
		}
		sqlite3_finalize(stmt);
		if (!found)
			return false;
	}

	// Load row values
	{
		std::string sql =
			"SELECT * FROM user_" + m_tableName + " WHERE id=? LIMIT 1;";
		sqlite3_stmt *stmt = nullptr;
		if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) !=
			SQLITE_OK)
			return false;
		sqlite3_bind_text(stmt, 1, m_rowId.c_str(), -1, SQLITE_TRANSIENT);
		bool found = false;
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			int ncols = sqlite3_column_count(stmt);
			// col 0 = id, cols 1..N = user columns
			for (int i = 1; i < ncols; ++i) {
				const unsigned char *t = sqlite3_column_text(stmt, i);
				outValues.push_back(t ? reinterpret_cast<const char *>(t) : "");
			}
			found = true;
		}
		sqlite3_finalize(stmt);
		return found;
	}
}

// ── UI rebuild
// ────────────────────────────────────────────────────────────────

void CustomTableEntryView::rebuild() {
	CustomTableDef def;
	std::vector<std::string> values;
	bool ok = loadData(def, values);

	// ── Hero header
	// ───────────────────────────────────────────────────────────
	if (m_header) {
		// Title: prefer first column value; fall back to table name
		std::string title = (!ok || values.empty()) ? m_tableName : values[0];
		if (title.empty())
			title = m_tableName;

		// Entity line: prefer nounSingular → displayName → table name, plus
		// short UUID
		std::string noun =
			def.nounSingular.empty()
				? (def.displayName.empty() ? def.name : def.displayName)
				: def.nounSingular;
		std::string shortUuid =
			m_rowId.size() > 8 ? m_rowId.substr(0, 8) + "…" : m_rowId;
		std::string entity = noun + "  ·  " + shortUuid;

		m_header->title->setTitle(title);
		m_header->title->setEntity(entity);
		m_header->image->setSrc("icons/Document.svg");
	}

	/*
	// ── Body — clear & rebuild field rows
	// ─────────────────────────────────────
	if (!m_body)
		return;
	m_body->clear();

	if (!ok) {
		// Error state
		auto *errLabel = new SPTextElement(m_body);
		errLabel->setInnerText("Row not found.");
		errLabel->setHeight(24);
		errLabel->setForegroundColor(SPColor(180, 80, 80, 255));
		m_body->appendChild(errLabel);
		m_body->pack();
		this->pack();
		this->invalidate();
		return;
	}

	// Column separator after header
	auto *sep = new SPTextElement(m_body);
	sep->setHeight(1);
	sep->setBackgroundColor(SPColor(60, 60, 60, 180));
	sep->setFlex(0);
	m_body->appendChild(sep);

	// One row per column
	for (size_t i = 0; i < def.columns.size(); ++i) {
		const std::string &colName = def.columns[i].first;
		const std::string &colType =
			def.columns[i].second; // e.g. "TEXT", "BELONGS_TO:artists"
		const std::string &colValue = (i < values.size()) ? values[i] : "";

		// Resolve BELONGS_TO relations
		bool isBelongsTo = (colType.rfind("BELONGS_TO:", 0) == 0);
		std::string displayValue = colValue;
		std::string typeBadge = colType;
		if (isBelongsTo && m_db && !colValue.empty()) {
			// The stored value is bungalow:<target>:<uuid>
			SPEntity ent = SPEntityResolver::resolve(m_db, colValue);
			if (ent.valid) {
				displayValue = ent.name.empty() ? colValue : ent.name;
			}
			// Show "→ <target_table>" as the badge
			typeBadge = "→ " + colType.substr(11);
		}

		// Row container (HBox)
		auto *row = new SPHBoxElement(m_body);
		row->setHeight(44);
		row->setPadding({0, 8, 0, 8});
		row->setGap(12);

		// Column name label (left, fixed width)
		auto *nameLabel = new SPTextElement(row);
		nameLabel->setWidth(160);
		nameLabel->setHeight(44);
		nameLabel->setInnerText(colName);
		nameLabel->setForegroundColor(SPColor(140, 140, 140, 255));
		nameLabel->setIntegerAttribute("size", 11);
		row->appendChild(nameLabel);

		// Type badge
		auto *typeLabel = new SPTextElement(row);
		typeLabel->setWidth(80);
		typeLabel->setHeight(44);
		typeLabel->setInnerText(typeBadge);
		typeLabel->setForegroundColor(
			isBelongsTo ? SPColor(100, 140, 200, 255)  // blue-ish for relations
						: SPColor(90, 110, 140, 255)); // grey for scalar types
		typeLabel->setIntegerAttribute("size", 10);
		row->appendChild(typeLabel);

		// Value label (flex)
		auto *valueLabel = new SPTextElement(row);
		valueLabel->setFlex(1);
		valueLabel->setHeight(44);
		valueLabel->setInnerText(displayValue.empty() ? "—" : displayValue);
		valueLabel->setForegroundColor(SPColor(220, 220, 220, 255));
		valueLabel->setIntegerAttribute("size", 12);
		row->appendChild(valueLabel);

		m_body->appendChild(row);

		// Thin divider between rows (skip after last)
		if (i + 1 < def.columns.size()) {
			auto *div = new SPTextElement(m_body);
			div->setHeight(1);
			div->setBackgroundColor(SPColor(45, 45, 45, 200));
			m_body->appendChild(div);
		}
	}

	m_body->pack();
	*/
	this->pack();
	this->invalidate();
}

} // namespace views
} // namespace space
