#include "MainElement.h"
#include "FaviconCache.h"
#include "BungalowApp.h"
#include "LibraryView.h"
#include "StartView.h"
#include "SPInputElement.h"
#include "SPInputEventArgs.h"
#include "SPWindowElement.h"
#include "SPSplitterElement.h"
#include "SPNavigateEventArgs.h"
#include "SPInfoElement.h"
#include "SPTreeItem.h"
#include "SPViewStackElement.h"
#include "SPTextElement.h"
#include "SPSkin.h"
#include "PlayQueueView.h"
#include "SpaceView.h"
#include "SPAppFooterElement.h"
#include "SPNowPlayingBarElement.h"
#include "SPAppHeaderElement.h"
#include "SPTreeViewElement.h"
#include "SPButtonElement.h"
#include "ConfigView.h"
#include "LocalFilesView.h"
#include "SiteView.h"
#include "AppView.h"
#include "CuratorView.h"
#include "StoreView.h"
#include "DownloadsView.h"
#include "PlayerView.h"
#include "DownloadManager.h"
#include "SPBookmarkEventArgs.h"
#include "Database.h"
#include "SPUri.h"

#include "PlatformUi.h"

namespace space {

    MainElement::MainElement()
    {
    }

    void MainElement::hideMessage() {
        this->infoElement->hide();
        this->infoElement->invalidate();
    }

    void MainElement::showMessage(SPMessageType msgType, string msg) {
        this->infoElement->show(msgType, msg);
    }

    // ── Helper: normalize alternative URI scheme prefixes ────────────────────
    // space:, bungalow:, and musik: are all aliases for the bungalow: scheme.
    static string normalizeScheme(const string& uri) {
        static const pair<const char*, const char*> kAliases[] = {
            { "space:",    "bungalow:" },
            { "bungalow:", "bungalow:" },
            { "musik:",    "bungalow:" },
        };
        for (auto& [alias, canonical] : kAliases) {
            if (uri.rfind(alias, 0) == 0)
                return canonical + uri.substr(strlen(alias));
        }
        return uri;
    }

    // ── Helper: returns true if the string starts with a known bungalow scheme ─
    static bool hasBungalowScheme(const string& uri) {
        return uri.rfind("bungalow:", 0) == 0
            || uri.rfind("space:",    0) == 0
            || uri.rfind("bungalow:", 0) == 0
            || uri.rfind("musik:",    0) == 0;
    }

    // ── Helper: favicon domain from a bungalow URI ─────────────────────────────
    // Returns the effective hostname for favicon fetching, applying the
    // appropriate hosted-domain suffix for bare names (app/curator/store).
    // Returns "" for URIs that don't map to a web domain.
    /*static*/ string MainElement::faviconDomain(const string& uri) {
        struct SchemeEntry { const char* prefix; size_t len; const char* hosted; };
        static const SchemeEntry kSchemes[] = {
            { "bungalow:site:",    13, nullptr                },
            { "bungalow:app:",     12, ".apps.bungalow.se"     },
            { "bungalow:curator:", 16, ".curator.bungalow.se"  },
            { "bungalow:store:",   14, ".store.bungalow.se"    },
        };

        for (auto& s : kSchemes) {
            if (uri.rfind(s.prefix, 0) != 0) continue;
            string remainder = uri.substr(s.len);
            size_t colon = remainder.find(':');
            string domain = (colon == string::npos) ? remainder : remainder.substr(0, colon);
            if (s.hosted && domain.find('.') == string::npos && !domain.empty())
                domain += s.hosted;
            return domain;
        }
        return "";
    }

    // ── Helper: extract domain-level URI ─────────────────────────────────────
    // bungalow:site:example.com:page -> bungalow:site:example.com
    // bungalow:app:finder:sub       -> bungalow:app:finder
    // Returns empty string if the URI is already the domain level
    static string domainUri(const string& uri) {
        // Find 3rd colon (after "bungalow:scheme:domain")
        size_t first = uri.find(':');
        if (first == string::npos) return "";
        size_t second = uri.find(':', first + 1);
        if (second == string::npos) return "";
        size_t third = uri.find(':', second + 1);
        if (third == string::npos) return ""; // already at domain level
        return uri.substr(0, third);
    }

    // ── Nav item event handlers ───────────────────────────────────────────────

    int MainElement::treeview_itemselected(SPObject *sender, SPEventArgs *e) {
        auto *args = (TreeViewEventArgs *)e;
        SPTreeItem *selectedItem = args->getItem();
        SPUri *uri = selectedItem->getUri();
        if (!uri || uri->getUri().empty()) return 0;
        this->navigate(uri->getUri());
        return 0;
    }

    int MainElement::onNavItemContextMenu(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<TreeViewEventArgs*>(e);
        if (!args) return 0;
        SPTreeItem* item = args->getItem();
        if (!item || !item->isBookmark()) return 0;

        // Capture URI before menu closes (item pointer may be invalidated after rebuild)
        string uri = item->getUri() ? item->getUri()->getUri() : "";

        enum { ID_REMOVE_BOOKMARK = 10200 };
        const int sel = platform::showPopupMenu({ { ID_REMOVE_BOOKMARK, "Remove bookmark" } });

        if (sel == ID_REMOVE_BOOKMARK && !uri.empty()) {
            auto* app = BungalowApp::current();
            if (app && app->getDatabase())
                app->getDatabase()->removeBookmark(uri);
            rebuildNavItems();
            treeView->invalidate();
        }
        return 0;
    }

    // ── Bookmark toggled (fired from SiteView / AppView) ─────────────────────

    int MainElement::onBookmarkToggled(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<space::SPBookmarkEventArgs*>(e);
        if (!args) return 0;

        auto* app = BungalowApp::current();
        if (!app || !app->getDatabase()) return 0;
        Database* db = app->getDatabase();

        const string& uri   = args->getUri();
        const string& title = args->getTitle();

        bool wasBookmarked = db->isBookmarked(uri);

        if (wasBookmarked) {
            db->removeBookmark(uri);
        } else {
            string parent = domainUri(uri);
            if (parent.empty()) {
                db->addBookmark(uri, title);
            } else {
                if (!db->isBookmarked(parent))
                    db->addBookmark(parent, parent);
                int parentId = db->getBookmarkId(parent);
                db->addBookmark(uri, title, parentId);
            }

            // Kick off a favicon fetch for newly added site/app bookmarks
            if (m_faviconCache) {
                string domain = faviconDomain(uri);
                if (!domain.empty()) {
                    string faviconUrl = "https://" + domain + "/favicon.ico";
                    m_faviconCache->fetchIfNeeded(domain, faviconUrl, [this]() {
                        rebuildNavItems();
                        treeView->invalidate();
                    });
                }
            }
        }

        rebuildNavItems();
        treeView->invalidate();
        return 0;
    }

    // ── Navigation ────────────────────────────────────────────────────────────

    void MainElement::navigate(const string& uri) const
    {
        string normalized = normalizeScheme(uri);
        this->viewStack->navigate(normalized);
        this->searchElement->setValue(uri);

        treeView->forEachItem([&normalized](SPTreeItem* item) {
            if (item->getUri() && !item->getUri()->getUri().empty())
                item->setSelected(item->getUri()->getUri() == normalized);
        });
        treeView->invalidate();
    }

    // ── Nav rebuild (called on startup and after bookmark changes) ────────────

    void MainElement::rebuildNavItems() {
        treeView->clearItems();

        auto* app = BungalowApp::current();
        Database* db = (app && app->getDatabase()) ? app->getDatabase() : nullptr;

        // Resolve icon path: use cached favicon if available, otherwise the default SVG.
        auto resolveIcon = [&](const string& uri, const string& fallback) -> string {
            string domain = faviconDomain(uri);
            if (domain.empty() || !m_faviconCache) return fallback;
            string cached = m_faviconCache->getCachedPath(domain);
            return cached.empty() ? fallback : cached;
        };

        // Build parent+children bookmark items for a given URI prefix and default icon.
        auto addBookmarkSection = [&](const string& prefix, const string& defaultIcon) {
            if (!db) return;
            for (auto& parent : db->getTopLevelBookmarks(prefix)) {
                auto* parentItem = new SPTreeItem(new SPUri(parent.uri));
                parentItem->setText(parent.title);
                parentItem->setIsBookmark(true);
                parentItem->setCollapsed(false);
                parentItem->setIconPath(resolveIcon(parent.uri, defaultIcon));
                for (auto& child : db->getChildBookmarks(parent.id)) {
                    auto* childItem = new SPTreeItem(new SPUri(child.uri));
                    childItem->setText(child.title);
                    childItem->setIsBookmark(true);
                    childItem->setIconPath(resolveIcon(child.uri, defaultIcon));
                    parentItem->addChild(childItem);
                }
                treeView->addItem(parentItem);
            }
        };

        // ── MAIN ─────────────────────────────────────────────────────────────
        treeView->addSection("MAIN");


        auto* start = new SPTreeItem(new SPUri("bungalow:start"));
        start->setText("Start");
        start->setIconPath("icons/NowPlaying.svg");
        treeView->addItem(start);

        auto* itemNowPlaying = new SPTreeItem(new SPUri("bungalow:player"));
        itemNowPlaying->setText("Now Playing");
        itemNowPlaying->setIconPath("icons/NowPlaying.svg");
        treeView->addItem(itemNowPlaying);

        // ── LIBRARY ──────────────────────────────────────────────────────────
        treeView->addSection("LIBRARY");

        auto* itemSettings = new SPTreeItem(new SPUri("bungalow:config"));
        itemSettings->setText("Settings");
        itemSettings->setIconPath("icons/Document.svg");
        itemSettings->setCollapsed(false); // expanded by default
        treeView->addItem(itemSettings);

        auto* itemLibrary = new SPTreeItem(new SPUri("bungalow:library"));
        itemLibrary->setText("Library");
        itemLibrary->setIconPath("icons/Library.svg");
        auto* itemTracks = new SPTreeItem(new SPUri("bungalow:library"));
        itemTracks->setText("Tracks");
        itemTracks->setIconPath("icons/MusicNote.svg");
        itemLibrary->addChild(itemTracks);
        treeView->addItem(itemLibrary);

        auto* itemLocalFiles = new SPTreeItem(new SPUri("bungalow:localfiles"));
        itemLocalFiles->setText("Local Files");
        itemLocalFiles->setIconPath("icons/MusicFile.svg");
        itemLibrary->addChild(itemLocalFiles);

        auto* itemDownloads = new SPTreeItem(new SPUri("bungalow:downloads"));
        itemDownloads->setText("Downloads");
        itemDownloads->setIconPath("icons/Downloads.svg");
        treeView->addItem(itemDownloads);
        
        if (false) {
            // ── APPS ─────────────────────────────────────────────────────────────
            treeView->addSection("APPS");

            auto* itemAppFinder = new SPTreeItem(new SPUri("bungalow:app:finder"));
            itemAppFinder->setText("App Finder");
            itemAppFinder->setIconPath(resolveIcon("bungalow:app:finder", "icons/App.svg"));
            treeView->addItem(itemAppFinder);

            addBookmarkSection("bungalow:app:",  "icons/App.svg");

            // ── CURATORS ──────────────────────────────────────────────────────────
            treeView->addSection("CURATORS");

            addBookmarkSection("bungalow:curator:", "icons/WebPage.svg");

            // ── STORE ─────────────────────────────────────────────────────────────
            treeView->addSection("STORE");

            addBookmarkSection("bungalow:store:", "icons/WebPage.svg");

            // ── WEB SITES ─────────────────────────────────────────────────────────
            treeView->addSection("WEB SITES");

            addBookmarkSection("bungalow:site:", "icons/WebPage.svg");
        }
    }

    // ── createElement ─────────────────────────────────────────────────────────

    SPElement* MainElement::createElement(const string& name, SPElement *parent) {
        SPElement *elm = nullptr;
        if (name == "appheader") {
            elm = new SPAppHeaderElement(parent);
            assert(elm->getParentElement() != nullptr);
        } else if (name == "viewstack") {
            elm = new SPViewStackElement(parent);
            assert(elm->getParentElement() != nullptr);
        } else if (name == "treeview") {
            elm = new SPTreeViewElement(parent);
            assert(elm->getParentElement() != nullptr);
        } else if (name == "appfooter") {
            elm = new SPAppFooterElement(parent);
        } else if (name == "nowplayingbar") {
            elm = new SPNowPlayingBarElement(parent);
            assert(elm->getParentElement() != nullptr);
        } else if (name == "tabbar") {
            elm = new SPTabBarElement(parent);
            assert(elm->getParentElement() != nullptr);
        } else {
            elm = SPElement::createElement(name, parent);
            assert(elm->getParentElement() != nullptr);
        }
        return elm;
    }

    void MainElement::onLoad(SPNode *sender, SPEventArgs *args) {
    }

    // ── layout() ─────────────────────────────────────────────────────────────

    void MainElement::layout() {
        this->clipView = false;

        header = (SPAppHeaderElement *)this->getElementById("header");
        header->setHeight(60);
        header->setPadding({ 5, 25, 5, 5 });
        header->setGap(5);

        backButton = (SPButtonElement *)this->getElementById("backButton");
        backButton->setInnerText("<");
        backButton->setWidth(28);
        backButton->setHeight(28);
        backButton->addEventListener("click", SPCallback(&MainElement::onBackClicked), this, backButton);

        forwardButton = (SPButtonElement *)this->getElementById("forwardButton");
        forwardButton->setInnerText(">");
        forwardButton->setWidth(28);
        forwardButton->setHeight(28);
        forwardButton->addEventListener("click", SPCallback(&MainElement::onForwardClicked), this, forwardButton);

        searchElement = (SPInputElement *)this->getElementById("searchInput");
        searchElement->setWidth(120);
        searchElement->setHeight(28);
        searchElement->setColorAttribute("bgcolor", SPColor("#777777"));
        searchElement->addEventListener("enter", reinterpret_cast<SPCallback>(&MainElement::onSearchEntered), this, this);

        infoElement = new space::SPInfoElement(this);
        infoElement->setHeight(28);
        infoElement->setId("Info");
        infoElement->hide();

        body = (SPHBoxElement *)this->getElementById("body");
        body->setFlex(1);
        leftSidebar = (SPVBoxElement *)this->getElementById("leftSidebar");

        SPElement *leftTabBar = leftSidebar->getElementById("leftTabBar");
        tabBar = (SPTabBarElement *)leftTabBar;
        treeView = (SPTreeViewElement *)this->getElementById("treeview");

        treeView->addEventListener("itemselected",   SPCallback(&MainElement::treeview_itemselected), this, treeView);
        treeView->addEventListener("itemcontextmenu", SPCallback(&MainElement::onNavItemContextMenu),  this, treeView);
        treeView->setFlex(1);

        rebuildNavItems();

        // Now-playing widget pinned to the bottom of the sidebar
        m_nowPlayingBar = new SPNowPlayingBarElement(leftSidebar);
        leftSidebar->appendChild(m_nowPlayingBar);

        // Add viewstack
        viewStack = (SPViewStackElement *)this->getElementById("viewstack");
        viewStack->setFlex(1);
        sqlite3* db = nullptr;
        if (auto* app = BungalowApp::current())
            db = app->getDatabase() ? app->getDatabase()->handle() : nullptr;

        DownloadManager* dm = nullptr;
        if (auto* app = BungalowApp::current())
            dm = app->getDownloadManager();

        viewStack->appendChild(new views::SpaceView(viewStack));
        viewStack->appendChild(new views::PlayerView(viewStack));
        viewStack->appendChild(new views::StartView(viewStack));
        viewStack->appendChild(new views::ConfigView(viewStack));
        viewStack->appendChild(new views::LibraryView(viewStack, db));
        viewStack->appendChild(new views::LocalFilesView(viewStack, db));
        viewStack->appendChild(new views::DownloadsView(viewStack, dm));
        viewStack->appendChild(new views::SiteView(viewStack));
        viewStack->appendChild(new views::AppView(viewStack));
        viewStack->appendChild(new views::CuratorView(viewStack));
        viewStack->appendChild(new views::StoreView(viewStack));

        body->appendChild(new SPSplitterElement(body));
        auto *playQueuePanel = new views::PlayQueueView(body);
        body->appendChild(playQueuePanel);
        body->pack();

        /*
        // ── Track title label ──────────────────────────────────────────────────
        m_trackLabel = new SPTextElement(this);
        m_trackLabel->setInnerText("");
        m_trackLabel->setHeight(22);
        m_trackLabel->setForegroundColor(SPColor(200, 200, 200, 255));
        this->appendChild(m_trackLabel);
        */
        footer = (SPAppFooterElement *)this->getElementById("footer");
        footer->setHeight(32);
        footer->setId("footer");

        this->navigate("bungalow:start");
    }

    MainElement::MainElement(SPElement *parent) : SPVBoxElement(parent)
    {
        m_faviconCache = new FaviconCache();
        this->loadXUL("views/app.xml");
        this->layout();
        this->addEventListener("load", SPCallback(&MainElement::onLoad), this, this);
        this->getWindowElement()->addEventListener("navigate",         static_cast<SPCallback>(&MainElement::onNavigate),       this, this);
        this->getWindowElement()->addEventListener("bookmarktoggled",  static_cast<SPCallback>(&MainElement::onBookmarkToggled), this, this);
    }

    int MainElement::onNavigate(SPObject* /*sender*/, SPEventArgs* args)
    {
        auto *eventArgs = dynamic_cast<SPNavigateEventArgs *>(args);
        if (!eventArgs) return 0;
        this->navigate(eventArgs->getUrl());
        return 0;
    }

    int MainElement::onSearchEntered(SPObject *, SPEventArgs *eventArgs) const
    {
        auto *ie = dynamic_cast<SPInputEventArgs*>(eventArgs);
        if (!ie) return 0;
        string query = ie->getText();
        if (!hasBungalowScheme(query)) {
            query = "bungalow:search:" + query;
        }
        this->navigate(query);
        return 0;
    }

    int MainElement::onBackClicked(SPObject *, SPEventArgs *) {
        if (!viewStack->canGoBack()) return 0;
        viewStack->goBack();
        const string& uri = viewStack->getCurrentUri();
        searchElement->setValue(uri);
        treeView->forEachItem([&uri](SPTreeItem* item) {
            if (item->getUri() && !item->getUri()->getUri().empty())
                item->setSelected(item->getUri()->getUri() == uri);
        });
        treeView->invalidate();
        return 0;
    }

    int MainElement::onForwardClicked(SPObject *, SPEventArgs *) {
        if (!viewStack->canGoForward()) return 0;
        viewStack->goForward();
        const string& uri = viewStack->getCurrentUri();
        searchElement->setValue(uri);
        treeView->forEachItem([&uri](SPTreeItem* item) {
            if (item->getUri() && !item->getUri()->getUri().empty())
                item->setSelected(item->getUri()->getUri() == uri);
        });
        treeView->invalidate();
        return 0;
    }

    MainElement::~MainElement()
    {
        delete m_faviconCache;
    }

    SPViewStackElement *MainElement::getViewStack() {
        return this->viewStack;
    }

    void MainElement::setFullWidthAddressBar(bool fullWidth) {
        if (!searchElement) return;
        if (fullWidth) {
            searchElement->setFlex(1);
            searchElement->setWidth(0);
        } else {
            searchElement->setFlex(0);
            searchElement->setWidth(120);
        }
    }
}
