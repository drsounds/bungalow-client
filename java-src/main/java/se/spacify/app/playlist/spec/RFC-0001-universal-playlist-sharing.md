# RFC-0001 — Universal Playlist Sharing Link (UPSL)

| | |
|---|---|
| **Status** | **Accepted — v1 frozen** (one external dependency: register the canonical domain, §11.1) |
| **Created** | 2026-06-30 |
| **Accepted** | 2026-06-30 |
| **Revised** | 2026-07-05 — content references adopt the `musik:` scheme ([RFC-0002](../../music/spec/RFC-0002-musik-uri-scheme.md)); legacy `spacify:` content refs remain accepted (§7). |
| **Author(s)** | Alexander Forselius (@drsounds) |
| **Source sketch** | [UNIVERSAL_PLAYLIST_SHARING.md](UNIVERSAL_PLAYLIST_SHARING.md) |
| **Scope** | The **UPSL URI scheme** and its federation guidelines. This is a wire-format spec; it deliberately does **not** prescribe how any particular application resolves, stores, or renders a UPSL. |

> This document specifies a link format only. Client integration (how an app
> dispatches the URI, builds a playlist object, draws a preview, etc.) is out of
> scope and intentionally left to implementers.

---

## 1. Abstract

A **Universal Playlist Sharing Link (UPSL)** encodes a playlist — its name,
description, optional cover image, and an ordered list of content references —
into a single, self-contained `spacify:` URI (with an equivalent `https://`
mirror). Each UPSL is attributed to a federated (Mastodon-style) handle and
carries a stable random playlist id, enabling the origin instance to act as an
optional hub for subscriptions and update pingbacks. A UPSL **MAY** be signed by
the origin instance for verifiable provenance.

## 2. Motivation

Playlists are most useful when they travel. Because content is addressed by
**stable identifiers** (ISRC, or artist/release/track names + recording version)
rather than by one provider's row ids, a playlist is portable: any client that
can resolve those identifiers reconstructs the same listening experience on
whatever services it has. UPSL makes that portability *shareable* and
*federated*:

- **Decentralized** — a link is self-describing; no central catalogue is required.
- **Federated** — each link is attributed to a `@user@instance` handle and a stable
  playlist id, so it travels through the fediverse and the origin instance can
  coordinate subscriptions and updates (§9).
- **Service-agnostic** — references resolve through an identifier/name lookup model,
  so each recipient plays items on *their* services.
- **Self-contained or hosted** — small playlists embed entirely in the link; large
  ones reference a hosted copy by id (§8).

## 3. Terminology

The key words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY** are to
be interpreted as in RFC 2119.

- **UPSL** — a complete link in the `spacify:federated:` form (§5) or its `https:`
  mirror (§8).
- **Handle** — a Mastodon `acct` identity, `@localpart@domain` (RFC 7565).
- **Playlist id** — a random UUIDv4 (RFC 4122) identifying the playlist across
  edits; the key the origin instance uses for subscriptions/pingbacks (§9).
- **Body** — the structured, delimited string describing the playlist contents
  (§6). Present in *embedded* links, absent in *hosted* links.
- **Content reference** — one inner URI naming a playable item (§7).
- **Signature** — an optional origin-instance signature over the link (§10).

> *Notation:* `<x>` denotes a placeholder; angle brackets are **not** literal in a
> real link. `B62(x)` / `B64U(x)` denote the encodings of §4.

## 4. Encoding primitives

A UPSL uses two encodings so that user text can never collide with the structural
delimiters `:` and `,`. Any implementation needs a Base62 and a Base64url codec.

### 4.1 Base62
- Alphabet (index → glyph): `0-9` (0–9), `A-Z` (10–35), `a-z` (36–61).
- `B62(bytes)`: interpret `bytes` as a big-endian unsigned integer and emit its
  base-62 representation; each leading `0x00` byte **MUST** be preserved as a
  leading `'0'` glyph (as Base58Check preserves leading zeros), so the transform is
  exactly reversible. `B62(text)` ≙ `B62(UTF8(text))`. Output is `[0-9A-Za-z]`.

### 4.2 Base64url
- `B64U(bytes)`: RFC 4648 §5 (URL-/filename-safe alphabet), **without** padding.
  Output is `[0-9A-Za-z_-]`. Used for the cover image (§6) and the signature (§10).

## 5. The `spacify:federated:` URI

```abnf
upsl        = "spacify:federated:user:" handle ":playlist:" playlist-id
              [ ":" body ] [ ":sig:" signature ]
handle      = "@" localpart "@" domain           ; Mastodon acct, RFC 7565
localpart   = 1*( ALPHA / DIGIT / "_" / "." / "-" )
domain      = 1*( ALPHA / DIGIT / "." / "-" )
playlist-id = UUID                               ; random UUIDv4, RFC 4122
body        = <§6>                               ; present ⇒ embedded; absent ⇒ hosted
signature   = 1*( B64U-CHAR )                    ; <§10>
```

**Parsing.** After the fixed `spacify:federated:user:` prefix, read up to `:playlist:`
as the `handle`, then the next `:`-delimited token (a UUID — no colons) as the
`playlist-id`. The remainder is interpreted by markers, all unambiguous because
`body` leaves are Base62 and `signature` is Base64url (neither contains `:`):

- a `body` is **present iff** the literal marker `:description:` occurs after the
  `playlist-id` (every body has it, §6);
- a `signature` is the final `:sig:`-introduced segment, if any.

A bare `spacify:federated:user:<handle>:playlist:<uuid>` (no body) is a **hosted**
reference (§8).

## 6. Body grammar

```abnf
body        = name ":description:" description
              [ ":image:" image ]
              ":uris:" uri-list
name        = B62TEXT            ; B62( UTF8( display name ) )
description = B62TEXT            ; may be empty
image       = B64UTEXT          ; B64U( UTF8( image URL ) )
uri-list    = [ content-ref *( "," content-ref ) ]      ; terminal, no brackets
content-ref = B62TEXT            ; B62( UTF8( content URI, §7 ) )
```

Implementations **SHOULD** ignore unknown `:key:value` segments between
`description` and `:uris:`, so the format can gain fields without breaking
readers. After `:uris:` the body holds only Base62 refs separated by `,` (no
further `:`), which keeps the optional `:sig:` separator unambiguous (§5).

## 7. Content references

Each `content-ref`, once Base62-decoded, is a [`musik:` URI](../../music/spec/RFC-0002-musik-uri-scheme.md)
in the identifier/name lookup space. A conforming reader **MUST** support at least:

| Form | Meaning |
|---|---|
| `musik:isrc:<isrc>` | A recording by ISRC — the canonical, portable key. **Preferred** (most compact). |
| `musik:artist:<artist>:release:<release>:track:<n>:name:<recording>[:version:<version>]` | A recording by human-readable names + position (RFC-0002 §6), with an **optional** version. Fallback when no ISRC is known. |
| `https:` / `http:` | An external resource (video, store page, …). |

```abnf
content-uri = isrc-ref / name-ref / web-ref
isrc-ref    = "musik:isrc:" 1*VCHAR
name-ref    = "musik:artist:" seg ":release:" seg [ ":track:" 1*2DIGIT ]
              ":name:" seg [ ":version:" seg ]
seg         = PCTTEXT       ; percent-encoded name (RFC-0002 §4.1)
web-ref     = ( "http" / "https" ) "://" 1*VCHAR
```

The `:` inside a `name-ref` is inert: every name segment is percent-encoded
(RFC-0002 §4.1) and the entire `content-ref` is one Base62 leaf in the body, so no
inner `:` is ever a body delimiter.

**Version matching.** For a `name-ref`: an absent `:version:` ⇒ resolve the
**newest / primary** known recording of that name; a present-and-found version ⇒
exact match; a present-but-missing version ⇒ resolve the primary and present the
item as **approximate** (do not silently drop it).

> **Content scheme (RFC-0002).** Content refs are `musik:` URIs as of this
> revision; the scheme is specified in
> [RFC-0002](../../music/spec/RFC-0002-musik-uri-scheme.md). For links minted
> before it, a reader **SHOULD** also accept the legacy `spacify:isrc:` /
> `spacify:recording:isrc:` and `spacify:artist:…:recording:…:version:[…]` forms
> and treat them as equivalent.

## 8. HTTPS mirror & hosted form

```abnf
upsl-url = "https://" base-domain "/user/" pct-handle "/playlist/" playlist-id
           [ "/" body ] [ "?sig=" signature ]
```

- `base-domain` = **`ups.spacify.net`** (provisional canonical host — §11.1).
- `pct-handle` is the `handle` percent-encoded per RFC 3986.
- `body` is path-safe as-is (`:` and `,` are `pchar`; Base62/Base64url leaves are
  unreserved). The optional `signature` rides as the `sig` query parameter.

**Embedded vs hosted.** With a `body` the link *is* the data (subject to the §11
size limit). Without a `body`, `(handle, playlist-id)` is a **hosted** reference:
the origin instance serves the current contents for that id — letting the HTTPS
form carry playlists too large to embed, and enabling live updates (§9). A reader
**SHOULD** prefer an embedded body when present and consult the host only for
hosted references; it **MUST NOT** require the host to be reachable to read an
embedded link. (The hosting/fetch protocol itself is out of scope.)

## 9. Federation, subscriptions & pingback (guidelines)

The `@user@instance` handle and the per-playlist `playlist-id` let the origin
instance act as an **optional** coordination hub, without making embedded
resolution depend on it. These are guidelines (SHOULD/MAY), not a mandatory
protocol; an instance that implements none of them still serves valid links.

- **Stable identity.** `(handle, playlist-id)` is the durable identity of a playlist
  across edits. The `playlist-id` is minted once (random UUIDv4) when the playlist
  is first shared and reused for every later share/update of the same playlist.
- **Subscription.** A recipient **MAY** subscribe to a playlist by following the
  issuer's actor (standard ActivityPub `Follow`) and filtering on `playlist-id`,
  or via the subscribe pingback below. The instance — already a follower hub — is
  the natural place to fan out changes.
- **Update events.** When the issuer edits a playlist, the origin instance **SHOULD**
  publish an ActivityPub `Update` (or `Announce`) carrying the `playlist-id` and a
  fresh UPSL (or hosted pointer). Subscribers' clients **SHOULD** treat the
  `playlist-id` as the correlation key and replace prior contents.
- **Pingback.** On subscribe / unsubscribe — and, **only with user consent**, on
  save or play — a recipient's client **MAY** send a minimal pingback to the
  issuer's instance keyed by `playlist-id` (e.g. an ActivityPub activity to the
  actor's inbox, or a documented webhook). This lets the instance **track
  subscription events** — subscriber counts, update notifications, basic reach.
- **Privacy bounds.** Pingbacks reveal that a recipient holds the link. Clients
  **MUST** make save/play pingbacks opt-in, **SHOULD** send the minimum (the
  `playlist-id` and event type, never a user identifier unless the user consents),
  and **MUST NOT** block resolution on the instance being reachable.
- **Provenance tie-in.** Signing (§10) binds these events to a verifiable origin
  key, so an instance can distinguish authentic update events from spoofed ones.

## 10. Signing & verification (optional)

Signing is **OPTIONAL** but **RECOMMENDED** for links published through the
fediverse.

- **Algorithm:** Ed25519 (RFC 8032). `signature = B64U( Ed25519-Sign(K_priv,
  canonical) )`, where `canonical` is the exact UTF-8 of the link up to **but
  excluding** the `:sig:` segment, i.e.
  `spacify:federated:user:<handle>:playlist:<playlist-id>[:<body>]`.
- **Key discovery:** the verifier resolves the `handle` to its ActivityPub actor
  (WebFinger, RFC 7033, on `domain`) and uses the actor's published signing key.
- **Provenance, not authorization:** a valid signature proves the link was issued
  by that instance's key; it confers no trust beyond "verified origin". An absent
  or failed signature degrades to an unverified handle *claim* (§12). Verifiers
  **MUST** surface a failed signature rather than treat it as verified, and **MUST
  NOT** require origin reachability to read an embedded link.

## 11. Size constraints

URIs are widely capped at ~2048 characters; the **embedded** form therefore has a
finite item budget. Single-encoding keeps the body near the raw leaf bytes
inflated by Base62/Base64url expansion (~1.34×), plus the fixed prefix and the
36-char `playlist-id`:

```
len(link) ≈ len("spacify:federated:user:") + len(handle) + len(":playlist:")
          + 36 (uuid) + Σ( ~1.34 × leaf-bytes ) + structural ":"/"," chars
          [ + len(":sig:") + ~86  if signed ]
```

Rules:
- A producer **MUST** keep an embedded UPSL ≤ **2048** characters.
- When a playlist would exceed the budget, the producer **MUST** either emit the
  **hosted** form (§8) — **RECOMMENDED**, since it also enables updates — or truncate
  to the first *N* items that fit and mark the result truncated.
- Prefer compact `isrc-ref`s over verbose `name-ref`s to raise the embeddable count.

## 11.1 External dependency

`base-domain = ups.spacify.net` is **provisional**: the domain must be registered
and the hosted/mirror endpoint stood up before the `https:` form and hosted
references go live. In-app `spacify:federated:` links work without it.

## 12. Security & privacy considerations

- **Encoding ≠ encryption.** Base62/Base64url are reversible; a UPSL exposes the
  full playlist to anyone holding it. Keep private data out of `name`/`description`.
- **Provenance.** Without a valid signature (§10), the `handle` is a claim; UI
  **SHOULD** show "shared by `<handle>`" and only badge "verified" on success.
- **External references & tracking.** `http(s):` content refs, the `image` URL, and
  pingbacks (§9) all involve third-party / origin requests that can leak the
  recipient's IP or act as tracking. Readers **SHOULD** fetch lazily, **MUST** make
  save/play pingbacks opt-in, and **SHOULD** minimize what they send.
- **Injection.** The parser trusts that `:`/`,` appear only as delimiters; this holds
  only because every leaf is Base62/Base64url. A reader **MUST** reject a body whose
  leaves do not cleanly decode rather than guessing.
- **Resource limits (DoS).** A reader **MUST** bound decoded size, item count, and
  per-reference network work, independent of the §11 producer limit.

## 13. Worked example (illustrative)

A two-item playlist by `@drsounds@mastodon.social` named *Roadtrip*
("Summer bangers"), one ISRC item and one name-addressed item, signed:

```
body (B62/B64U shown symbolically):
  B62("Roadtrip"):description:B62("Summer bangers"):uris:
    B62("musik:isrc:GBAAA0000001"),
    B62("musik:artist:Aphex%20Twin:release:SAW%2085-92:track:1:name:Xtal")

embedded UPSL (signed):
  spacify:federated:user:@drsounds@mastodon.social:playlist:8f2b…-uuid:<body>:sig:<B64U(sig)>

hosted UPSL (no body — instance serves it, subscribable):
  spacify:federated:user:@drsounds@mastodon.social:playlist:8f2b…-uuid

HTTPS mirror (embedded):
  https://ups.spacify.net/user/%40drsounds%40mastodon.social/playlist/8f2b…-uuid/<body>?sig=<B64U(sig)>
```

(`<body>`/`<sig>`/`8f2b…-uuid` are placeholders; a conforming encoder emits
concrete `0-9A-Za-z` / `0-9A-Za-z_-` / UUID strings.)

## 14. References

- RFC 2119 — Requirement-level key words.
- RFC 3986 — URI generic syntax.
- RFC 4122 — UUID.
- RFC 4648 — Base16/32/64 (Base64url, §4).
- RFC 7033 — WebFinger.
- RFC 7565 — the `acct:` URI scheme (Mastodon handles).
- RFC 8032 — EdDSA / Ed25519.
- W3C ActivityPub (2018) — `Follow` / `Update` / `Announce`, actor & inbox.
