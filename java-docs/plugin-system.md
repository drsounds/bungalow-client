---
layout: default
title: The Plugin System
nav_order: 3
---

# The plugin system

A **plugin** is the unit of distribution. It contributes any mix of Services,
Concepts, Features, Views, sidebar nodes, and look-and-feel pieces (Chrome /
Skin / Design / Theme) — and everything it contributes can be cleanly removed
again when it is disabled or uninstalled.

Key types live in [`se.spacify.plugin`](https://github.com/drsounds/bungalow/tree/main/src/main/java/se/spacify/plugin):
`Plugin`, `PluginManager`, `PluginLoader`, `PluginContext`, `PluginDescriptor`,
`PluginManifest`, `PluginPaths`, `PluginSettings`, `BuiltinPluginRegistry`.

## The `Plugin` entry point

```java
public abstract class Plugin implements Aspect {
    PluginManager manager;                 // injected at load time
    public void onActivate(PluginContext ctx) {}   // contribute here
    public void onDeactivate() {}                   // release resources
    public Icon getIcon() { return null; }
    public List<PluginSetting> getSettingsSchema() { return List.of(); }
}
```

A minimal plugin:

```java
public class MyPlugin extends Plugin {
    @Override public String getId()   { return "myplugin"; }
    @Override public String getName() { return "My Plugin"; }

    @Override public void onActivate(PluginContext ctx) {
        ctx.registerService(new MyStreamingService());   // a back-end
        ctx.registerView(new MyView(ctx.viewStack()));   // a screen
        ctx.addSidebarNode(new SidebarNode("My Thing", "spacify:mything"));
    }
}
```

## Discovery: three sources

[`PluginLoader.discover()`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/PluginLoader.java)
returns descriptors from three places, built-ins first:

| Source | Location | Class loader | Removable? |
|--------|----------|--------------|------------|
| `BUILTIN_BUNDLE` | compiled into the app, listed in `BuiltinPluginRegistry` | app class loader | no (disable only) |
| `APP_DIR` | `<app>/plugins/*.jar` (beside the executable) | isolated `URLClassLoader` | no |
| `EXTERNAL` | `~/Bungalow/*.jar` (user-installed) | isolated `URLClassLoader` | yes |

Jar plugins declare their identity + main class in `META-INF/MANIFEST.MF`
(parsed by `PluginManifest`); built-ins declare it in
[`BuiltinPluginRegistry`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/BuiltinPluginRegistry.java).
Each jar gets its **own** `URLClassLoader` (parent = the app loader) so plugins
are isolated from one another.

The built-in set today (in load order):

```
wmp · spot           ← look-and-feel (Designs/Chromes/Skins/Themes), enabled by default
localmusic           ← local file MediaService
library              ← the library views + "Your Library" sidebar
web                  ← embedded browser / store views
musicbrainz          ← a MusicCatalogueService (discovery)
catalog              ← the "Catalogs" sidebar built from every MusicCatalogueService
youtube              ← a streaming MediaService (resolution fallback)
testapp              ← hello-world demo of the Spider template engine (spacify:testapp)
```

Order matters where one plugin consumes another's contributions — e.g. `catalog`
loads after `musicbrainz` so the catalogue Services exist when it builds the
"Catalogs" folder.

## Lifecycle & the recording context

`PluginManager.start()` discovers all plugins and activates the enabled ones.
Enable/disable state persists to `~/.spacify/plugins/state.properties`; a plugin
defaults to enabled.

Activation hands the plugin a [`PluginContext`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/PluginContext.java).
The concrete implementation is `PluginManager.Recorder`, which **records every
contribution** as it is made:

```java
ctx.registerService(s);   // → ServiceManager.register(s) + remembered
ctx.registerConcept(c);   // → ConceptManager.register(c) + remembered
ctx.registerView(v);      // → ViewStack.registerView(v)  + remembered
ctx.addSidebarNode(n);    // → sidebar subtree            + remembered (returns a SidebarHandle)
ctx.registerSkin(s);      // → SkinManager.register(s)     + remembered
ctx.registerChrome(c);    // → ChromeManager + remembered
ctx.registerDesign(d);    // → DesignManager + remembered
ctx.registerTheme(t);     // → ThemeManager  + remembered
ctx.registerFeature(f);   // → FeatureManager + auto-registers the feature's views & nodes
```

On disable/uninstall the recorder replays the registrations **in reverse**,
removing every view, node, service and skin the plugin added — so a removed
plugin leaves no trace. The injected `manager` reference lets a plugin reach the
live app (`getManager().getMainWindow()…`) from `onActivate` onward.

`SidebarHandle` (returned by `addSidebarNode`) lets a plugin manage **dynamic**
sidebar children after activation — e.g. the Library concept keeps the
Releases/Artists lists live as the database changes.

## Settings

A plugin returns a typed schema from `getSettingsSchema()` (a list of
`PluginSetting`s). `PluginSettings` persists the values per plugin to
`~/.spacify/plugins/<id>.properties`, and the plugin-manager UI renders an editor
from the schema. The values are reachable at runtime via `ctx.settings()`.

## Writing & installing a plugin

1. Implement `Plugin`, contribute via `onActivate(PluginContext)`.
2. For a jar: declare id / name / version / main-class in the manifest, drop the
   jar in `~/Bungalow/` (or `<app>/plugins/`). `PluginManager.install(jar)` copies
   an external jar into `~/Bungalow` and activates it immediately.
3. For a built-in: add a `builtin(...)` line to `BuiltinPluginRegistry`.

`registerConcept(c)` also calls `c.onActivate(ConceptContext)` (the recorder
implements `ConceptContext`), so a Concept-based plugin's contributions are wired
and recorded for clean teardown just like a Feature's. See
[Status & roadmap](status-and-roadmap.html) for the migration state.
