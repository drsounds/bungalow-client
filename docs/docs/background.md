---
title: Background
sidebar_position: 1
---

# Background

> *Why build a desktop music player as a plugin operating system, styled after
> software that was cancelled twenty years ago?*

Bungalow argues that the usual shape of music software today, where one company
owns discovery, the catalogue, browsing, your library, the stream and the
checkout, is a business decision and not a technical necessity. That decision
has costs for listeners, for independent artists and for discovery. Bungalow
tries to show that the composable, open "platform" model the industry was
heading toward in the mid-2000s can still be built.

The full argument, with sources, is in the Java client's
[introduction](https://github.com/drsounds/bungalow/blob/main/docs/introduction.md).
This page is a summary.

## The monolith problem

A streaming service combines six jobs into one product:

| Job | What it should be | What the monolith makes it |
|-----|-------------------|----------------------------|
| **Discovery** | finding new music, from anywhere | an algorithm tuned to retention, not taste |
| **Catalogue** | the universe of recordings | only what *this* vendor licensed |
| **Browse** | navigating that universe | a walled garden you can't leave |
| **Library** | *your* collection | a list of pointers that vanish when you cancel |
| **Streaming** | playing the bytes | locked to one back-end |
| **Purchase** | owning/supporting | mostly removed, or a separate silo |

Because these are fused, you cannot keep your library and change the streaming
back-end, or browse one service and buy from another. The bundle is the
lock-in. Bungalow separates each job into its own contribution.

## Economics and gatekeeping

Pro-rata streaming pays fractions of a cent per play, and payouts go mostly to
whoever already has scale. Editorial and algorithmic playlists have become the
new radio, and so the new gatekeepers. When one company controls discovery, the
catalogue *and* the payouts, it has a built-in conflict of interest.

Bungalow can't change royalty rates. It can separate *where you discover and
own* from *who you pay*. That leaves room for direct-support and purchase models
next to streaming, and lets any number of catalogues and curators sit side by
side.

## Vendor lock-in

When your library and playlists live inside one company's account, leaving
means losing them. Bungalow keeps your library in a local SQLite database that
you own, described with vendor-neutral identifiers (ISRC for recordings, UPC for
releases, ISNI for artists). Back-ends can be swapped underneath it.

## Lightweight software

The music clients of around 2009 were small native programs that opened
instantly. Over the next decade most of them were rebuilt on embedded-browser
shells and became heavier, slower and more promotional.

Bungalow is a **native C++ application** with its own small rendering layer
(Space). It starts quickly, uses little memory and treats the player as a tool
rather than a storefront. It still embeds a browser (CEF), but only for web
sites and HTML5 apps that need one, not for the whole UI.

## The road not taken: Longhorn and Windows Media Player

Around 2003–2006, Microsoft's **Longhorn** project described a different
future: **WinFS**, where your media was queryable data you owned; the glassy
**Aero** visual style, now called **Frutiger Aero**; and **Windows Media Player**
as a platform whose music stores (URGE, PlaysForSure) plugged *into* the player.

That model, a neutral player and a library you own with stores and catalogues as
plugins, is close to what Bungalow implements. Bungalow's look follows it too:
it ships a [Frutiger Aero skin](./space/skins.md) with HSL tinting.

## What Bungalow proposes

- **Discovery is pluggable** (search providers, catalogues, curators), so there is no single gatekeeper.
- **Streaming is pluggable** (`SPMediaService`), so there is no back-end lock-in.
- **The library is local and vendor-neutral**, so cancelling a service doesn't erase your collection.
- **Purchase sits next to streaming** (store views), reviving the PlaysForSure marketplace idea.
- **The look is pluggable** (skins, including skins shipped in app bundles).
- **Apps are first-class**: native Lua apps and sandboxed HTML5 apps extend the client.

## Scope

This is a research and hobby project, not a competitor to a streaming service.
What it can do is show, in working code, that the closed monolith is a choice
and that its parts can be separated.
