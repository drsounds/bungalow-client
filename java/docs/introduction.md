---
layout: default
title: Introduction & Background
nav_order: 1
---

# Introduction & Background

> *Why build a desktop music player in the 2020s, as a plugin operating system,
> styled after software that was cancelled twenty years ago?*

Bungalow is a thesis in the form of an application. It argues that the dominant
shape of music software today — one company owning discovery, the catalogue,
browsing, your library, the stream, and the checkout, all welded into a single
account — is not a law of nature. It is a business decision, and it has costs:
for listeners (worse software), for independent artists (worse economics), and
for the medium itself (worse discovery). Bungalow is an existence proof that the
*other* model — the composable, open, "platform" model the industry was actually
heading toward in the mid-2000s — can still be built.

This page is the **background and motivation**. The rest of the
[docs](./) describe how the architecture delivers on it.

---

## 1. The monolith problem

A streaming service fuses six different jobs into one inseparable product:

| Job | What it should be | What the monolith makes it |
|-----|-------------------|----------------------------|
| **Discovery** | finding new music, from anywhere | an algorithm tuned to retention, not taste |
| **Catalogue** | the universe of recordings | only what *this* vendor licensed |
| **Browse** | navigating that universe | a walled garden you can't leave |
| **Library** | *your* collection | a list of pointers that vanish when you cancel |
| **Streaming** | playing the bytes | locked to one back-end |
| **Purchase** | owning/supporting | mostly removed, or a separate silo |

Because they're fused, you cannot keep your library and swap the streaming
back-end. You cannot use a better discovery tool against the same catalogue. You
cannot browse one service and buy from another. The bundle is the lock-in.

Bungalow **unbundles every one of these into a separate, swappable contribution**
— a plugin or a capability *aspect* — so the same library row can be discovered
in one place, resolved to whichever back-end can play it, and purchased from a
third. See [Aspects & Services](aspects-and-services.html) and
[Playback & resolution](playback-and-resolution.html).

---

## 2. The economics: a race to the bottom

The pro-rata streaming model pays fractions of a cent per play (publicly
reported figures hover around **US$0.003–0.005 per stream**, varying by service
and deal). Under that model, revenue pools toward whoever already has scale, and
the marginal independent release earns almost nothing.

It got more pointed in 2024, when Spotify introduced a **threshold that
demonetises tracks below ~1,000 streams in a 12-month window** — the long tail,
where most independent and niche music lives, stops paying out at all and its
share is redistributed upward. Whatever one thinks of the rationale, the
*direction* is unmistakable: the system optimises for the head and starves the
tail. For an independent artist this is a race to the bottom — make *more*,
*cheaper*, *faster*, just to clear a floor.

> These figures are drawn from public reporting and industry disclosures and are
> approximate and illustrative; exact rates are deal-specific and not published.

Bungalow doesn't fix royalties — software can't legislate a fairer rate. But by
**separating "where you discover/own" from "who you pay"**, it removes the
structural reason a listener is forced through one revenue funnel, and makes room
for direct-support and purchase models (Bandcamp-style) to sit *beside* streaming
rather than be buried under it.

---

## 3. Gatekeeping, playlists, and "Perfect Fit Content"

Editorial and algorithmic **playlists became the new radio** — and therefore the
new gatekeepers. Placement drives streams, streams drive income, and the keys to
placement sit with a small number of curators and the relationships around them.
That concentration invites exactly the nepotism and pay-to-play dynamics radio
spent a century being criticised for.

A sharper allegation has been reported by journalist **Liz Pelly** (in *Mood
Machine* and in *Harper's*): a Spotify program reportedly called **"Perfect Fit
Content" (PFC)**, in which low-cost production music — sometimes from a small
number of suppliers, under stock-music economics — is placed into popular
mood/ambient playlists where listeners don't look closely at who they're hearing.
Critics allege this lets the platform fill high-volume playlists with cheaper
catalogue and **crowd out independent artists** from the very real estate that
pays.

> Stated as **reporting and allegation**, attributed to that journalism — not as
> an adjudicated fact. The point for this project isn't to litigate one program;
> it's that **a single entity controlling discovery, the catalogue, *and* the
> payouts has an irreducible conflict of interest.** Whoever owns the playlist
> owns the outcome.

Bungalow's answer is structural: **discovery is a plugin, not the platform.** Any
number of `MusicCatalogueService`s (MusicBrainz today; Discogs, Bandcamp, a
community catalogue tomorrow) can be installed side by side, and the app browses
all of them. No single curator sits between you and the catalogue.

---

## 4. Vendor lock-in

When your library, your playlists, and your playback identity live inside one
company's account, leaving means losing all three. That's not an accident; it's
the retention mechanism. The catalogue you "have" is a set of licences that can
lapse; the track that played yesterday can grey out tomorrow.

Bungalow keeps the things that should be *yours* — your library, your playlists,
and **your resolution choices** ("play this track from here") — in a local store
you own, expressed in vendor-neutral terms (ISRC, title/artist, URIs). Back-ends
are interchangeable under that. See
[Playback & resolution](playback-and-resolution.html): a track is resolved to a
Service *at play time*, and you can pin or change that choice.

---

## 5. The UX regression: from lightning to bloat

There's a smaller, more visceral grievance, and anyone who used these apps in
2009 feels it: **the clients got worse.**

The Spotify desktop client of ~2009 was a small, native, near-instant program —
it opened immediately, searched as you typed, and got out of the way. Over the
following decade the desktop app was re-built on web technology (a Chromium-
embedded shell — the thing people loosely call "an Electron client"). It grew
heavier, slower to start, more animated, more promotional, and less direct. The
same arc played out across the industry: native craft traded for cross-platform
convenience and a surface optimised for engagement metrics rather than for the
person trying to play a song.

Bungalow is, deliberately, a **native Java/Swing desktop app** with a custom,
GPU-cheap rendering layer — fast to open, keyboard-driven, dense where density
helps. It treats the player as a *tool*, not a storefront.

---

## 6. The road not taken: Longhorn, WinFS, Aero, and Windows Media Player

Here's the part that makes Bungalow a period piece on purpose.

Around 2003–2006, Microsoft's **"Longhorn"** project sketched a different future
for media on the desktop:

- **WinFS** — a relational, metadata-rich storage layer where your media was
  *data you owned and could query*, not rows in a vendor's cloud database.
- **Avalon / WPF + "Aero"** — a composited, vector, glassy UI (the aesthetic the
  internet now lovingly calls **Frutiger Aero**): glossy, optimistic, tactile.
- **Windows Media Player 10/11** as a *platform*, not a product — with an
  **online-store model (URGE, PlaysForSure)** where multiple music stores plugged
  *into* the player. The player did playback and library; the **stores were
  pluggable**. Discovery and purchase were meant to be a *marketplace*, not a
  monopoly.

That architecture — **a media OS where catalogues and stores are plugins around a
neutral player and a library you own** — is almost exactly the composable model
Bungalow implements. Longhorn was gutted and shipped as Vista; WinFS never
landed; PlaysForSure was undercut and abandoned; and the industry took the *other*
fork — the closed streaming monolith.

**Bungalow is what that fork might have grown into had it survived.** Its default
look is a Windows-Media-Player-style "Chrome" (`WMP1XDesign`), its accent is
unapologetically Frutiger Aero glass, and its plugin model is the
"stores-and-catalogues-plug-into-the-player" idea, brought forward to a world of
streaming and open metadata. It is equal parts critique, restoration, and
proposal.

---

## 7. What Bungalow proposes

Concretely, the architecture turns each grievance above into a seam you can open:

- **Discovery is a plugin** (`MusicCatalogueService`) → no single gatekeeper.
- **Streaming is a plugin** (`MediaService`) → no back-end lock-in.
- **Resolution is explicit** ("Play with…") → the track isn't owned by a vendor;
  it's resolved at play time and you can pin the choice.
- **Library and playlists are local and vendor-neutral** → cancelling a service
  doesn't erase your collection.
- **Purchase can sit beside streaming** (store front-ends + `AuthAspect`) → the
  PlaysForSure marketplace idea, revived.
- **The whole look-and-feel is a plugin** (`Design = Chrome + Skin`) → the player
  is a tool you re-skin, not a storefront you endure.

The goal is an **operating system for an open music ecosystem**: the app is the
neutral substrate; the catalogues, stores, streamers, playlist providers, and
even the visual identity are "apps" that plug in and compete on merit.

---

## 8. Honest scope

This is a research/hobby project and a statement of intent, not a shipping
competitor to a streaming service. It cannot, by itself, fix royalty rates,
license major-label catalogues, or change anyone's payouts. What it *can* do is
demonstrate — in working code — that the closed monolith is a choice, that the
pieces come apart cleanly, and that the more open model the industry abandoned is
still technically within reach.

Where this document references specific company practices (royalty thresholds,
"Perfect Fit Content," client architecture), those are summarised from public
reporting and disclosures and are framed as reporting or allegation, not as
adjudicated fact. The argument doesn't depend on any single claim — it depends on
the structural point that **bundling discovery, catalogue, payment, and playback
into one account concentrates power in ways that are bad for listeners and
independent artists alike.**

See next: [Architecture overview](architecture.html).
