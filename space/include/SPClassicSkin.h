//
// Created by Alexander Forselius on 2025-12-11.
//

#ifndef SPACIFYCLIENT_SPCLASSICSKIN_H
#define SPACIFYCLIENT_SPCLASSICSKIN_H
#include "SPSkin.h"


namespace space
{
    class SPClassicSkin : public SPSkin
    {
    private:
        SPColor m_lightColor = SPColor(170, 170, 170, 255);
        SPColor m_shadowColor = SPColor(120, 120, 120, 255);
    	bool m_glossy = true;
    	SPButtonStyle m_buttonStyle = kButtonGradient;
    public:
        void setGlossy(bool value) override { m_glossy = value; }
        bool isGlossy() const override { return m_glossy; }
        void setButtonStyle(SPButtonStyle s) override { m_buttonStyle = s; }
        SPButtonStyle getButtonStyle() const override { return m_buttonStyle; }
        bool drawElement(SPGraphicsContext *gc, SPElement *, const string& state, const sp_rectangle& bounds) override;
        void drawTableRow(SPGraphicsContext* g, SPTableElement* table, int i, int x, int y, int width, int height, int row) override;
        void drawTableHeader(SPGraphicsContext* g, SPTableElement* table, int x, int y, int width, int height) override;
        void drawTabBarTab(SPGraphicsContext* g, SPTabBarElement *tabBar, SPTab *tab, int index, bool active, int x, int y, int width, int height) override;
        void drawTabBarToolButton(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTabToolButton* btn, int x, int y, int width, int height) override;
        void drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar, int x, int y, int width, int height) override;
        void drawTreeItem(SPGraphicsContext* g, SPTreeViewElement *treeView, SPTreeItem* item, int itemHeight, int x, int y, int width, int height) override;
        void drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView, const string& title, int x, int y, int width, int height) override;

    	SPClassicSkin();

    };
}

#endif //SPACIFYCLIENT_SPCLASSICSKIN_H