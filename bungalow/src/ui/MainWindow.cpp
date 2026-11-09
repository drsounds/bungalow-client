#include "ui/MainWindow.h"

#include "ui/LeftLibraryMenu.h"
#include "app/BuiltinRegistry.h"
#include "concept/ConceptManager.h"
#include "feature/FeatureManager.h"
#include "search/SearchManager.h"
#include "web/BookmarkManager.h"
#include "playlist/Upsl.h"

#include "BungalowApp.h"
#include "SPInputElement.h"
#include "SPInputEventArgs.h"
#include "SPWindowElement.h"
#include "SPSplitterElement.h"
#include "SPNavigateEventArgs.h"
#include "SPInfoElement.h"
#include "SPTreeItem.h"
#include "SPViewStackElement.h"
#include "SPTextElement.h"
#include "PlayQueueView.h"
#include "SpaceView.h"
#include "SPAppFooterElement.h"
#include "SPNowPlayingBarElement.h"
#include "SPAppHeaderElement.h"
#include "SPTreeViewElement.h"
#include "SPButtonElement.h"
#include "SPTabBarElement.h"
#include "SPBookmarkEventArgs.h"
#include "Database.h"
#include "SPUri.h"

#include "PlatformUi.h"
#include "ConfigView.h"

#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace space {
namespace ui {

    MainWindow::MainWindow()
    {
    }

    void MainWindow::hideMessage() {
        this->infoElement->hide();
        this->infoElement->invalidate();
    }

    void MainWindow::showMessage(SPMessageType msgType, string msg) {
        this->infoElement->show(msgType, msg);
    }

    // ── Helper: normalize alternative URI scheme prefixes ────────────────────
    // space:, bungalow:, and musik: are all aliases for the bungalow: scheme,
    // except federated playlist links, which are canonically musik:federated:
    // (RFC-0001) whichever spelling they came in.
    static string normalizeScheme(const string& uri) {
        if (playlist::Upsl::isUpsl(uri))
            return playlist::Upsl::canonicalUri(uri);
        static const pair<const char*, const char*> kAliases[] = {
            { "space:",    "bungalow:" },
            { "bungalow:", "bungalow:" },
            { "musik:",    "bungalow:" },
            { "spotify:", "bungalow:" },
            { "spacify:", "bungalow:" },
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
            || uri.rfind("musik:",    0) == 0
            || uri.rfind("spacify:", 0) == 0
            || uri.rfind("spotify:", 0) == 0;
    }

    // ── Nav item event handlers ───────────────────────────────────────────────

    int MainWindow::treeview_itemselected(SPObject *sender, SPEventArgs *e) {
        auto *args = (TreeViewEventArgs *)e;
        SPTreeItem *selectedItem = args->getItem();
        SPUri *uri = selectedItem->getUri();
        if (!uri || uri->getUri().empty()) return 0;
        this->navigate(uri->getUri());
        return 0;
    }

    int MainWindow::onNavItemContextMenu(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<TreeViewEventArgs*>(e);
        if (!args) return 0;
        SPTreeItem* item = args->getItem();
        if (!item) return 0;

        // Capture URI before menu closes (item pointer may be invalidated after rebuild)
        string uri = item->getUri() ? item->getUri()->getUri() : "";

        enum { ID_REMOVE_BOOKMARK = 10200, ID_RENAME, ID_DELETE };
        platform::MenuItems menu;
        if (item->isBookmark()) menu.push_back({ ID_REMOVE_BOOKMARK, "Remove bookmark" });
        if (m_sidebar->canRename(item)) menu.push_back({ ID_RENAME, "Rename" });
        if (m_sidebar->canRemove(item)) menu.push_back({ ID_DELETE, "Delete" });
        if (menu.empty()) return 0;

        const int sel = platform::showPopupMenu(menu);

        if (sel == ID_REMOVE_BOOKMARK && !uri.empty()) {
            auto* app = BungalowApp::current();
            if (app && app->getDatabase())
                web::BookmarkManager::remove(*app->getDatabase(), uri);
        } else if (sel == ID_RENAME) {
            m_sidebar->beginRename(item);
        } else if (sel == ID_DELETE) {
            m_sidebar->remove(item);
        }
        return 0;
    }

    int MainWindow::onNavItemRenamed(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<TreeViewEventArgs*>(e);
        if (args && args->getItem()) m_sidebar->rename(args->getItem(), args->getText());
        return 0;
    }

    int MainWindow::onNavItemMoved(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<TreeViewEventArgs*>(e);
        if (args && args->getItem()) m_sidebar->move(args->getItem(), args->getIndex());
        return 0;
    }

    int MainWindow::onNavItemDragHovered(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<TreeViewEventArgs*>(e);
        SPTreeItem* item = args ? args->getItem() : nullptr;
        if (item && item->getUri() && !item->getUri()->getUri().empty())
            navigate(item->getUri()->getUri());
        return 0;
    }

    int MainWindow::onSidebarDragMove(SPObject* sender, SPEventArgs* e) {
        return m_sidebar ? m_sidebar->onDragMove(sender, e) : 0;
    }

    int MainWindow::onSidebarDragEnd(SPObject* sender, SPEventArgs* e) {
        return m_sidebar ? m_sidebar->onDragEnd(sender, e) : 0;
    }

    // ── Bookmark toggled (fired from SiteView / AppView) ─────────────────────
    // The web concept follows BookmarkEvents to refresh the sidebar and fetch
    // favicons.

    int MainWindow::onBookmarkToggled(SPObject* /*sender*/, SPEventArgs* e) {
        auto* args = dynamic_cast<space::SPBookmarkEventArgs*>(e);
        if (!args) return 0;

        auto* app = BungalowApp::current();
        if (!app || !app->getDatabase()) return 0;
        web::BookmarkManager::toggle(*app->getDatabase(), args->getUri(), args->getTitle());
        return 0;
    }

    // ── Navigation ────────────────────────────────────────────────────────────

    void MainWindow::navigate(const string& uri) const
    {
        string normalized = normalizeScheme(uri);
        this->viewStack->navigate(normalized);
        showCurrentUri(normalized);
        m_sidebar->select(normalized);
    }

    // ── Nav rebuild (called on startup and after sidebar content changes) ─────

    void MainWindow::rebuildNavItems() {
        if (!m_sidebar || !m_features) return;
        m_sidebar->setNodes(m_features->sidebarNodes());
    }

    // ── createElement ─────────────────────────────────────────────────────────

    SPElement* MainWindow::createElement(const string& name, SPElement *parent) {
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

    void MainWindow::onLoad(SPNode *sender, SPEventArgs *args) {
    }

    // ── layout() ─────────────────────────────────────────────────────────────

    void MainWindow::layout() {
        this->clipView = false;

        header = (SPAppHeaderElement *)this->getElementById("header");
        header->setHeight(60);
        header->setPadding({ 5, 25, 5, 5 });
        header->setGap(5);

        backButton = (SPButtonElement *)this->getElementById("backButton");
        backButton->setInnerText("<");
        backButton->setWidth(28);
        backButton->setHeight(28);
        backButton->addEventListener("click", SPCallback(&MainWindow::onBackClicked), this, backButton);

        forwardButton = (SPButtonElement *)this->getElementById("forwardButton");
        forwardButton->setInnerText(">");
        forwardButton->setWidth(28);
        forwardButton->setHeight(28);
        forwardButton->addEventListener("click", SPCallback(&MainWindow::onForwardClicked), this, forwardButton);

        searchElement = (SPInputElement *)this->getElementById("searchInput");
        searchElement->setWidth(120);
        searchElement->setHeight(28);
        searchElement->setColorAttribute("bgcolor", SPColor("#777777"));
        searchElement->addEventListener("enter", reinterpret_cast<SPCallback>(&MainWindow::onSearchEntered), this, this);

        // Address bar: shown (and expanded) when "Full Width" is enabled in settings
        addressElement = (SPInputElement *)this->getElementById("addressInput");
        addressElement->setFlex(1);
        addressElement->setHeight(28);
        addressElement->setColorAttribute("bgcolor", SPColor("#777777"));
        addressElement->addEventListener("enter", reinterpret_cast<SPCallback>(&MainWindow::onAddressEntered), this, this);
        addressElement->hide();

        infoElement = new space::SPInfoElement(this);
        infoElement->setHeight(28);
        infoElement->setId("Info");
        infoElement->hide();

        body = (SPHBoxElement *)this->getElementById("body");
        body->setFlex(1);
        leftSidebar = (SPVBoxElement *)this->getElementById("leftSidebar");

        auto *sidebarSplitter = (SPSplitterElement *)this->getElementById("sidebarSplitter");
        sidebarSplitter->setTarget(leftSidebar);
        sidebarSplitter->setMinWidth(180);
        sidebarSplitter->setMaxWidth(600);

        SPElement *leftTabBar = leftSidebar->getElementById("leftTabBar");
        tabBar = (SPTabBarElement *)leftTabBar;
        treeView = (SPTreeViewElement *)this->getElementById("treeview");

        treeView->addEventListener("itemselected",   SPCallback(&MainWindow::treeview_itemselected), this, treeView);
        treeView->addEventListener("itemcontextmenu", SPCallback(&MainWindow::onNavItemContextMenu),  this, treeView);
        treeView->addEventListener("itemrenamed",     SPCallback(&MainWindow::onNavItemRenamed),      this, treeView);
        treeView->addEventListener("itemmoved",       SPCallback(&MainWindow::onNavItemMoved),        this, treeView);
        treeView->addEventListener("itemdraghovered", SPCallback(&MainWindow::onNavItemDragHovered), this, treeView);
        treeView->addEventListener("mousemove", SPCallback(&MainWindow::onSidebarDragMove), this, treeView);
        treeView->addEventListener("mouseup", SPCallback(&MainWindow::onSidebarDragEnd), this, treeView);
        treeView->setFlex(1);
        m_sidebar = std::make_unique<LeftLibraryMenu>(treeView);

        // Now-playing widget pinned to the bottom of the sidebar
        m_nowPlayingBar = new SPNowPlayingBarElement(leftSidebar);
        leftSidebar->appendChild(m_nowPlayingBar);

        viewStack = (SPViewStackElement *)this->getElementById("viewstack");
        viewStack->setFlex(1);

        // The view stack routes a URI to the last view accepting it, so the
        // catch-all SpaceView goes first and every contributed view wins over it.
        viewStack->appendChild(new views::SpaceView(viewStack));

        // The managers must exist before anything registers into them.
        m_features = std::make_unique<feature::FeatureManager>(viewStack);
        m_search = std::make_unique<search::SearchManager>();
        m_concepts = std::make_unique<concepts::ConceptManager>(concepts::ConceptManager::Host{
            viewStack, m_features.get(), m_search.get(), [this] { rebuildNavItems(); }});
        app::registerBuiltins(*m_features, *m_concepts);
        rebuildNavItems();

        auto *queueSplitter = new SPSplitterElement(body);
        body->appendChild(queueSplitter);
        auto *playQueuePanel = new views::PlayQueueView(body);
        body->appendChild(playQueuePanel);
        m_queueSplitter = queueSplitter;
        m_queuePanel = playQueuePanel;
        viewStack->addEventListener("togglequeue", SPCallback(&MainWindow::onToggleQueue), this, viewStack);

        // Hidden unless the user left it open last run.
        bool showQueue = false;
        {
            std::ifstream in(views::ConfigView::getSettingsPath());
            if (in.is_open()) {
                std::ostringstream ss;
                ss << in.rdbuf();
                try {
                    auto obj = nlohmann::json::parse(ss.str());
                    if (obj.contains("showPlayQueue"))
                        showQueue = obj.at("showPlayQueue").get<bool>();
                } catch (...) {}
            }
        }
        setPlayQueueVisible(showQueue);

        footer = (SPAppFooterElement *)this->getElementById("footer");
        footer->setHeight(32);
        footer->setId("footer");

        this->navigate("bungalow:start");
    }

    MainWindow::MainWindow(SPElement *parent) : SPVBoxElement(parent)
    {
        this->loadXUL("views/app.xml");
        this->layout();
        this->addEventListener("load", SPCallback(&MainWindow::onLoad), this, this);
        this->getWindowElement()->addEventListener("navigate",         static_cast<SPCallback>(&MainWindow::onNavigate),       this, this);
        this->getWindowElement()->addEventListener("bookmarktoggled",  static_cast<SPCallback>(&MainWindow::onBookmarkToggled), this, this);
    }

    int MainWindow::onNavigate(SPObject* /*sender*/, SPEventArgs* args)
    {
        auto *eventArgs = dynamic_cast<SPNavigateEventArgs *>(args);
        if (!eventArgs) return 0;
        this->navigate(eventArgs->getUrl());
        return 0;
    }

    int MainWindow::onSearchEntered(SPObject *, SPEventArgs *eventArgs) const
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

    int MainWindow::onAddressEntered(SPObject *, SPEventArgs *eventArgs) const
    {
        auto *ie = dynamic_cast<SPInputEventArgs*>(eventArgs);
        if (!ie) return 0;
        string uri = ie->getText();
        if (uri.empty()) return 0;
        // Anything that isn't a bungalow URI is treated as a search query
        if (!hasBungalowScheme(uri)) {
            uri = "bungalow:search:" + uri;
        }
        this->navigate(uri);
        return 0;
    }

    void MainWindow::showCurrentUri(const string& uri) const {
        if (addressElement && addressElement->isVisible())
            addressElement->setValue(uri);
    }

    int MainWindow::onBackClicked(SPObject *, SPEventArgs *) {
        if (!viewStack->canGoBack()) return 0;
        viewStack->goBack();
        const string& uri = viewStack->getCurrentUri();
        showCurrentUri(uri);
        m_sidebar->select(uri);
        return 0;
    }

    int MainWindow::onForwardClicked(SPObject *, SPEventArgs *) {
        if (!viewStack->canGoForward()) return 0;
        viewStack->goForward();
        const string& uri = viewStack->getCurrentUri();
        showCurrentUri(uri);
        m_sidebar->select(uri);
        return 0;
    }

    MainWindow::~MainWindow() = default;

    SPViewStackElement *MainWindow::getViewStack() {
        return this->viewStack;
    }

    void MainWindow::setPlayQueueVisible(bool visible) {
        m_queueVisible = visible;
        if (!m_queuePanel || !m_queueSplitter) return;
        if (visible) {
            m_queueSplitter->show();
            m_queuePanel->show();
        } else {
            m_queueSplitter->hide();
            m_queuePanel->hide();
        }
        body->pack();
        body->invalidate();
    }

    int MainWindow::onToggleQueue(SPObject* /*sender*/, SPEventArgs* /*args*/) {
        setPlayQueueVisible(!m_queueVisible);

        // Merge into the settings file so other keys survive.
        nlohmann::json obj = nlohmann::json::object();
        {
            std::ifstream in(views::ConfigView::getSettingsPath());
            if (in.is_open()) {
                std::ostringstream ss;
                ss << in.rdbuf();
                try {
                    auto existing = nlohmann::json::parse(ss.str());
                    if (existing.is_object()) obj = std::move(existing);
                } catch (...) {}
            }
        }
        obj["showPlayQueue"] = m_queueVisible;
        std::ofstream file(views::ConfigView::getSettingsPath());
        if (file.is_open())
            file << obj.dump();
        return 0;
    }

    void MainWindow::setFullWidthAddressBar(bool fullWidth) {
        if (!searchElement || !addressElement) return;
        // The search field keeps its size; the address bar after it takes the
        // remaining width and shows the current URI.
        const string uri = viewStack ? viewStack->getCurrentUri() : string();
        if (fullWidth) {
            addressElement->show();
            addressElement->setValue(uri);
        } else {
            addressElement->hide();
        }
    }
} // namespace ui
} // namespace space
