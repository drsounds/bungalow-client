# RFC-0002 — The `musik:` URI Scheme

| | |
|---|---|
| **Scheme name** | `musik` |
| **Status** | **Provisional** |
| **Created** | 2026-07-05 |
| **Applications/protocols that use this scheme name** | Bungalow |
| **Contact** | Alexander Forselius <drsounds@gmail.com> |
| **Change controller** | Alexander Forselius <drsounds@gmail.com> |
| **Scope** | The **`musik:` URI scheme** — a service-agnostic way to *name* a piece of music (a recording, work, release, artist, genre or mood) by a stable identifier or by human-readable coordinates. This is a naming/wire-format spec; it deliberately does **not** prescribe how any particular application resolves or renders a `musik:` URI. |

> This document specifies an identifier format only. Client integration (how an app
> dispatches the URI, builds a playable object, draws a page, etc.) is out of scope
> and left to implementers. See [`docs/musik-uri-scheme.md`](../../../../../../../../docs/musik-uri-scheme.md)
> for how Bungalow resolves these URIs in practice.

---

## 1. Abstract

A **`musik:` URI** names one musical entity independently of any provider. It
addresses content either by a **globally standard identifier** (ISRC, ISWC, UPC,
ISNI, IPI) or by **human-readable coordinates** (artist / release / track / name /
version), plus lightweight **facet** references (genre, mood). Because a `musik:`
URI carries identity rather than a vendor row id, the same URI resolves on
whatever services a given client has — it is the portable, shareable atom that
playlists (see [RFC-0001](../../playlist/spec/RFC-0001-universal-playlist-sharing.md)),
libraries and drag-and-drop payloads are built from.

## 2. Motivation

Music identity outlives any one catalogue. A recording has an ISRC; a work has an
ISWC; a release has a UPC; a creator has an ISNI/IPI. When those exist, a link
**should** use them, so any conforming reader reconstructs the same item on its
own services. When they don't, the URI degrades gracefully to **names +
position**, which a reader resolves by lookup. One scheme spans both, so a client
never has to choose between "portable" and "expressible".

- **Service-agnostic** — no provider is named; each reader resolves on its services.
- **Standard-first** — prefers registered identifiers (ISRC/ISWC/UPC/ISNI/IPI).
- **Human-addressable fallback** — names + track position when no identifier is known.
- **Composable** — one URI per entity, so lists (playlists, queues) are just
  ordered `musik:` URIs.

## 3. Terminology

The key words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY** are to
be interpreted as in RFC 2119.

- **Entity** — the thing a URI names: a recording, work, release, artist, genre or mood.
- **Identifier form** — a URI keyed by a registered standard id (§5).
- **Name form** — the hierarchical `artist:…` URI keyed by human-readable coordinates (§6).
- **Query form** — the `musik:track?…` variant of the name form using a query string (§7).
- **Slug** — a lower-case, hyphen-joined, URL-encoded label (§4.2).

> *Notation:* `<x>` denotes a placeholder; angle brackets are **not** literal.
> `PCT(x)` denotes the percent-encoding of §4.1. `[ ]` marks an optional group.

## 4. Encoding

A `musik:` URI is a `URI` per RFC 3986. It uses only the opaque (scheme-specific)
part after `musik:`; there is no authority (`//`) component. The two structural
delimiters are `:` (segment separator) and, in the query form, the usual `?`/`&`/`=`.

### 4.1 Percent-encoding of free text

Any free-text value — an artist name, a release name, a track name, a version
title — **MUST** be percent-encoded (RFC 3986 §2.1, `PCT`) so that a literal `:`,
`?`, `#`, `&`, `%`, or whitespace inside the text can never be mistaken for a
delimiter. A reader **MUST** percent-decode each segment before use. Space
**SHOULD** be encoded as `%20` (not `+`) in the segment forms; in the query form's
`application/x-www-form-urlencoded` string, `+` **MAY** denote space per that
serialization.

### 4.2 Slugs (genre, mood)

A `genre` or `mood` is a **slug**: the label lower-cased, non-alphanumeric runs
collapsed to a single `-`, trimmed of leading/trailing `-`, then percent-encoded.
`"Drum & Bass"` → `drum-bass`; `"Lo-Fi"` → `lo-fi`. Slugging is lossy and
**intentionally** so — facets are coarse buckets, not exact strings.

### 4.3 Standard identifiers

Registered identifiers are used **verbatim**, uppercased where the standard is
case-insensitive, with formatting separators removed:

| Kind | Standard | Canonical form in a URI |
|---|---|---|
| `isrc` | ISRC (ISO 3901) | 12 chars `CCXXXYYNNNNN`, no hyphens, upper-case. |
| `iswc` | ISWC (ISO 15707) | `T` + 9 digits + check digit, no dots/hyphens (e.g. `T0345246800`). |
| `upc`  | UPC-A / EAN-13 (GTIN) | 12–13 digits, no spaces. |
| `isni` | ISNI (ISO 27729) | 16 digits, no spaces (e.g. `000000012146438X`). |
| `ipi`  | IPI name number | 9–11 digits, no spaces. |

A reader **SHOULD** normalise (strip separators, upper-case) before matching and
**MUST NOT** reject a URI solely because an identifier's check digit fails — it
**MAY** surface it as unverified.

## 5. Identifier forms

```abnf
musik-id    = "musik:" id-kind ":" id-value
id-kind     = "isrc" / "iswc" / "upc" / "isni" / "ipi" / "genre" / "mood"
id-value    = 1*pchar          ; §4.3 for standard ids, §4.2 for genre/mood
```

| Form | Names |
|---|---|
| `musik:isrc:<ISRC>` | a particular **recording**. |
| `musik:iswc:<ISWC>` | a particular **work** (composition). |
| `musik:upc:<UPC>` | a particular **release**. |
| `musik:isni:<ISNI>` | a particular **artist / creator**. |
| `musik:ipi:<IPI>` | a creator by **IPI** name number. |
| `musik:genre:<slug>` | the **genre** facet named by `<slug>` (§4.2). |
| `musik:mood:<slug>` | the **mood** facet named by `<slug>` (§4.2). |

Examples:

```
musik:isrc:GBAYE0601498
musik:iswc:T0345246800
musik:upc:00602557091936
musik:isni:000000012146438X
musik:ipi:00016860138
musik:genre:drum-bass
musik:mood:melancholic
```

## 6. Name form (human-readable coordinates)

When no standard identifier is known, an entity is addressed by nested,
percent-encoded coordinates. Each deeper segment narrows the entity: artist →
release → track (by number) → recording name → version.

```abnf
musik-name  = "musik:artist:" artist
              [ ":release:" release
                [ [ ":track:" track-no ] ":name:" track-name
                  [ ":version:" version ] ] ]
artist      = PCTTEXT          ; PCT( artist display name )
release     = PCTTEXT          ; PCT( release display name )
track-no    = 1*2DIGIT         ; 0–99, the position on the release
track-name  = PCTTEXT          ; PCT( recording / track name )
version     = PCTTEXT          ; PCT( version title, e.g. "Radio Edit" )
```

What each depth names:

| Depth | Names |
|---|---|
| `musik:artist:<a>` | the **artist** `<a>`. |
| `…:release:<r>` | the **release** `<r>` by `<a>`. |
| `…:release:<r>:track:<n>:name:<t>` | the **recording** at position `<n>` on `<r>`, named `<t>`. |
| `…:name:<t>` (no `:track:`) | the recording named `<t>` on `<r>` (position unknown). |
| `…:version:<v>` | the specific **version** `<v>` of that recording. |

The `:track:<n>` group is optional: a reader **SHOULD** use `<n>` to disambiguate
when two recordings on a release share a name, and **MUST** fall back to matching
`<t>` alone when it is absent. `<n>` is `0`–`99`; `0` means "position unspecified"
if a producer must emit the group without a known number.

Examples:

```
musik:artist:Aphex%20Twin
musik:artist:Aphex%20Twin:release:Selected%20Ambient%20Works%2085-92
musik:artist:Aphex%20Twin:release:Selected%20Ambient%20Works%2085-92:track:1:name:Xtal
musik:artist:Radiohead:release:OK%20Computer:track:5:name:Karma%20Police:version:Live
musik:artist:Radiohead:release:OK%20Computer:name:Karma%20Police
```

**Version matching.** No `:version:` ⇒ resolve the **newest / primary** recording
of that name; a present-and-found version ⇒ exact match; a present-but-missing
version ⇒ resolve the primary and present the item as **approximate** (a reader
**MUST NOT** silently drop it).

## 7. Query form

The name form has an equivalent flat query variant, convenient for producers that
would rather emit `application/x-www-form-urlencoded` than nest segments. It names
a **recording**.

```abnf
musik-query = "musik:track?" query
query       = param *( "&" param )
param       = "name="         PCTTEXT
            / "artist_name="  PCTTEXT
            / "release_name=" PCTTEXT
            / "number="       1*2DIGIT     ; 0–99
            / "version="      PCTTEXT
```

- `name` is **REQUIRED**; the rest are **OPTIONAL**.
- Unknown parameters **MUST** be ignored (forward-compatibility).
- The query form and the name form (§6) are **equivalent**: a reader **SHOULD**
  resolve `musik:track?name=Xtal&artist_name=Aphex%20Twin&release_name=Selected%20Ambient%20Works%2085-92&number=1`
  identically to the matching `musik:artist:…:release:…:track:1:name:Xtal`.

Example:

```
musik:track?name=Karma%20Police&artist_name=Radiohead&release_name=OK%20Computer&number=5&version=Live
```

## 8. Resolution model (guidelines)

Resolution is a client concern; these are SHOULD/MAY guidelines, not a wire
requirement. A conforming reader:

- **MUST** support at least the `isrc` and name/query forms — the two portable
  recording addresses — and **SHOULD** support `iswc`, `upc`.
- resolves an **`isrc`** by looking the recording up across its services (the most
  reliable, most compact key), and a **name/query** form by a title+artist
  (+release, +number, +version) metadata lookup;
- resolves an **`iswc`** to any recording of that work, and a **`upc`** to that
  release's tracks;
- treats **`isni` / `ipi`** as artist/creator references (navigate to a creator
  page or filter), and **`genre` / `mood`** as facet references (browse/filter) —
  these name entities but need not be directly *playable*;
- **MUST NOT** require any single provider to be reachable to interpret a URI.

Precedence when producing a URI for a recording: prefer `isrc`; else the name form
with as many known coordinates as possible.

## 9. Relationship to `spacify:` URIs

`musik:` is the **canonical, portable content-identifier** scheme. Bungalow also
has internal `spacify:` navigation/aggregate URIs (`spacify:library`,
`spacify:playlist:<id>`, `spacify:release:…`) that address *app views*, not
portable content, and remain in use. The legacy content URI
`spacify:recording:isrc:<ISRC>` is the exact predecessor of `musik:isrc:<ISRC>`; a
reader **SHOULD** continue to accept it and treat it as equivalent, and producers
**SHOULD** emit `musik:isrc:` going forward.

## 10. Security & privacy considerations

- **Encoding ≠ secrecy.** A `musik:` URI is plainly readable; it exposes the named
  title/artist. Keep nothing private in the coordinates.
- **Lookup fan-out (DoS).** A name/query form triggers cross-service metadata
  lookups; a reader **MUST** bound the work per URI (result caps, timeouts) and
  **SHOULD** de-duplicate identical URIs in a list before resolving.
- **Injection.** The parser trusts that unescaped `:`/`?`/`&` are delimiters; this
  holds only because free text is percent-encoded (§4.1). A reader **MUST** reject
  a segment that does not cleanly percent-decode rather than guessing.
- **Spoofable names.** The name form asserts an artist/title with no proof; a valid
  `musik:` URI is a *claim*, not provenance. Provenance for shared collections is
  layered on top by [RFC-0001](../../playlist/spec/RFC-0001-universal-playlist-sharing.md) (signing).

## 11. Examples (round-trip)

```
recording by ISRC          musik:isrc:GBAYE0601498
work by ISWC               musik:iswc:T0345246800
release by UPC             musik:upc:00602557091936
artist by ISNI             musik:isni:000000012146438X
creator by IPI             musik:ipi:00016860138
genre facet                musik:genre:drum-bass
mood facet                 musik:mood:melancholic
artist                     musik:artist:Aphex%20Twin
release                    musik:artist:Aphex%20Twin:release:Drukqs
recording (with position)  musik:artist:Aphex%20Twin:release:Drukqs:track:1:name:Jynweythek
recording (query form)     musik:track?name=Jynweythek&artist_name=Aphex%20Twin&release_name=Drukqs&number=1
```

## 12. References

- RFC 2119 — Requirement-level key words.
- RFC 3986 — URI generic syntax; percent-encoding.
- ISO 3901 — ISRC. · ISO 15707 — ISWC. · ISO 27729 — ISNI.
- GS1 — UPC-A / EAN-13 (GTIN). · CISAC — IPI.
- [RFC-0001](../../playlist/spec/RFC-0001-universal-playlist-sharing.md) — Universal Playlist Sharing Link.
