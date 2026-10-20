# sp://import — Apps API (version 1)

Shared JavaScript libraries for HTML5 app bundles, compatible with the
classic `getSpotifyApi(1)` / `sp.require()` API used by apps such as
Time Machine.

| URL | File | What |
| --- | --- | --- |
| (injected) | `scripts/sp.js` | `getSpotifyApi()`, `sp.require()`, `sp.core`, `String.prototype.decodeForText/decodeForLink/encodeToHTML/decodeForHTML` |
| `sp://import/scripts/api/models` | `scripts/api/models.js` | `EVENT`, `Observable`, `Track`, `Album`, `Artist`, `User`, `Playlist`, `Search`, `Link`, `application`, `player`, `session`, `library`, `readFile` |
| `sp://import/scripts/api/views` | `scripts/api/views.js` | `Image`, `Player`, `Track` (+ `Track.FIELD`), `List` |
| `sp://import/scripts/api/auth` | `scripts/api/auth.js` | `authenticateWithFacebook`, `showAuthenticationDialog` (mocked) |
| `sp://import/scripts/googletracker` | `scripts/googletracker.js` | `GoogleTracker` (logs only) |
| `sp://import/css/adam.css`, `api.css` | `css/` | default dark theme and view layout |

## Metadata

All metadata comes from `scripts/api/datasource.js`: a fictive, deterministic
catalog (same URI → same entity; unknown URIs are generated on demand). It is
the only module that knows where data comes from — the C++ bridge replaces
its exported functions (`lookup`, `search`, `createPlaylist`,
`playlistChanged`, `setStarred`, `setSubscribed`, ...) without changing
`models` or `views`. The player is simulated (state and position only).

## Routing

`bungalow:app:<app>:a:b` is served as `sp://<app>/a/b`, and
`models.application.arguments` is `["a", "b"]`. Clicking a `spotify:` link or
assigning `location = "spotify:..."` is routed through
`window.bungalow.navigate()` (as `bungalow:...`), so the host's view stack
records it and calls back `window.navigate(path)` for the app's own URIs.

## Dev mode

Outside the host (`http:` pages), `sp://<bundle>/x` is fetched from
`<origin>/<bundle>/x`, so the bundles can be served side by side from any
static server with `scripts/sp.js` included in the app page.
