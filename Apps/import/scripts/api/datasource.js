/**
 * @module datasource
 *
 * Metadata backend behind sp://import/scripts/api/models.
 *
 * For now this is a fictive, deterministic catalog generated in JavaScript:
 * the same URI always yields the same artist/album/track, unknown URIs are
 * generated on demand from their own hash, and nothing leaves the page.
 * The C++ bridge will replace this module; models only uses the functions
 * exported at the bottom (lookup, search, createPlaylist, ...), all of which
 * return plain data objects:
 *
 *   track    { uri, name, duration, popularity, number, starred, playable,
 *              artists: [artistRef], album: albumRef }
 *   album    { uri, name, year, cover, genre, albumType, artist: artistRef, tracks: [uri] }
 *   artist   { uri, name, portrait, genres, popularity, biography }
 *   playlist { uri, name, description, cover, owner: userRef, subscribed,
 *              collaborative, tracks: [uri] }
 *   user     { uri, name, username, picture }
 */

var FIRST_YEAR = 1950;
var LAST_YEAR = new Date().getFullYear();
var ARTIST_COUNT = 220;
var MIN_ARTISTS_PER_YEAR = 8;

var BASE62 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// ── Deterministic randomness ─────────────────────────────────────────────

function hash(str) {
    // FNV-1a, 32 bit
    var h = 0x811c9dc5;
    for (var i = 0; i < str.length; i++) {
        h ^= str.charCodeAt(i);
        h = Math.imul(h, 0x01000193);
    }
    return h >>> 0;
}

function rng(seed) {
    // mulberry32
    var a = typeof seed === "number" ? seed : hash(String(seed));
    return function () {
        a = (a + 0x6D2B79F5) | 0;
        var t = Math.imul(a ^ (a >>> 15), 1 | a);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

function pick(rand, list) { return list[Math.floor(rand() * list.length)]; }
function between(rand, min, max) { return min + Math.floor(rand() * (max - min + 1)); }

function spotifyId(seed) {
    var rand = rng("id:" + seed);
    var id = "";
    for (var i = 0; i < 22; i++) id += BASE62[Math.floor(rand() * 62)];
    return id;
}

function capitalize(s) { return s.charAt(0).toUpperCase() + s.slice(1); }

// ── Word lists for the fictive names ─────────────────────────────────────

var ADJECTIVES = ["silver", "midnight", "electric", "velvet", "golden", "crystal", "neon", "hollow",
    "wild", "quiet", "scarlet", "frozen", "burning", "lonely", "cosmic", "paper", "violet", "northern",
    "broken", "secret", "endless", "lunar", "amber", "distant", "restless", "sleepy", "copper", "faded"];
var NOUNS = ["harbor", "echo", "garden", "river", "machine", "satellite", "lantern", "horizon",
    "parade", "mirror", "comet", "orchard", "signal", "ballroom", "tide", "highway", "fever", "island",
    "cathedral", "telephone", "meadow", "winter", "carousel", "ocean", "engine", "forest", "neighbour"];
var FIRST_NAMES = ["Alva", "Bruno", "Cecilia", "Dexter", "Elin", "Felix", "Greta", "Hugo", "Ingrid",
    "Jonas", "Klara", "Leon", "Maja", "Nils", "Olivia", "Pontus", "Ronja", "Sixten", "Tove", "Ulf",
    "Vera", "Wilma", "Axel", "Signe", "Ebbe", "Lova", "Otto", "Stella", "Valter", "Iris"];
var LAST_NAMES = ["Lindqvist", "Berg", "Holm", "Sundin", "Ek", "Falk", "Strand", "Moberg", "Wallin",
    "Dahl", "Norén", "Hedlund", "Sjöberg", "Lund", "Ahlström", "Blom", "Kvist", "Rapp", "Öhman", "Frost"];
var BAND_PREFIX = ["The", "", "", "Los", "", "Club", "", "Orchestra"];
var GENRES = ["Pop", "Rock", "Jazz", "Soul", "Disco", "Synthpop", "Punk", "Funk", "Folk", "Electronic",
    "Hip-Hop", "Indie", "Schlager", "Country", "Blues", "House", "Ambient", "Metal", "R&B", "Reggae"];
// Rough era each genre is most common in, so the year search feels plausible.
var GENRE_ERA = {
    "Jazz": [1950, 1975], "Blues": [1950, 1972], "Rock": [1958, LAST_YEAR], "Soul": [1960, 1985],
    "Folk": [1955, LAST_YEAR], "Country": [1950, LAST_YEAR], "Disco": [1972, 1985], "Funk": [1966, 1990],
    "Punk": [1975, 2005], "Synthpop": [1978, 1995], "Reggae": [1968, 2000], "Metal": [1975, LAST_YEAR],
    "Hip-Hop": [1982, LAST_YEAR], "House": [1985, LAST_YEAR], "Electronic": [1980, LAST_YEAR],
    "Indie": [1988, LAST_YEAR], "R&B": [1960, LAST_YEAR], "Ambient": [1978, LAST_YEAR],
    "Pop": [1955, LAST_YEAR], "Schlager": [1955, LAST_YEAR]
};
var TITLE_PATTERNS = [
    function (r) { return capitalize(pick(r, ADJECTIVES)) + " " + capitalize(pick(r, NOUNS)); },
    function (r) { return "The " + capitalize(pick(r, NOUNS)) + " Song"; },
    function (r) { return capitalize(pick(r, NOUNS)) + " of " + capitalize(pick(r, ADJECTIVES)) + " Light"; },
    function (r) { return "Dancing in the " + capitalize(pick(r, NOUNS)); },
    function (r) { return capitalize(pick(r, ADJECTIVES)); },
    function (r) { return "Love Me Like a " + capitalize(pick(r, NOUNS)); },
    function (r) { return capitalize(pick(r, NOUNS)) + "s"; },
    function (r) { return "Don't Leave the " + capitalize(pick(r, NOUNS)); },
    function (r) { return capitalize(pick(r, ADJECTIVES)) + " " + capitalize(pick(r, NOUNS)) + " Blues"; }
];

function artistName(rand) {
    switch (between(rand, 0, 3)) {
    case 0: return pick(rand, FIRST_NAMES) + " " + pick(rand, LAST_NAMES);
    case 1: return (pick(rand, BAND_PREFIX) + " " + capitalize(pick(rand, ADJECTIVES)) + " " +
                    capitalize(pick(rand, NOUNS)) + "s").trim();
    case 2: return capitalize(pick(rand, NOUNS)) + " " + capitalize(pick(rand, NOUNS));
    default: return pick(rand, FIRST_NAMES) + " & the " + capitalize(pick(rand, NOUNS)) + "s";
    }
}

function title(rand) { return pick(rand, TITLE_PATTERNS)(rand); }

// ── Generated artwork (SVG data URIs, allowed by the app sandbox) ────────

function escapeXml(s) {
    return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function initials(name) {
    return name.split(/\s+/).filter(function (w) { return /^[A-Za-zÅÄÖåäö0-9]/.test(w) && w !== "The" && w !== "&"; })
        .slice(0, 2).map(function (w) { return w.charAt(0).toUpperCase(); }).join("");
}

function artwork(seed, label, round) {
    var rand = rng("art:" + seed);
    var hue = between(rand, 0, 359);
    var hue2 = (hue + between(rand, 30, 150)) % 360;
    var shape = between(rand, 0, 2);
    var deco = shape === 0
        ? '<circle cx="' + between(rand, 60, 240) + '" cy="' + between(rand, 60, 240) + '" r="' + between(rand, 50, 140) +
          '" fill="hsla(' + hue2 + ',80%,70%,0.35)"/>'
        : shape === 1
        ? '<rect x="' + between(rand, -60, 120) + '" y="' + between(rand, 0, 200) + '" width="420" height="' +
          between(rand, 20, 90) + '" transform="rotate(-' + between(rand, 10, 40) + ' 150 150)" fill="hsla(' + hue2 + ',80%,75%,0.3)"/>'
        : '<path d="M0 ' + between(rand, 150, 260) + ' Q150 ' + between(rand, 40, 200) + ' 300 ' + between(rand, 120, 260) +
          ' L300 300 L0 300Z" fill="hsla(' + hue2 + ',70%,30%,0.45)"/>';
    var svg = '<svg xmlns="http://www.w3.org/2000/svg" width="300" height="300" viewBox="0 0 300 300">' +
        '<defs><linearGradient id="g" x1="0" y1="0" x2="1" y2="1">' +
        '<stop offset="0" stop-color="hsl(' + hue + ',65%,55%)"/><stop offset="1" stop-color="hsl(' + hue2 + ',60%,25%)"/>' +
        '</linearGradient></defs>' +
        (round ? '<clipPath id="c"><circle cx="150" cy="150" r="150"/></clipPath><g clip-path="url(#c)">' : '<g>') +
        '<rect width="300" height="300" fill="url(#g)"/>' + deco +
        '<text x="150" y="172" text-anchor="middle" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-size="72" ' +
        'font-weight="700" fill="rgba(255,255,255,0.85)">' + escapeXml(initials(label)) + '</text></g></svg>';
    return "data:image/svg+xml;charset=utf-8," + encodeURIComponent(svg);
}

// ── Catalog ──────────────────────────────────────────────────────────────

var entities = {};   // uri -> data
var artists = [];    // all artist data, generation order
var albums = [];
var tracks = [];
var built = false;

function ref(entity) { return { uri: entity.uri, name: entity.name }; }

function register(entity) {
    entities[entity.uri] = entity;
    return entity;
}

// Adds an artist with a discography. uris lets a requested (unknown) URI be
// assigned to the artist, its first album or its first track; startYear
// forces the year of the first album.
function generateArtist(seed, uris, startYear) {
    uris = uris || {};
    var rand = rng("artist:" + seed);
    var genre = pick(rand, GENRES);
    var era = GENRE_ERA[genre];
    var start = between(rand, era[0], Math.max(era[0], era[1] - 2));
    if (startYear) start = startYear;
    var name = artistName(rand);
    var artist = register({
        uri: uris.artist || "spotify:artist:" + spotifyId("artist:" + seed),
        name: name,
        portrait: null,
        genres: [genre],
        popularity: between(rand, 5, 95),
        biography: name + " is a fictive " + genre.toLowerCase() + " act formed in " + start +
                   ". (Mock metadata until the Bungalow bridge is connected.)",
        albums: [],
        type: "artist"
    });
    artist.portrait = artwork(artist.uri, artist.name, true);
    artists.push(artist);

    var albumCount = between(rand, 1, 4);
    var year = start;
    for (var a = 0; a < albumCount && year <= LAST_YEAR; a++) {
        var albumSeed = seed + ":album:" + a;
        var album = register({
            uri: (a === 0 && uris.album) || "spotify:album:" + spotifyId(albumSeed),
            name: title(rand),
            year: year,
            genre: genre,
            albumType: rand() < 0.15 ? "single" : "album",
            type: "album",
            artist: ref(artist),
            cover: null,
            tracks: []
        });
        album.cover = artwork(album.uri, album.name, false);
        albums.push(album);
        artist.albums.push(album.uri);

        var trackCount = album.albumType === "single" ? between(rand, 1, 3) : between(rand, 7, 13);
        for (var t = 0; t < trackCount; t++) {
            var track = register({
                uri: (a === 0 && t === 0 && uris.track) || "spotify:track:" + spotifyId(albumSeed + ":track:" + t),
                name: t === 0 && album.albumType === "single" ? album.name : title(rand),
                duration: between(rand, 125, 330) * 1000 + between(rand, 0, 999),
                popularity: Math.max(0, Math.min(100, artist.popularity + between(rand, -25, 15))),
                number: t + 1,
                discNumber: 1,
                starred: false,
                playable: true,
                local: false,
                explicit: rand() < 0.08,
                artists: [ref(artist)],
                album: { uri: album.uri, name: album.name, year: album.year, cover: album.cover, artist: ref(artist) },
                type: "track"
            });
            tracks.push(track);
            album.tracks.push(track.uri);
        }
        year += between(rand, 1, 6);
    }
    return artist;
}

// "Various Artists" compilations: a few per year, made of that year's tracks.
function generateCompilations() {
    var various = register({
        uri: "spotify:artist:0LyfQWJT6nXafLPZqxe9Of",
        name: "Various Artists",
        portrait: null,
        genres: [],
        popularity: 50,
        biography: "Compilations.",
        albums: [],
        type: "artist"
    });
    various.portrait = artwork(various.uri, "V A", true);

    var byYear = {};
    tracks.forEach(function (t) { (byYear[t.album.year] = byYear[t.album.year] || []).push(t); });

    for (var year = FIRST_YEAR; year <= LAST_YEAR; year++) {
        var pool = byYear[year] || [];
        // Neighbouring years help sparse years fill a compilation.
        if (pool.length < 12) pool = pool.concat(byYear[year - 1] || [], byYear[year + 1] || []);
        if (pool.length < 6) continue;
        var rand = rng("compilation:" + year);
        var count = between(rand, 1, 3);
        for (var c = 0; c < count; c++) {
            var name = pick(rand, ["Hits of " + year, "Best of " + year, year + " Gold",
                "Summer " + year, "Now That's Music " + year, "The " + year + " Collection",
                capitalize(pick(rand, ADJECTIVES)) + " " + year]);
            var album = register({
                uri: "spotify:album:" + spotifyId("compilation:" + year + ":" + c),
                name: name,
                year: year,
                genre: "Various",
                albumType: "compilation",
                type: "album",
                artist: ref(various),
                cover: null,
                tracks: []
            });
            album.cover = artwork(album.uri, album.name, false);
            var used = {};
            var size = Math.min(pool.length, between(rand, 10, 18));
            for (var n = 0; album.tracks.length < size && n < size * 4; n++) {
                var t = pick(rand, pool);
                if (!used[t.uri]) { used[t.uri] = true; album.tracks.push(t.uri); }
            }
            albums.push(album);
            various.albums.push(album.uri);
        }
    }
}

function build() {
    if (built) return;
    built = true;
    for (var i = 0; i < ARTIST_COUNT; i++) generateArtist("catalog:" + i);

    // Every year gets enough releases for a year search to look populated.
    var activeByYear = {};
    albums.forEach(function (album) {
        var set = activeByYear[album.year] = activeByYear[album.year] || {};
        set[album.artist.uri] = true;
    });
    for (var year = FIRST_YEAR; year <= LAST_YEAR; year++) {
        var active = Object.keys(activeByYear[year] || {}).length;
        for (var n = active; n < MIN_ARTISTS_PER_YEAR; n++) generateArtist("year:" + year + ":" + n, null, year);
    }
    generateCompilations();
}

// ── Users, library and playlists ─────────────────────────────────────────

var STORAGE_KEY = "bungalow.mock.library";
var currentUser = {
    uri: "spotify:user:bungalow",
    username: "bungalow",
    name: "Bungalow User",
    picture: artwork("spotify:user:bungalow", "Bungalow User", true),
    type: "user"
};
register(currentUser);

var library = { playlists: [], starred: {} };

function loadLibrary() {
    try {
        var stored = JSON.parse(localStorage.getItem(STORAGE_KEY) || "null");
        if (!stored) return;
        library.starred = stored.starred || {};
        (stored.playlists || []).forEach(function (p) {
            register(p);
            library.playlists.push(p.uri);
        });
    } catch (e) { /* storage unavailable: library lives in memory only */ }
}

function saveLibrary() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify({
            starred: library.starred,
            playlists: library.playlists.map(function (uri) { return entities[uri]; })
        }));
    } catch (e) { /* ignore */ }
}

function userFor(username) {
    var uri = "spotify:user:" + username;
    if (entities[uri]) return entities[uri];
    return register({
        uri: uri,
        username: username,
        name: username,
        picture: artwork(uri, username, true),
        type: "user"
    });
}

function playlistCover(playlist) {
    var first = playlist.tracks.length ? entities[playlist.tracks[0]] : null;
    return first ? first.album.cover : artwork(playlist.uri, playlist.name, false);
}

function generatePlaylist(uri) {
    var parts = uri.split(":");
    var owner = userFor(parts[2] || "unknown");
    var rand = rng("playlist:" + uri);
    var playlist = register({
        uri: uri,
        name: pick(rand, ["Favourites", "Road Trip", "Late Night", "Sunday Morning", "Workout",
            "Throwbacks", "Kitchen Disco", "Rainy Days"]) + " " + between(rand, 1, 99),
        description: "Fictive playlist (mock metadata).",
        owner: ref(owner),
        collaborative: false,
        subscribed: false,
        cover: null,
        tracks: [],
        type: "playlist"
    });
    var count = between(rand, 12, 30);
    for (var i = 0; i < count; i++) playlist.tracks.push(pick(rand, tracks).uri);
    playlist.cover = playlistCover(playlist);
    return playlist;
}

// ── URI handling ─────────────────────────────────────────────────────────

// Accepts spotify:, spacify:, bungalow:, space: and musik: URIs and
// open.spotify.com links; returns the spotify: form.
function normalizeUri(uri) {
    if (!uri) return uri;
    uri = String(uri).trim();
    var m = uri.match(/^https?:\/\/(?:open|play)\.spotify\.com\/(.+)$/);
    if (m) return "spotify:" + m[1].split("?")[0].replace(/\//g, ":");
    return uri.replace(/^(spacify|bungalow|space|musik):/, "spotify:");
}

function typeOf(uri) {
    var parts = normalizeUri(uri).split(":");
    if (parts[0] !== "spotify") return null;
    if (parts[1] === "user") {
        if (parts[3] === "playlist") return "playlist";
        if (parts[3] === "starred") return "playlist";
        return parts.length === 3 ? "user" : null;
    }
    return parts[1] || null;
}

function lookup(uri) {
    build();
    uri = normalizeUri(uri);
    if (entities[uri]) return entities[uri];

    // Unknown URIs get a generated entity of the requested type.
    switch (typeOf(uri)) {
    case "artist":
        return generateArtist(uri, { artist: uri });
    case "album":
        return entities[generateArtist("album-owner:" + uri, { album: uri }).albums[0]];
    case "track":
        generateArtist("track-owner:" + uri, { track: uri });
        return entities[uri];
    case "playlist":
        return generatePlaylist(uri);
    case "user":
        return userFor(uri.split(":")[2]);
    default:
        return null;
    }
}

// ── Search ───────────────────────────────────────────────────────────────

// Parses Spotify search syntax: year:1990, year:1990-1999, artist:"Name",
// album:x, track:x, genre:x, plus free text. "OR"/"AND" are ignored.
function parseQuery(query) {
    var q = { text: [], years: null, artist: null, album: null, track: null, genre: null };
    var re = /(\w+):"([^"]*)"|(\w+):(\S+)|"([^"]*)"|(\S+)/g;
    var m;
    var text = String(query || "");
    try { text = decodeURIComponent(text); } catch (e) { /* keep as is */ }
    while ((m = re.exec(text))) {
        var field = (m[1] || m[3] || "").toLowerCase();
        var value = m[2] !== undefined ? m[2] : m[4];
        if (field === "year") {
            var range = value.split("-");
            var from = parseInt(range[0], 10);
            var to = range.length > 1 ? parseInt(range[1], 10) : from;
            if (!isNaN(from)) q.years = [from, isNaN(to) ? from : to];
        } else if (field === "artist" || field === "album" || field === "track" || field === "genre") {
            q[field] = value.toLowerCase();
        } else {
            var word = (m[5] !== undefined ? m[5] : (field ? field + ":" + value : m[6])).toLowerCase();
            if (word && word !== "or" && word !== "and" && word.indexOf("<!") !== 0) q.text.push(word);
        }
    }
    return q;
}

function contains(haystack, needle) {
    return String(haystack || "").toLowerCase().indexOf(needle) !== -1;
}

function inYears(year, q) { return !q.years || (year >= q.years[0] && year <= q.years[1]); }

function textMatches(words, fields) {
    return words.every(function (w) {
        return fields.some(function (f) { return contains(f, w); });
    });
}

function byPopularity(a, b) { return (b.popularity || 0) - (a.popularity || 0); }

function search(query, options) {
    build();
    options = options || {};
    var q = parseQuery(query);
    var offset = options.offset || 0;
    var pageSize = options.pageSize || 50;

    function page(list) { return list.slice(offset, offset + pageSize); }

    var result = { query: query, tracks: [], albums: [], artists: [], playlists: [],
                   totalTracks: 0, totalAlbums: 0, totalArtists: 0, totalPlaylists: 0 };

    if (options.searchTracks !== false) {
        var t = tracks.filter(function (track) {
            var album = entities[track.album.uri];
            return inYears(track.album.year, q) &&
                (!q.artist || contains(track.artists[0].name, q.artist)) &&
                (!q.album || contains(track.album.name, q.album)) &&
                (!q.track || contains(track.name, q.track)) &&
                (!q.genre || contains(album.genre, q.genre)) &&
                textMatches(q.text, [track.name, track.artists[0].name, track.album.name, album.genre]);
        }).sort(byPopularity);
        result.totalTracks = t.length;
        result.tracks = page(t);
    }
    if (options.searchAlbums !== false) {
        var a = albums.filter(function (album) {
            return inYears(album.year, q) &&
                (!q.artist || contains(album.artist.name, q.artist)) &&
                (!q.album || contains(album.name, q.album)) &&
                (!q.genre || contains(album.genre, q.genre)) &&
                textMatches(q.text, [album.name, album.artist.name, album.genre]);
        }).sort(function (x, y) { return y.tracks.length - x.tracks.length; });
        result.totalAlbums = a.length;
        result.albums = page(a);
    }
    if (options.searchArtists !== false) {
        var r = artists.filter(function (artist) {
            if (q.years) {
                var active = artist.albums.some(function (uri) { return inYears(entities[uri].year, q); });
                if (!active) return false;
            }
            return (!q.artist || contains(artist.name, q.artist)) &&
                (!q.genre || artist.genres.some(function (g) { return contains(g, q.genre); })) &&
                textMatches(q.text, [artist.name].concat(artist.genres));
        }).sort(byPopularity);
        result.totalArtists = r.length;
        result.artists = page(r);
    }
    if (options.searchPlaylists !== false) {
        var p = library.playlists.map(function (uri) { return entities[uri]; }).filter(function (pl) {
            return q.text.length && textMatches(q.text, [pl.name]);
        });
        result.totalPlaylists = p.length;
        result.playlists = page(p);
    }
    return result;
}

// ── Mutations ────────────────────────────────────────────────────────────

var tempCounter = 0;

// A named playlist is created in the user's library; an unnamed one is a
// temporary, in-memory playlist (spotify:internal:temp_playlist:...).
function createPlaylist(name) {
    var persistent = typeof name === "string" && name.length > 0;
    var uri = persistent
        ? currentUser.uri + ":playlist:" + spotifyId("new:" + Date.now() + ":" + Math.random())
        : "spotify:internal:temp_playlist:" + (++tempCounter);
    var playlist = register({
        uri: uri,
        name: persistent ? name : "",
        description: "",
        owner: ref(currentUser),
        collaborative: false,
        subscribed: persistent,
        cover: null,
        tracks: [],
        type: "playlist",
        temporary: !persistent
    });
    if (persistent) {
        library.playlists.push(uri);
        saveLibrary();
    }
    return playlist;
}

function playlistChanged(playlist) {
    playlist.cover = playlistCover(playlist);
    if (!playlist.temporary && library.playlists.indexOf(playlist.uri) !== -1) saveLibrary();
}

function setStarred(uri, starred) {
    var track = lookup(uri);
    if (!track) return;
    track.starred = !!starred;
    if (starred) library.starred[track.uri] = true;
    else delete library.starred[track.uri];
    saveLibrary();
}

function setSubscribed(uri, subscribed) {
    var playlist = lookup(uri);
    if (!playlist) return;
    playlist.subscribed = !!subscribed;
    var index = library.playlists.indexOf(playlist.uri);
    if (subscribed && index === -1) library.playlists.push(playlist.uri);
    if (!subscribed && index !== -1) library.playlists.splice(index, 1);
    saveLibrary();
}

loadLibrary();

exports.isMock = true;
exports.normalizeUri = normalizeUri;
exports.typeOf = typeOf;
exports.lookup = function (uri) {
    var entity = lookup(uri);
    if (entity && entity.type === "track") entity.starred = !!library.starred[entity.uri];
    return entity;
};
exports.search = search;
exports.parseQuery = parseQuery;
exports.createPlaylist = createPlaylist;
exports.playlistChanged = playlistChanged;
exports.setStarred = setStarred;
exports.setSubscribed = setSubscribed;
exports.currentUser = function () { return currentUser; };
exports.libraryPlaylists = function () {
    return library.playlists.map(function (uri) { return entities[uri]; });
};
exports.starredTracks = function () {
    build();
    return Object.keys(library.starred).map(lookup).filter(Boolean);
};
exports.artwork = artwork;
