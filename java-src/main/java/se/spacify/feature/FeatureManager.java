package se.spacify.feature;

import java.util.Collection;
import java.util.Collections;
import javax.swing.tree.DefaultMutableTreeNode;

import se.spacify.aspect.BaseAspectManager;
import se.spacify.navigation.ViewStack;
import se.spacify.navigation.SidebarNode;
import se.spacify.ui.MainWindow;

public class FeatureManager extends BaseAspectManager<Feature> {
    
    public FeatureManager(MainWindow mainWindow) {
        super(mainWindow);
    }
 
    public void register(Feature feature) {
        super.register(feature);
        feature.onRegister(this);
    }

    /** Drop a feature from the registry (its views/nodes are removed by the caller). */
    public void unregister(Feature feature) {
        super.unregister(feature);
    }

    /** Drop a feature from the registry (its views/nodes are removed by the caller). */
    public void unregister(String featureId) {
        super.unregister(featureId);
    }

    /**
     * Wire all registered features into the running UI.
     * Call this after MainWindow has been built and the SPViewStack is live.
     *
     * @param viewStack     the app's main view stack
     * @param sidebarRoot   root node of the sidebar JTree model
     */
    public void activateFeatures(ViewStack viewStack, DefaultMutableTreeNode sidebarRoot) {
        for (Feature f : getNodes().values()) {
            f.getViews().forEach(viewStack::registerView);
            for (SidebarNode sn : f.getSidebarNodes()) {
                sidebarRoot.add(buildTreeNode(sn));
            }
        }
    }

    private static DefaultMutableTreeNode buildTreeNode(SidebarNode sn) {
        DefaultMutableTreeNode node = new DefaultMutableTreeNode(sn);
        for (SidebarNode child : sn.getChildren()) node.add(buildTreeNode(child));
        return node;
    }

    public Collection<Feature> all() { return Collections.unmodifiableCollection(getNodes().values()); }

    @Override
    public Feature get(String featureId) {
        return getNodes().get(featureId);
    }
}
