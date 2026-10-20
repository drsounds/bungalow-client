package se.bungalow.feature;

import se.bungalow.aspect.Aspect;
import se.bungalow.navigation.View;
import se.bungalow.navigation.SidebarNode;

import java.util.Collections;
import java.util.List;

/**
 * Base class for app features that contribute views and sidebar navigation.
 * Features are registered with ServiceManager alongside Services.
 */
public abstract class Feature implements Aspect {
    public abstract String getId();

    public abstract String getName();

    /** SPView instances to register with the view stack at activation time. */
    public List<View> getViews() { return Collections.emptyList(); }

    /** SidebarNode trees to append to the sidebar at activation time. */
    public List<SidebarNode> getSidebarNodes() { return Collections.emptyList(); }

    /** Called once when this feature is activated via ServiceManager. */
    public void onRegister(FeatureManager featureManager) {}
}
