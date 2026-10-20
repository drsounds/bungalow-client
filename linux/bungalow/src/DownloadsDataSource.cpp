#include "DownloadsDataSource.h"

#include <filesystem>
#include <string>

namespace space {

static const char* kColumns[] = { "Name", "Type", "Source URL", "Status" };

DownloadsDataSource::DownloadsDataSource(DownloadManager* dm, Filter filter)
    : m_dm(dm), m_filter(filter) {
    refresh();
}

void DownloadsDataSource::refresh() {
    m_snapshot.clear();
    if (!m_dm) return;
    for (const auto& item : m_dm->items()) {
        bool ongoing = (item.status == DLStatus::Ongoing);
        if (m_filter == Filter::Ongoing   &&  ongoing) m_snapshot.push_back(item);
        if (m_filter == Filter::Completed && !ongoing) m_snapshot.push_back(item);
    }
}

int DownloadsDataSource::getRowCount() {
    return static_cast<int>(m_snapshot.size());
}

std::string DownloadsDataSource::getTextForColumn(int column) {
    if (column >= 0 && column < 4) return kColumns[column];
    return "";
}

std::string DownloadsDataSource::getTextForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_snapshot.size())) return "";
    const auto& item = m_snapshot[row];

    switch (column) {
        case 0: return item.filename;
        case 1: {
            // Show a friendly type label from the content-type or extension
            std::string ext = std::filesystem::path(item.filename).extension().string();
            if (!ext.empty() && ext.front() == '.') ext = ext.substr(1);
            if (!item.contentType.empty()) {
                auto slash = item.contentType.find('/');
                if (slash != std::string::npos) {
                    std::string sub = item.contentType.substr(slash + 1);
                    auto semi = sub.find(';');
                    if (semi != std::string::npos) sub = sub.substr(0, semi);
                    if (!sub.empty() && sub != "octet-stream") return sub;
                }
            }
            return ext.empty() ? "File" : ext;
        }
        case 2: return item.url;
        case 3: {
            // Status column — text shown when there is no progress bar
            switch (item.status) {
                case DLStatus::Completed: return "Completed";
                case DLStatus::Failed:
                    return item.errorMsg.empty() ? "Failed" : "Failed: " + item.errorMsg;
                case DLStatus::Cancelled: return "Cancelled";
                case DLStatus::Ongoing: {
                    // Show percentage as text fallback (in case progress bar isn't drawn)
                    if (item.totalBytes > 0) {
                        int pct = static_cast<int>(item.progress() * 100.f);
                        return std::to_string(pct) + "%";
                    }
                    return "Downloading…";
                }
            }
            return "";
        }
        default: return "";
    }
}

float DownloadsDataSource::getProgressForRow(int row) {
    if (row < 0 || row >= static_cast<int>(m_snapshot.size())) return -1.f;
    const auto& item = m_snapshot[row];
    if (item.status != DLStatus::Ongoing) return -1.f;
    return item.progress();
}

int64_t DownloadsDataSource::getItemIdForRow(int row) const {
    if (row < 0 || row >= static_cast<int>(m_snapshot.size())) return 0;
    return m_snapshot[row].id;
}

} // namespace space
