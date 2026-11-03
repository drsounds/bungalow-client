#ifndef BUNGALOW_UI_MAINWINDOW_H
#define BUNGALOW_UI_MAINWINDOW_H
#include "SPVBoxElement.h"

#include "SPMessageType.h"

#include "libspace.h"

#include <memory>

namespace space {
    class SPViewStackElement;
    class SPTreeViewElement;
    class SPInfoElement;
    class SPTextElement;
    class SPEventArgs;
    class SPAppFooterElement;
    class SPAppHeaderElement;
    class SPButtonElement;
    class SPHBoxElement;
    class SPTabBarElement;
    class SPInputElement;
    class SPNowPlayingBarElement;

    namespace concepts { class ConceptManager; }
    namespace feature { class FeatureManager; }
    namespace search { class SearchManager; }

    namespace ui {
    class LeftLibraryMenu;

    /// The application shell: header, sidebar, view stack and footer. Owns
    /// one manager per kind of contribution; the built-in concepts and
    /// features (see app/BuiltinRegistry.h) register into them at startup.
    class MainWindow : public SPVBoxElement
    {
        protected:
            SPTextElement *m_trackLabel = nullptr;
            SPInputElement *searchElement;
            SPInputElement *addressElement = nullptr;
            SPViewStackElement *viewStack = nullptr;
            SPTreeViewElement *treeView = nullptr;
            SPInfoElement *infoElement;
            SPAppFooterElement *footer;
            SPAppHeaderElement *header;
            SPHBoxElement *body;
            SPVBoxElement *leftSidebar;
            SPTabBarElement *tabBar;
            SPButtonElement *backButton;
            SPButtonElement *forwardButton;
            SPButtonElement *accountButton;
            SPNowPlayingBarElement* m_nowPlayingBar = nullptr;
            SPElement* m_queueSplitter = nullptr;
            SPElement* m_queuePanel = nullptr;
            bool m_queueVisible = false;

            // Declaration order matters: concepts are deactivated first.
            std::unique_ptr<LeftLibraryMenu> m_sidebar;
            std::unique_ptr<feature::FeatureManager> m_features;
            std::unique_ptr<search::SearchManager> m_search;
            std::unique_ptr<concepts::ConceptManager> m_concepts;

        public:
            void navigate(const string& uri) const;
            void hideMessage();
            int treeview_itemselected(SPObject* sender, SPEventArgs* e);
            int onNavItemContextMenu(SPObject* sender, SPEventArgs* e);
            int onNavItemRenamed(SPObject* sender, SPEventArgs* e);
            int onNavItemMoved(SPObject* sender, SPEventArgs* e);
            MainWindow();
            void onLoad(SPNode *sender, SPEventArgs *args);
            virtual SPElement* createElement(const string&, SPElement*) override;

            string getType() override
            {
                return "main";
            }
            int onNavigate(SPObject* sender, SPEventArgs* args);
            int onBackClicked(SPObject* sender, SPEventArgs* args);
            int onForwardClicked(SPObject* sender, SPEventArgs* args);
            int onBookmarkToggled(SPObject* sender, SPEventArgs* args);
            explicit MainWindow(SPElement *parent);
            int onSearchEntered(SPObject* sender, SPEventArgs* eventArgs) const;
            int onAddressEntered(SPObject* sender, SPEventArgs* eventArgs) const;
            void layout();
            ~MainWindow() override;
            SPViewStackElement *getViewStack();

            feature::FeatureManager& getFeatureManager() { return *m_features; }
            search::SearchManager& getSearchManager() { return *m_search; }
            concepts::ConceptManager& getConceptManager() { return *m_concepts; }

            void showMessage(SPMessageType msgType, string msg);
            void setFullWidthAddressBar(bool fullWidth);
            // Shows or hides the play queue panel (and its splitter).
            void setPlayQueueVisible(bool visible);
            int onToggleQueue(SPObject* sender, SPEventArgs* args);
            // Rebuilds the sidebar from the registered features' current nodes.
            void rebuildNavItems();

        private:
            // Mirrors the current URI into the address bar, or into the search
            // field when the address bar is hidden.
            void showCurrentUri(const string& uri) const;
    };
    } // namespace ui
}
#include "libspace.h"

#endif // BUNGALOW_UI_MAINWINDOW_H
