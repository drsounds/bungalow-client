---
layout: default
title: Bungalow Architecture Documentation
nav_title: Home
nav_order: 0
---

# Bungalow Architecture Documentation

Bungalow is a **Java / Swing desktop media player** built as a small **plugin
operating system for music**. Where Spotify or Apple Music bundle *discovery,
catalogue lookup, purchase, streaming, listening history and the entire UI* into
one closed vertical, Bungalow breaks each of those into an independently
swappable contribution that a plugin can provide, replace, or extend.

The goal is an **open, Frutiger-Aero–inspired music ecosystem**: the app is the
"OS", and the catalogues, stores, streaming back-ends, playlist providers and
even the whole look-and-feel are "apps" that plug in.

> **Naming.** *Bungalow* is the product/brand name. *Bungalow* is the internal
> **code name** — you'll still see it throughout the source (the `se.bungalow.*`
> packages, the `bungalow:` navigation URIs, the `~/.bungalow/` config directory,
> the `BungalowApp` entry point). Those are technical identifiers and are left
> as-is; only the brand wording in these docs is "Bungalow".

```
        Spotify / Apple Music                         Bungalow
   ┌──────────────────────────────┐      ┌──────────────────────────────────┐
   │  discovery                   │      │  discovery   → MusicCatalogueService
   │  lookup / resolution         │      │  resolution  → MusicService + PlaybackCoordinator
   │  purchase / store            │      │  purchase    → store views + AuthAspect
   │  streaming                   │      │  streaming   → MediaService
   │  listening / queue           │      │  listening   → PlayQueue
   │  the entire UI               │      │  UI          → View / Chrome / Skin / Design
   │  ── all welded together ──   │      │  ── each a separate plugin/aspect ──
   └──────────────────────────────┘      └──────────────────────────────────┘
          one vendor, locked                  many vendors, composable
```

## Read in this order

0. [Introduction & background](introduction.html) — *why* this exists: the monolith
   problem, streaming economics, gatekeeping, vendor lock-in, the UX regression,
   and the Longhorn / Windows Media Player road not taken.
1. [Architecture overview](architecture.html) — the layers, the boot sequence, and
   how a play request flows end to end.
2. [The plugin system](plugin-system.html) — how plugins are discovered, activated,
   and cleanly removed; how to write one.
3. [Aspects & Services](aspects-and-services.html) — the capability model that
   replaces the monolith: `MediaService`, `MusicService`,
   `MusicCatalogueService`, `AuthAspect`, …
4. [Navigation & Views](navigation-and-views.html) — the `bungalow:` URI space, the
   `ViewStack`, and the sidebar.
5. [Spider templates](spider-templates.html) — the ASP/JSP-style Lua template engine
   that renders view content into a control tree and posts back on interaction.
6. [Look & feel: Design = Chrome + Skin](look-and-feel.html) — themeable, skinnable,
   re-chromeable UI selected at runtime.
7. [Playback & resolution](playback-and-resolution.html) — the play queue and the
   "Play with…" cross-service resolver.
8. [The `musik:` URI scheme](musik-uri-scheme.html) — the portable, service-agnostic
   way content is named (recordings, works, releases, artists, facets); formal
   spec in [RFC-0002](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/bungalow/app/music/spec/RFC-0002-musik-uri-scheme.md).
9. [Status & roadmap](status-and-roadmap.html) — what is wired today, the known
   gaps in the in-progress refactor, and where the ecosystem is headed.

## Vocabulary at a glance

| Term | What it is | Key type |
|------|------------|----------|
| **Plugin** | A unit of distribution that contributes everything below. | `se.bungalow.plugin.Plugin` |
| **Aspect** | A capability interface (identity + `onRegister`). Everything pluggable is an Aspect. | `se.bungalow.aspect.Aspect` |
| **Service** | A back-end with a lifecycle; takes on capability aspects (streaming, discovery, auth). | `se.bungalow.service.Service` |
| **Concept** | A coarse feature domain a plugin activates (library, search, playlists). | `se.bungalow.concept.Concept` |
| **Feature** | A bundle of views + sidebar nodes registered as a unit. | `se.bungalow.feature.Feature` |
| **View** | A screen bound to a `bungalow:` URI. | `se.bungalow.navigation.View` |
| **Spider** | A template engine: Lua-preprocessed XUL markup → a control tree, with postbacks. | `se.bungalow.app.spider.Spider` |
| **Design** | A premix of a **Chrome** (window shell/layout) and a **Skin** (control painting). | `se.bungalow.design.Design` |
| **Theme / Taste** | Colour/tint model; `Taste` is the per-window state, `ThemeManager` the live palette. | `se.bungalow.ui.theme.*` |

> These docs describe the code as it currently stands. The codebase is mid-way
> through a refactor toward the Concept/Aspect model; see
> [Status & roadmap](status-and-roadmap.html) for what is and isn't wired yet.
