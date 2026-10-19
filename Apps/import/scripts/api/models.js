/**
 * @module models
 *
 * sp://import/scripts/api/models — data model of the Apps API, version 1:
 * application, session, player, library and the Track, Album, Artist,
 * Playlist, User, Search and Link classes.
 *
 * Metadata comes from ./datasource (fictive for now). Classes wrap the plain
 * data objects it returns in `.data`, and expose the common fields as
 * properties too (track.name, album.year, ...).
 */

var datasource = sp.require("sp://import/scripts/api/datasource");

var EVENT = {
    ACTIVATE: "activate",
    ARGUMENTSCHANGED: "argumentschanged",
    CHANGE: "change",
    DEACTIVATE: "deactivate",
    ITEMS_ADDED: "itemsAdded",
    ITEMS_REMOVED: "itemsRemoved",
    LINKSCHANGED: "linkschanged",
    LOAD: "load",
    LOAD_ERROR: "loadError",
    RENAME: "rename",
    STATECHANGED: "statechanged"
};

function async(fn) { setTimeout(fn, 0); }

// ── Observable ───────────────────────────────────────────────────────────

/**
 * Base of everything that emits events. Listeners live on the instance
 * (created lazily), so `Sub.prototype = new Observable()` is safe.
 * Callbacks get one event object: { type, data, target }.
 */
function Observable() {}

Observable.prototype._listeners = function (type) {
    if (!Object.prototype.hasOwnProperty.call(this, "__listeners"))
        Object.defineProperty(this, "__listeners", { value: {}, enumerable: false });
    return this.__listeners[type] || (this.__listeners[type] = []);
};

Observable.prototype.observe = function (type, callback) {
    if (typeof callback === "function") this._listeners(type).push(callback);
    return this;
};

Observable.prototype.ignore = function (type, callback) {
    var list = this._listeners(type);
    if (callback === undefined) list.length = 0;
    else {
        var i = list.indexOf(callback);
        if (i !== -1) list.splice(i, 1);
    }
    return this;
};

Observable.prototype.notify = function (type, data) {
    var event = { type: type, data: data, target: this };
    this._listeners(type).slice().forEach(function (callback) {
        try {
            callback.call(this, event);
        } catch (e) {
            console.error("Error in " + type + " observer:", e);
        }
    }, this);
};

// Shorthand: subclass with prototype methods from an object.
function extend(Base, Sub, methods) {
    Sub.prototype = Object.create(Base.prototype);
    Sub.prototype.constructor = Sub;
    Object.keys(methods || {}).forEach(function (key) {
        var desc = Object.getOwnPropertyDescriptor(methods, key);
        Object.defineProperty(Sub.prototype, key, desc);
    });
    return Sub;
}

// ── Entity cache: one instance per URI ───────────────────────────────────

var instances = {};

function wrap(data) {
    if (!data) return null;
    var existing = instances[data.uri];
    if (existing) return existing;
    switch (data.type) {
    case "track": return new Track(data);
    case "album": return new Album(data);
    case "artist": return new Artist(data);
    case "playlist": return new Playlist(data);
    case "user": return new User(data);
    default: return null;
    }
}

function resolve(uriOrItem) {
    if (!uriOrItem) return null;
    if (uriOrItem instanceof Loadable) return uriOrItem;
    if (typeof uriOrItem === "object" && uriOrItem.uri) return wrap(datasource.lookup(uriOrItem.uri));
    return wrap(datasource.lookup(String(uriOrItem)));
}

// fromURI(uri, callback): returns the instance at once and also calls back
// asynchronously, like the original API. Unknown/invalid URIs call back with
// null and fire LOAD_ERROR-style logging.
function fromURI(expectedType) {
    return function (uri, callback) {
        var item = null;
        try {
            item = resolve(uri);
        } catch (e) {
            console.error("fromURI(" + uri + ") failed:", e);
        }
        if (item && expectedType && item.data.type !== expectedType) {
            console.warn("fromURI: " + uri + " is a " + item.data.type + ", not a " + expectedType);
        }
        if (typeof callback === "function") async(function () { callback(item); });
        return item;
    };
}

// ── Loadable entities ────────────────────────────────────────────────────

function Loadable(data) {
    this.data = data;
    instances[data.uri] = this;
}
extend(Observable, Loadable, {
    get uri() { return this.data.uri; },
    get name() { return this.data.name; },
    get loaded() { return true; },
    toString: function () { return this.data.uri; },
    toURI: function () { return this.data.uri; },
    toURL: function () { return new Link(this.data.uri).toURL(); }
});

function Track(data) { Loadable.call(this, data); }
extend(Loadable, Track, {
    get artists() { return this.data.artists.map(resolve); },
    get album() { return resolve(this.data.album.uri); },
    get duration() { return this.data.duration; },
    get popularity() { return this.data.popularity; },
    get number() { return this.data.number; },
    get playable() { return this.data.playable; },
    get availableForPlayback() { return this.data.playable; },
    get local() { return this.data.local; },
    get explicit() { return this.data.explicit; },
    get image() { return this.data.album.cover; },
    get starred() { return this.data.starred; },
    set starred(value) {
        datasource.setStarred(this.uri, value);
        this.notify(EVENT.CHANGE, { starred: !!value });
        library.notify(EVENT.CHANGE, { starred: this });
    }
});
Track.fromURI = fromURI("track");

function Album(data) { Loadable.call(this, data); }
extend(Loadable, Album, {
    get artist() { return resolve(this.data.artist.uri); },
    get year() { return this.data.year; },
    get cover() { return this.data.cover; },
    get image() { return this.data.cover; },
    get genre() { return this.data.genre; },
    get albumType() { return this.data.albumType; }, // "album" | "single" | "compilation"
    get tracks() { return this.data.tracks.map(resolve); },
    get length() { return this.data.tracks.length; },
    get: function (index) { return resolve(this.data.tracks[index]); },
    indexOf: function (track) {
        return this.data.tracks.indexOf(typeof track === "string" ? datasource.normalizeUri(track) : track && track.uri);
    }
});
Album.fromURI = fromURI("album");

function Artist(data) { Loadable.call(this, data); }
extend(Loadable, Artist, {
    get portrait() { return this.data.portrait; },
    get image() { return this.data.portrait; },
    get genres() { return this.data.genres.slice(); },
    get popularity() { return this.data.popularity; },
    get biography() { return this.data.biography; },
    get albums() { return this.data.albums.map(resolve); }
});
Artist.fromURI = fromURI("artist");

function User(data) { Loadable.call(this, data); }
extend(Loadable, User, {
    get username() { return this.data.username; },
    get displayName() { return this.data.name; },
    get picture() { return this.data.picture; },
    get image() { return this.data.picture; }
});
User.fromURI = fromURI("user");

/**
 * new Playlist()      temporary playlist, not in the library
 * new Playlist(name)  new playlist in the user's library
 * Playlist.fromURI(uri, callback)
 */
function Playlist(nameOrData) {
    var data = nameOrData && typeof nameOrData === "object" && nameOrData.type === "playlist"
        ? nameOrData
        : datasource.createPlaylist(nameOrData);
    Loadable.call(this, data);
    if (!data.temporary && typeof nameOrData === "string")
        async(function () { library.notify(EVENT.CHANGE, { playlistAdded: this }); }.bind(this));
}
extend(Loadable, Playlist, {
    get name() { return this.data.name; },
    set name(value) {
        this.data.name = String(value);
        datasource.playlistChanged(this.data);
        this.notify(EVENT.RENAME, { name: this.data.name });
        this.notify(EVENT.CHANGE, { renamed: true });
    },
    get owner() { return resolve(this.data.owner.uri); },
    get cover() { return this.data.cover; },
    get image() { return this.data.cover; },
    get tracks() { return this.data.tracks.map(resolve); },
    get length() { return this.data.tracks.length; },
    get subscribed() { return this.data.subscribed; },
    set subscribed(value) {
        datasource.setSubscribed(this.uri, value);
        this.notify(EVENT.CHANGE, { subscribed: !!value });
        library.notify(EVENT.CHANGE, { subscribed: this });
    },
    get collaborative() { return this.data.collaborative; },
    set collaborative(value) {
        this.data.collaborative = !!value;
        datasource.playlistChanged(this.data);
        this.notify(EVENT.CHANGE, { collaborative: !!value });
    },
    get temporary() { return !!this.data.temporary; },
    get: function (index) { return resolve(this.data.tracks[index]); },
    indexOf: function (track) {
        var uri = typeof track === "string" ? datasource.normalizeUri(track) : track && track.uri;
        return this.data.tracks.indexOf(uri);
    },
    /** add(track | uri | [tracks]) — appends and fires ITEMS_ADDED + CHANGE. */
    add: function (items) {
        var list = Array.isArray(items) ? items : [items];
        var added = [];
        list.forEach(function (item) {
            var track = resolve(item);
            if (!track || track.data.type !== "track") {
                console.warn("Playlist.add: not a track:", item);
                return;
            }
            this.data.tracks.push(track.uri);
            added.push(track);
        }, this);
        if (!added.length) return;
        datasource.playlistChanged(this.data);
        this.notify(EVENT.ITEMS_ADDED, { tracks: added });
        this.notify(EVENT.CHANGE, { added: added });
    },
    /** remove(track | uri | index) — removes the first match. */
    remove: function (item) {
        var index = typeof item === "number" ? item : this.indexOf(item);
        if (index < 0 || index >= this.data.tracks.length) return;
        var removed = resolve(this.data.tracks.splice(index, 1)[0]);
        datasource.playlistChanged(this.data);
        this.notify(EVENT.ITEMS_REMOVED, { tracks: [removed], index: index });
        this.notify(EVENT.CHANGE, { removed: [removed] });
    },
    clear: function () {
        var removed = this.tracks;
        this.data.tracks.length = 0;
        datasource.playlistChanged(this.data);
        this.notify(EVENT.ITEMS_REMOVED, { tracks: removed });
        this.notify(EVENT.CHANGE, { removed: removed });
    }
});
Playlist.fromURI = fromURI("playlist");

// ── Search ───────────────────────────────────────────────────────────────

/**
 * new Search(query, { pageSize, searchTracks, searchAlbums, searchArtists,
 *                     searchPlaylists })
 * Call appendNext() to load each page; CHANGE fires after every page (also
 * when it is empty), LOAD_ERROR if the backend fails.
 */
function Search(query, options) {
    options = options || {};
    this.query = String(query === undefined ? "" : query).trim();
    this.pageSize = options.pageSize || 50;
    this.searchTracks = options.searchTracks !== false;
    this.searchAlbums = options.searchAlbums !== false;
    this.searchArtists = options.searchArtists !== false;
    this.searchPlaylists = options.searchPlaylists !== false;
    this.tracks = [];
    this.albums = [];
    this.artists = [];
    this.playlists = [];
    this.totalTracks = 0;
    this.totalAlbums = 0;
    this.totalArtists = 0;
    this.totalPlaylists = 0;
    this.loading = false;
    this._offset = 0;
}
extend(Observable, Search, {
    get complete() {
        return this._offset > 0 &&
            this.tracks.length >= this.totalTracks && this.albums.length >= this.totalAlbums &&
            this.artists.length >= this.totalArtists && this.playlists.length >= this.totalPlaylists;
    },
    appendNext: function () {
        var self = this;
        self.loading = true;
        async(function () {
            var result;
            try {
                result = datasource.search(self.query, {
                    offset: self._offset,
                    pageSize: self.pageSize,
                    searchTracks: self.searchTracks,
                    searchAlbums: self.searchAlbums,
                    searchArtists: self.searchArtists,
                    searchPlaylists: self.searchPlaylists
                });
            } catch (e) {
                self.loading = false;
                console.error("Search failed:", e);
                self.notify(EVENT.LOAD_ERROR, { error: e });
                return;
            }
            self._offset += self.pageSize;
            var tracks = result.tracks.map(wrap), albums = result.albums.map(wrap);
            var artists = result.artists.map(wrap), playlists = result.playlists.map(wrap);
            Array.prototype.push.apply(self.tracks, tracks);
            Array.prototype.push.apply(self.albums, albums);
            Array.prototype.push.apply(self.artists, artists);
            Array.prototype.push.apply(self.playlists, playlists);
            self.totalTracks = result.totalTracks;
            self.totalAlbums = result.totalAlbums;
            self.totalArtists = result.totalArtists;
            self.totalPlaylists = result.totalPlaylists;
            self.loading = false;
            self.notify(EVENT.LOAD, null);
            self.notify(EVENT.CHANGE, { tracks: tracks, albums: albums, artists: artists, playlists: playlists });
        });
    }
});

// ── Link ─────────────────────────────────────────────────────────────────

var LINK_TYPE = {
    EMPTY: 0, ARTIST: 1, ALBUM: 2, TRACK: 4, PLAYLIST: 5, SEARCH: 6, PROFILE: 9,
    STARRED: 10, APPLICATION: 11, LOCAL_TRACK: 14, INTERNAL: 17
};

function Link(uri) {
    this.uri = datasource.normalizeUri(uri);
    var parts = this.uri.split(":");
    switch (parts[1]) {
    case "artist": this.type = LINK_TYPE.ARTIST; break;
    case "album": this.type = LINK_TYPE.ALBUM; break;
    case "track": this.type = LINK_TYPE.TRACK; break;
    case "local": this.type = LINK_TYPE.LOCAL_TRACK; break;
    case "search": this.type = LINK_TYPE.SEARCH; break;
    case "app": this.type = LINK_TYPE.APPLICATION; break;
    case "internal": this.type = LINK_TYPE.INTERNAL; break;
    case "user":
        this.type = parts[3] === "playlist" ? LINK_TYPE.PLAYLIST
                  : parts[3] === "starred" ? LINK_TYPE.STARRED : LINK_TYPE.PROFILE;
        break;
    default: this.type = LINK_TYPE.EMPTY;
    }
}
Link.TYPE = LINK_TYPE;
Link.prototype.toString = function () { return this.uri; };
Link.prototype.toURI = function () { return this.uri; };
Link.prototype.toURL = function () {
    return "http://open.spotify.com/" + this.uri.split(":").slice(1).map(encodeURIComponent).join("/");
};

// ── Player ───────────────────────────────────────────────────────────────

/**
 * Simulated player: keeps state and advances position on a timer, moving
 * through the context. No audio until the C++ bridge is connected.
 * CHANGE carries data { curtrack, curcontext, playstate, volume, shuffle, repeat }.
 */
function Player() {
    this.track = null;
    this.context = null;
    this.index = -1;
    this.playing = false;
    this.position = 0;
    this.volume = 1;
    this.shuffle = false;
    this.repeat = false;
    this._timer = null;
    this._tickAt = 0;
}
extend(Observable, Player, {
    _change: function (what) {
        this.notify(EVENT.CHANGE, {
            curtrack: !!what.curtrack, curcontext: !!what.curcontext, playstate: !!what.playstate,
            volume: !!what.volume, shuffle: !!what.shuffle, repeat: !!what.repeat
        });
    },
    _contextTracks: function () {
        if (!this.context) return this.track ? [this.track] : [];
        if (Array.isArray(this.context)) return this.context.map(resolve);
        return this.context.tracks || [];
    },
    _run: function (on) {
        clearInterval(this._timer);
        this._timer = null;
        if (!on) return;
        var self = this;
        self._tickAt = Date.now();
        self._timer = setInterval(function () {
            var now = Date.now();
            self.position += now - self._tickAt;
            self._tickAt = now;
            if (self.track && self.position >= self.track.duration) self.next();
        }, 500);
    },
    /**
     * play(track | uri, [context], [index]) — context may be an Album,
     * Playlist, array of tracks or a URI. play(context) starts the context.
     */
    play: function (item, context, index) {
        var ctx = context === undefined || context === null ? null : resolve(context) || context;
        var target = resolve(item);
        if (target && (target.data.type === "album" || target.data.type === "playlist") && !ctx) {
            ctx = target;
            target = null;
        }
        var list = ctx ? (Array.isArray(ctx) ? ctx.map(resolve) : ctx.tracks || []) : [];
        if (typeof index === "number" && list[index]) target = list[index];
        if (!target) target = list[0] || null;
        if (!target) return;

        var trackChanged = this.track !== target, contextChanged = this.context !== ctx;
        this.track = target;
        this.context = ctx;
        this.index = list.indexOf(target);
        this.position = 0;
        var wasPlaying = this._playing;
        this._playing = true; // bypass the setter: one CHANGE below covers it
        this._run(true);
        this._change({ curtrack: trackChanged, curcontext: contextChanged, playstate: !wasPlaying });
    },
    playTrackFromContext: function (context, index) { this.play(null, context, index); },
    pause: function () { this.playing = false; },
    next: function () {
        var list = this._contextTracks();
        if (!list.length) { this.playing = false; return; }
        var i = this.shuffle ? Math.floor(Math.random() * list.length) : this.index + 1;
        if (i >= list.length) {
            if (!this.repeat) { this.playing = false; this.position = 0; return; }
            i = 0;
        }
        this.index = i;
        this.track = list[i];
        this.position = 0;
        this._change({ curtrack: true });
    },
    previous: function () {
        var list = this._contextTracks();
        if (this.position > 3000 || this.index <= 0 || !list.length) {
            this.position = 0;
            this._change({});
            return;
        }
        this.index -= 1;
        this.track = list[this.index];
        this.position = 0;
        this._change({ curtrack: true });
    },
    seek: function (ms) { this.position = Math.max(0, ms); this._change({}); }
});

// playing/volume/shuffle/repeat are settable properties that fire CHANGE.
(function () {
    var proto = Player.prototype;
    function stateProperty(name, flag, onSet) {
        var key = "_" + name;
        Object.defineProperty(proto, name, {
            get: function () { return this[key]; },
            set: function (value) {
                var changed = this[key] !== value;
                this[key] = value;
                if (onSet) onSet.call(this, value);
                if (changed && this._ready) { var w = {}; w[flag] = true; this._change(w); }
            }
        });
    }
    stateProperty("playing", "playstate", function (value) { if (this._ready) this._run(!!value && !!this.track); });
    stateProperty("volume", "volume");
    stateProperty("shuffle", "shuffle");
    stateProperty("repeat", "repeat");
})();

var player = new Player();
player._ready = true;

// ── Session ──────────────────────────────────────────────────────────────

var SESSION_STATE = { LOGGED_OUT: 0, LOGGED_IN: 1, OFFLINE: 2, DISCONNECTED: 3 };

function Session() {}
extend(Observable, Session, {
    get state() { return navigator.onLine === false ? SESSION_STATE.OFFLINE : SESSION_STATE.LOGGED_IN; },
    get online() { return this.state === SESSION_STATE.LOGGED_IN; },
    get user() { return resolve(datasource.currentUser().uri); },
    get anonymousUserID() { return datasource.currentUser().username; },
    get country() { return sp.core.country; },
    get language() { return sp.core.language; },
    get resolution() { return window.devicePixelRatio || 1; },
    get developer() { return true; }
});
Session.STATE = SESSION_STATE;

var session = new Session();
window.addEventListener("online", function () { session.notify(EVENT.STATECHANGED, { state: session.state }); });
window.addEventListener("offline", function () { session.notify(EVENT.STATECHANGED, { state: session.state }); });

// ── Library ──────────────────────────────────────────────────────────────

function Library() {}
extend(Observable, Library, {
    get playlists() { return datasource.libraryPlaylists().map(wrap); },
    get starredTracks() { return datasource.starredTracks().map(wrap); },
    get tracks() { return this.starredTracks; },
    get albums() { return []; },
    get artists() { return []; }
});
var library = new Library();

// ── Application ──────────────────────────────────────────────────────────

/**
 * The running app. arguments mirror the in-app route:
 * spacify:app:timemachine:year:1990:2000 is served as /year/1990/2000 and
 * gives arguments ["year", "1990", "2000"] (segments stay URI-encoded).
 */
function Application() {
    this._arguments = argumentsFromLocation();
    this._active = document.visibilityState !== "hidden";
    this.links = [];
}
function argumentsFromLocation() {
    return sp.core.getRoute().split("/").filter(function (s) { return s.length > 0; });
}
extend(Observable, Application, {
    get arguments() { return this._arguments.slice(); },
    get uri() { return "spotify:app:" + sp.core.appId + (this._arguments.length ? ":" + this._arguments.join(":") : ""); },
    get player() { return player; },
    get active() { return this._active; },
    activate: function () {},
    hideLoadingScreen: function () {},
    showLoadingScreen: function () {},
    /** Opens a URI in the host (spotify:/spacify:/http links). */
    openURI: function (uri) { openUri(uri); },
    openApp: function (appId) { openUri("spotify:app:" + appId + Array.prototype.slice.call(arguments, 1).map(function (a) { return ":" + a; }).join("")); },
    /** Sets the in-app arguments without reloading the page. */
    setArguments: function (args) { navigateInApp(args); },
    setLinks: function (links) {
        this.links = (links || []).slice();
        this.notify(EVENT.LINKSCHANGED, { links: this.links });
    },
    _refreshArguments: function () {
        var args = argumentsFromLocation();
        if (args.join("/") === this._arguments.join("/")) return;
        this._arguments = args;
        this.notify(EVENT.ARGUMENTSCHANGED, { arguments: args.slice() });
    }
});

var application = new Application();

window.addEventListener("popstate", function () { application._refreshArguments(); });
document.addEventListener("visibilitychange", function () {
    var active = document.visibilityState !== "hidden";
    if (active === application._active) return;
    application._active = active;
    application.notify(active ? EVENT.ACTIVATE : EVENT.DEACTIVATE, null);
});

// ── Navigation between apps and views ────────────────────────────────────

function navigateInApp(args) {
    var path = sp.core.routeToUrl(args.map(String).join("/"));
    if (typeof window.navigate === "function") {
        window.navigate(path); // host bridge: pushState + popstate
    } else {
        history.pushState(null, "", path);
        window.dispatchEvent(new PopStateEvent("popstate", { state: null }));
    }
}

// Routes a spotify:-style URI: this app's own URIs change the in-app route
// (through the host, so the view stack records history); everything else
// goes to the host's view stack.
function openUri(uri) {
    uri = String(uri);
    if (/^https?:/i.test(uri)) {
        window.open(uri, "_blank");
        return;
    }
    var spotifyUri = datasource.normalizeUri(uri);
    var parts = spotifyUri.split(":");
    if (parts[0] !== "spotify") return;
    var spacifyUri = "spacify:" + parts.slice(1).join(":");
    var own = parts[1] === "app" && parts[2] && parts[2].toLowerCase() === String(sp.core.appId).toLowerCase();

    if (window.spacify && typeof window.spacify.navigate === "function") {
        window.spacify.navigate(spacifyUri);
    } else if (own) {
        navigateInApp(parts.slice(3));
    } else {
        console.log("[models] open (no host):", spacifyUri);
    }
}

function isAppUri(href) { return /^(spotify|spacify|bungalow|space|musik):/i.test(href || ""); }

// spotify: links and `location = "spotify:..."` would otherwise be ignored
// by the browser; route them through openUri.
document.addEventListener("click", function (e) {
    if (e.defaultPrevented || e.button !== 0) return;
    var a = e.target && e.target.closest ? e.target.closest("a[href]") : null;
    if (!a) return;
    var href = a.getAttribute("href");
    if (!isAppUri(href)) return;
    e.preventDefault();
    openUri(href);
}, true);

if (window.navigation && typeof window.navigation.addEventListener === "function") {
    window.navigation.addEventListener("navigate", function (e) {
        var url = e.destination && e.destination.url;
        if (!isAppUri(url) || !e.cancelable) return;
        e.preventDefault();
        openUri(url);
    });
}

// ── Misc ─────────────────────────────────────────────────────────────────

/** Reads a file of the current app synchronously, e.g. "templates/time.html". */
function readFile(path) {
    try {
        return sp.core.readFile(path);
    } catch (e) {
        console.error("readFile(" + path + ") failed:", e);
        return "";
    }
}

exports.EVENT = EVENT;
exports.Observable = Observable;
exports.Loadable = Loadable;
exports.Track = Track;
exports.Album = Album;
exports.Artist = Artist;
exports.User = User;
exports.Playlist = Playlist;
exports.Search = Search;
exports.Link = Link;
exports.Player = Player;
exports.Session = Session;
exports.application = application;
exports.player = player;
exports.session = session;
exports.library = library;
exports.readFile = readFile;
exports.fromURI = function (uri, callback) { return fromURI(null)(uri, callback); };
exports.isMock = datasource.isMock;
