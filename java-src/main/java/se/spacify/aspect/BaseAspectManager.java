package se.spacify.aspect;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

import se.spacify.ui.MainWindow;

public abstract class BaseAspectManager<T extends Aspect> implements AspectManager<T> {
    protected MainWindow mainWindow;
    // Mutable, insertion-ordered: register() puts into this map. An immutable
    // emptyMap() here would make every registration throw.
    private Map<String, T> nodes = new LinkedHashMap<>();
    public MainWindow getMainWindow() {
        return mainWindow;
    }
    public Map<String, T> getNodes() {
        return nodes;
    }
    public BaseAspectManager(MainWindow mainWindow) {
        this.mainWindow = mainWindow;
    }
 
    public void register(T concept) {
        nodes.put(concept.getId(), concept);
        concept.onRegister(this);
    }

    /** Drop a concept from the registry (its views/nodes are removed by the caller). */
    public void unregister(T concept) {
        nodes.remove(concept.getId());
    }

    /** Drop a concept from the registry (its views/nodes are removed by the caller). */
    public void unregister(String conceptId) {
        nodes.remove(conceptId);
    }
 
    public Collection<T> all() { return Collections.unmodifiableCollection(nodes.values()); }

    @Override
    public T get(String conceptId) {
        return nodes.get(conceptId);
    }    
}
