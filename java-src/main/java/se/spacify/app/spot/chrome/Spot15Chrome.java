package se.spacify.app.spot.chrome;

import java.awt.BorderLayout;

import java.awt.Dimension;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.controls.Panel;
import se.spacify.controls.SplitPane;
import se.spacify.app.spot.controls.Spot09AppFooter;
import se.spacify.app.spot.controls.Spot09AppHeader;

import se.spacify.ui.chrome.Chrome;

import se.spacify.ui.LeftLibraryMenu;
import se.spacify.ui.LeftMenuPanel;
import se.spacify.ui.NowPlayingPanel;

public class Spot15Chrome extends Chrome {

        @Override
        public String getId() {
                // TODO Auto-generated method stub
                return "spot15";
        }
        @Override
        public void onRegister(AspectManager<? extends Aspect> aspectManager) {
                // TODO Auto-generated method stub
        }
        @Override
        public void build() {
                super.build();
                getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.PAGE_AXIS));
                
                appHeader = new Spot09AppHeader(viewStack);
                appHeader.getComponent().setMaximumSize(new Dimension(Short.MAX_VALUE, 28));
                appHeader.getComponent().setMinimumSize(new Dimension(0, 28));
                add(appHeader);
                leftMenuPanel = new LeftMenuPanel();
                leftMenuPanel.getComponent().setMinimumSize(new Dimension(100, 0));
                leftMenuPanel.getComponent().setMaximumSize(new Dimension(100, Short.MAX_VALUE));
                leftMenuPanel.getComponent().setPreferredSize(new Dimension(100, Short.MAX_VALUE));
                leftMenuPanel.getComponent().setOpaque(false);
                appPanel = new Panel();

                add(leftMenuPanel);
                leftMenuPanel.setVisible(false);
                add(appPanel);
                appPanel.getComponent().setOpaque(false);
                leftMenuPanel.getComponent().setLayout(new BoxLayout(leftMenuPanel.getComponent(), BoxLayout.PAGE_AXIS));
                appPanel.getComponent().setLayout(new BoxLayout(appPanel.getComponent(), BoxLayout.PAGE_AXIS));

                // Reuse the shared sidebar (owned by MainWindow, populated by plugins) if set.
                if (leftLibraryMenu == null) leftLibraryMenu = new LeftLibraryMenu(viewStack);

                centerPanel = new Panel(new BorderLayout());
                centerPanel.add(viewStack);

                leftSplit = new SplitPane(SplitPane.HORIZONTAL_SPLIT, leftLibraryMenu, centerPanel);
                leftSplit.getComponent().setDividerLocation(220);
                leftSplit.getComponent().setDividerSize(6);
                // Keep the leftLibraryMenu at its width and let the centre view absorb resizes.
                leftSplit.getComponent().setResizeWeight(0.0);
                // Empty (non-UIResource) border survives the Nimbus reinstall in
                // rebuildTheme(); a null border would get a default border re-installed.
                leftSplit.getComponent().setBorder(BorderFactory.createEmptyBorder());
                leftSplit.getComponent().setContinuousLayout(true);

                mainSplit = new SplitPane(SplitPane.HORIZONTAL_SPLIT, leftSplit, new NowPlayingPanel(viewStack));
                mainSplit.getComponent().setDividerLocation(880);
                mainSplit.getComponent().setDividerSize(6);
                // Give all extra width to the left (leftLibraryMenu + centre); the right
                // Now Playing / queue panel keeps its width as the window resizes.
                mainSplit.getComponent().setResizeWeight(1.0);
                mainSplit.getComponent().setBorder(BorderFactory.createEmptyBorder());
                mainSplit.getComponent().setContinuousLayout(true);

                appPanel.add(mainSplit, BorderLayout.CENTER);

                appFooter = new Spot09AppFooter();
                appPanel.add(appFooter);
                appFooter.getComponent().setMaximumSize(new Dimension(Short.MAX_VALUE, 18));
                appFooter.getComponent().setMinimumSize(new Dimension(0, 18));

        }
}
