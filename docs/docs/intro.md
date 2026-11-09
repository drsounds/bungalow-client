---
slug: /
title: Introduction
sidebar_position: 0
---

# Bungalow

Bungalow is a native **C++ music and media manager** meant to work as a small
**plugin operating system for music**. Spotify and Apple Music bundle discovery,
catalogue lookup, purchase, streaming, listening history and the whole UI into
one closed product. Bungalow splits each of those into a separate contribution
that a feature, concept or app bundle can provide, replace or extend.

It is built on **Space**, a lightweight UI framework written for the project.
Space is a platform-neutral element tree, a XUL-like markup language, Lua
templates and a skinning system, and it runs on wxWidgets, plain Win32 or WTL.

```
        Spotify / Apple Music                         Bungalow
   ┌──────────────────────────────┐      ┌──────────────────────────────────┐
   │  discovery                   │      │  discovery   → search providers, catalogues
   │  lookup / resolution         │      │  resolution  → MusicService + PlaybackCoordinator
   │  purchase / store            │      │  purchase    → store & curator views
   │  streaming                   │      │  streaming   → MediaService
   │  listening / queue           │      │  listening   → play queue
   │  the entire UI               │      │  UI          → features, concepts, skins, apps
   │  ── all welded together ──   │      │  ── each a separate contribution ──
   └──────────────────────────────┘      └──────────────────────────────────┘
          one vendor, locked                  many vendors, composable
```

:::note Naming
*Bungalow* is the product name. Its code still contains older names: the
framework classes have an `SP` prefix (`SPElement`, `SPSkin`, …), app files are
served from `sp://`, and navigation uses `bungalow:` URIs. These are technical
identifiers and are left as they are.
:::

## Two clients, one architecture

There are two implementations:

| | C++ client (this repository) | Java client |
|---|---|---|
| Repository | [drsounds/bungalow-client](https://github.com/drsounds/bungalow-client) | [drsounds/bungalow](https://github.com/drsounds/bungalow) |
| UI | Space (wxWidgets / Win32 / WTL) | Swing |
| Templates | Lua (via the Lua C API) | Lua (via LuaJ) |
| Embedded web | CEF on WTL, wxWebView on wxWidgets | JCEF |
| Role | the native client under active development | the reference design for the package structure |

The C++ client follows the **Java client's package structure**: `aspect`,
`feature`, `concept`, `search`, `navigation`, `ui`, `web` and `app` exist in
both, with the same responsibilities. The Java client's own documentation is
included in the [Java reference client](./java-client/index.md) section.

## The Space UI idea

Most apps share the same archetype: **features**, **entities** and
**plugins**. Even so, each app re-implements that structure and ships it in web
or app bundles that take gigabytes of storage and a lot of RAM.

Space puts those archetypes into the framework. An app developer describes
*concepts* (a feature with views, sidebar entries and entities) instead of
building toolbars and menus from scratch. The goal is a cross-platform
application stack in which an app can be declared in markup, for example:

```xml
<app>
  <entities />
  <features>
    <feature id="start">
      <view>
        <section id="overview">
          <text>Hello World</text>
        </section>
      </view>
    </feature>
  </features>
</app>
```

or in YAML:

```yaml
app:
  title: Test
  features:
    start:
      view:
        section:
          id: overview
          text: Hello World
```

That declarative `<app>` format is the direction of the project, not something
you can use yet. What is implemented today is described in the
[architecture](./architecture/overview.md) and [Space](./space/overview.md)
sections.

## Where to start

1. [Background](./background.md): why Bungalow exists.
2. [Building](./getting-started/building.md): dependencies, UI backends, CMake presets.
3. [Project layout](./getting-started/project-layout.md): what is in each directory.
4. [Architecture overview](./architecture/overview.md): the layers, startup and package map.
5. [Space UI framework](./space/overview.md): elements, XUL, templates, skins and backends.
6. [Apps](./apps/bundles.md): app bundles, native Lua apps and HTML5 apps.
7. [Specifications](./specs/index.md): the `musik:` URI scheme and decentralised playlist links.
8. [Roadmap](./roadmap.md): what works today and what comes next.
