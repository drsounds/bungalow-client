package se.bungalow.concept;

import javax.swing.Icon;

import se.bungalow.aspect.Aspect;
import se.bungalow.app.Application;

public interface Concept extends Aspect {
    public void onActivate(ConceptContext ctx);
    /** Release any resources acquired in {@link #onActivate}. */
    public void onDeactivate();

    /** Icon shown in the plugin manager; null falls back to a default. */
    public Icon getIcon();
    public Application getApplication();
}
