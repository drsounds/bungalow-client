---
title: Sites and bookmarks
sidebar_position: 4
---

# Sites and bookmarks

Bungalow can show ordinary web sites, store fronts and curator pages inside the
client. They are addressed with `bungalow:` URIs like everything else, so they
take part in history, search and the sidebar. All of this is contributed by
`WebConcept`.

## Site URIs

`web::SiteUri` (`bungalow/include/web/SiteUri.h`) translates between
`bungalow:site:` URIs and https URLs:

```
bungalow:site:<host>[:<segment>…][?query][#fragment]

bungalow:site:example.com:foo:bar?x=1  ↔  https://example.com/foo/bar?x=1
bungalow:site:example.com              ↔  https://example.com
```

Segments separated by colons after the host become path components. The query
and fragment are passed through unchanged. Port and user info are dropped when
converting a URL back. Every function takes the prefix as a parameter, so the
same grammar is used for `bungalow:store:`.

## Store and curator pages

| URI | URL |
|-----|-----|
| `bungalow:store:mystore` | `https://mystore.store.bungalow.se` |
| `bungalow:store:mystore:page` | `https://mystore.store.bungalow.se/page` |
| `bungalow:store:example.com` | `https://example.com` |
| `bungalow:curator:mycurator` | `https://mycurator.curator.bungalow.se` |
| `bungalow:curator:example.com:page` | `https://example.com/page` |

A bare name is a hosted page on `bungalow.se`; a name with a dot is used as a
host. `StoreView` and `CuratorView` show them. This is how purchase and
curation can sit next to the library without being part of the client.

## The site browser

`SiteView` shows sites in an `SPEmbeddedSiteBrowser`: CEF on WTL, wxWebView on
wxWidgets.

* **All sites share one persistent profile.**
* Top-level navigation stays in the browser only for hosts that have been
  allowed. Navigation to any other host, or to a new window, is cancelled and
  passed back (`navigationRequested`) so the client can **ask for consent**.
  Accepted hosts are saved in the `accepted_domains` table.
* Links and redirects to `bungalow:` URIs open in the client
  (`navigateRequested`).
* Downloads are cancelled in the browser and handed to Bungalow's download
  manager (`downloadRequested`); they appear at `bungalow:downloads`.
* Address changes (page loads, `pushState`, hash changes) update the address
  bar and history.

## Bookmarks

`web::BookmarkManager` stores **hierarchical** bookmarks in the `bookmarks`
table:

* A **root** bookmark is a host (`bungalow:site:example.com`) or an app
  (`bungalow:app:finder`).
* Bookmarked sub-pages are its **children**. Bookmarking a sub-page creates its
  root as a container if needed.

```cpp
bool added = web::BookmarkManager::toggle(db, uri, title);
auto roots = web::BookmarkManager::roots(db, "bungalow:site:");
```

Every change fires `web::BookmarkEvents`. `WebConcept` listens, rebuilds the
sidebar and fetches the favicon of newly bookmarked hosts with `FaviconCache`
(`https://<host>/favicon.ico`, cached in `<user data>/favicons`). Bookmarks
appear in two sidebar sections:

| Section | Prefix |
|---------|--------|
| **APPS** | `bungalow:app:` |
| **SITES** | `bungalow:site:` |

Sidebar nodes created from bookmarks are marked with `setBookmark(true)`, so
they can be removed from the sidebar's context menu. The bookmark buttons in
`SiteView` and `AppView` fire a `bookmarktoggled` event on the window element,
which `MainWindow` handles.
