---
layout: default
title: Status & Roadmap
nav_order: 10
---

# Status & roadmap

These docs describe the architecture as designed. The codebase is mid-way
through a refactor toward the Concept/Aspect model, so a few seams are visible.
This page is the honest map of what's wired, what isn't, and where it's going.

## What works today

- **Plugin discovery, activation, and clean removal** — built-ins +
  `~/Bungalow` + `<app>/plugins`, each jar isolated, contributions recorded and
  undoable.
- **Aspect-based Service selection** — `getServices(MediaService.class)` etc.;
  local + YouTube media services, the MusicBrainz catalogue service.
- **Navigation** — `ViewStack` + the `spacify:` URI space + the sidebar.
- **Spider templates** — the Lua-preprocessed XUL template engine renders view
  content into a control tree, reconciles re-renders (React-style), and posts back
  on interaction; demoed by the `spacify:testapp` plugin. See
  [Spider templates](spider-templates.html).
- **Playback & "Play with…" resolution** — queue, cross-service resolver,
  persisted picks, active-service switching.
- **Runtime look-and-feel** — Designs (Chrome + Skin) selectable in Settings and
  persisted; app-wide tint via the Taste → ThemeManager bridge.

## Known gaps / seams (in-progress refactor)

### 1. Concept activation — ✅ wired

`PluginManager.Recorder` implements `ConceptContext`, so
`registerConcept(c)` registers the Concept **and** calls
`c.onActivate(this)`; the views/services/sidebar the Concept contributes are
recorded against the owning plugin and removed on `undo()` (which also calls
`Concept.onDeactivate()`). This mirrors how `registerFeature` wires a Feature.

Concretely: `LibraryPlugin.onActivate` does
`ctx.registerConcept(new LibraryConcept(this))`, and `LibraryConcept.onActivate`
now runs at activation, registering the tracks/recordings/releases/artists views
and the "Your Library" sidebar. (Verified: navigating to
`spacify:library:tracks` resolves.)

The migration toward "a plugin registers one Concept and the Concept contributes
everything" is the standard plugin shape going forward; `MediaConcept`,
`SearchConcept`, `PlaylistConcept` and `WebConcept` follow it.

### 2. Two colour holders: `Taste` and `ThemeManager`

`Taste` (per-window model) and `ThemeManager` (global live palette) hold
overlapping state. They're now bridged (Taste setters forward into ThemeManager),
which fixed app-wide tinting, but it's still duplicated state. A future cleanup
could make `ThemeManager` read straight from the active `Taste` (single source of
truth) and drop the mirrored static fields.

### 3. `MainWindow` vs `Chrome` ownership overlap

Some shell pieces exist on both `MainWindow` and `Chrome` (footer, now-playing,
sidebar). The sidebar is intentionally owned by `MainWindow` and embedded by the
Chrome; the footer now delegates to the Chrome. The remaining duplicated fields
are harmless but worth consolidating onto the Chrome.

### 4. Boilerplate `onRegister` / TODO stubs

Many aspects carry empty `onRegister(...)` bodies and `// TODO Auto-generated`
stubs from extraction. Harmless, but a signal of where the extraction is still
settling.

## Roadmap — toward the open music OS

The architecture is the foundation for a vendor-neutral ecosystem. Natural next
steps, in rough priority:

1. **Migrate the remaining plugins onto Concepts** (the activation plumbing is
   done) so "a plugin registers one Concept and the Concept contributes
   everything" is the uniform plugin shape.
2. **A purchase/store aspect** — formalise buying as a Service capability +
   `AuthAspect`, beyond today's web-view storefronts, so multiple stores compete
   for the same track the way streamers already do via "Play with…".
3. **More catalogue & streaming back-ends as external jars** — Discogs, Bandcamp,
   Subsonic/Jellyfin, local network shares — proving the `~/Bungalow` drop-in path.
4. **Portable resolution** — `MusicServiceTrack` picks today are local; an
   export/sync format would make "this track plays from here" portable between
   machines and users.
5. **A plugin SDK + manifest spec** — document the jar manifest headers and
   publish the contribution interfaces as a stable API so third parties can build
   against it.
6. **Design marketplace** — Chromes/Skins/Designs (incl. a true Frutiger-Aero
   glass Design) distributed and switched at runtime.

## Conventions for contributors

- Everything pluggable implements `Aspect` and is registered through a manager.
- Plugins contribute **only** through the `PluginContext` so removal stays clean.
- Views are reached by `spacify:` URI, never by direct reference.
- Callers select back-ends **by capability** (`getServices(SomeAspect.class)`),
  never by vendor id.
- Keep new comments at the altitude of the surrounding code; document the *why*.
