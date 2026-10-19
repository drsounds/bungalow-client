/**
 * @module views
 *
 * sp://import/scripts/api/views — DOM views of the Apps API, version 1:
 * Image, Player, Track and List. Each view exposes its element as `.node`.
 * Styling lives in sp://import/css/api.css.
 *
 * DOM shape (apps reach into it, so keep it stable):
 *   List    div.sp-list > div.sp-list-items > a.sp-item.sp-track ...
 *   Track   a.sp-item.sp-track > span.sp-track-field-<field> ...
 *   Player  div.sp-player > a.sp-player-image + button.sp-player-button
 *   Image   a.sp-image (span.sp-image when there is no uri) > span.sp-image-inner
 */

var models = sp.require("sp://import/scripts/api/models");

function el(tag, className, text) {
    var node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
}

function cssUrl(src) { return src ? 'url("' + String(src).replace(/"/g, '\\"') + '")' : "none"; }

function formatDuration(ms) {
    var s = Math.round((ms || 0) / 1000);
    var m = Math.floor(s / 60);
    s = s % 60;
    return m + ":" + (s < 10 ? "0" : "") + s;
}

function sameUri(a, b) { return !!a && !!b && a.uri === b.uri; }

// ── Image ────────────────────────────────────────────────────────────────

/** new Image(src, [uri], [title]) — a cover/portrait, linking to uri. */
function Image(src, uri, title) {
    this.src = src;
    this.uri = uri;
    this.node = el(uri ? "a" : "span", "sp-image");
    if (uri) this.node.setAttribute("href", uri);
    if (title) this.node.setAttribute("title", title);
    this.image = el("span", "sp-image-inner");
    this.node.appendChild(this.image);
    this.setImage(src);
}
Image.prototype.setImage = function (src) {
    this.src = src;
    this.image.style.backgroundImage = cssUrl(src);
    this.node.classList.toggle("sp-image-empty", !src);
};

// ── Player ───────────────────────────────────────────────────────────────

// Up to four distinct album covers of a playlist, for its mosaic.
function coversOf(item) {
    if (!item || !item.data) return [];
    if (item.data.type === "album") return [item.data.cover];
    if (item.data.type === "track") return [item.data.album.cover];
    if (item.data.type === "artist") return [item.data.portrait];
    var seen = {}, covers = [];
    (item.tracks || []).some(function (t) {
        var c = t && t.data.album.cover;
        if (c && !seen[c]) { seen[c] = true; covers.push(c); }
        return covers.length >= 4;
    });
    if (!covers.length && item.data.cover) covers.push(item.data.cover);
    return covers.length >= 4 ? covers : covers.slice(0, 1);
}

/**
 * new Player([context]) — cover with a play button. Set `.context` (album,
 * playlist or artist) and optionally `.track` (where playback starts).
 */
function Player(context) {
    var self = this;
    this.node = el("div", "sp-player");
    this.link = el("a", "sp-player-image");
    this.button = el("button", "sp-player-button");
    this.button.setAttribute("type", "button");
    this.button.setAttribute("aria-label", "Play");
    this.node.appendChild(this.link);
    this.node.appendChild(this.button);

    this._context = null;
    this._track = null;
    this._onContextChange = function () { self._render(); };

    this.button.addEventListener("click", function (e) {
        e.preventDefault();
        e.stopPropagation();
        self.toggle();
    });
    this.link.addEventListener("dblclick", function (e) {
        e.preventDefault();
        self.play();
    });
    models.player.observe(models.EVENT.CHANGE, function () { self._updateState(); });

    if (context) this.context = context;
}
Object.defineProperties(Player.prototype, {
    context: {
        get: function () { return this._context; },
        set: function (value) {
            if (this._context && this._context.ignore) this._context.ignore(models.EVENT.CHANGE, this._onContextChange);
            this._context = value || null;
            if (this._context && this._context.observe) this._context.observe(models.EVENT.CHANGE, this._onContextChange);
            this._render();
        }
    },
    track: {
        get: function () { return this._track; },
        set: function (value) { this._track = value || null; this._updateState(); }
    }
});
Player.prototype._render = function () {
    var ctx = this._context;
    this.link.innerHTML = "";
    if (!ctx) {
        this.link.removeAttribute("href");
        this._updateState();
        return;
    }
    if (ctx.uri) this.link.setAttribute("href", ctx.uri);
    var name = ctx.data && ctx.data.name;
    if (name) this.link.setAttribute("title", name);
    var covers = coversOf(ctx);
    this.link.classList.toggle("sp-player-mosaic", covers.length === 4);
    covers.forEach(function (src) {
        var tile = el("span", "sp-player-cover");
        tile.style.backgroundImage = cssUrl(src);
        this.link.appendChild(tile);
    }, this);
    this._updateState();
};
Player.prototype._isCurrent = function () {
    var p = models.player;
    return !!this._context && sameUri(p.context, this._context);
};
Player.prototype._updateState = function () {
    var current = this._isCurrent();
    var playing = current && models.player.playing;
    this.node.classList.toggle("sp-player-current", current);
    this.node.classList.toggle("playing", playing);
    this.node.classList.toggle("paused", current && !playing);
    this.button.setAttribute("aria-label", playing ? "Pause" : "Play");
};
Player.prototype.play = function () {
    if (!this._context && !this._track) return;
    if (this._context && this._context.data.type === "artist") {
        // Artists play their first album.
        var album = this._context.albums[0];
        models.player.play(this._track || (album && album.get(0)), album);
        return;
    }
    models.player.play(this._track || null, this._context);
};
Player.prototype.toggle = function () {
    if (this._isCurrent()) models.player.playing = !models.player.playing;
    else this.play();
};

// ── Track ────────────────────────────────────────────────────────────────

/**
 * new Track(track, fields) — one list row. fields is a bitmask of
 * Track.FIELD; the row exposes `.track`, and `.context` once in a List.
 */
function Track(track, fields) {
    var self = this;
    this.track = track;
    this.fields = fields === undefined ? Track.FIELD.DEFAULT : fields;
    this.context = null;
    this.node = el("a", "sp-item sp-track");
    this.node.setAttribute("data-uri", track.uri);
    this.node.setAttribute("tabindex", "-1");
    this.node.setAttribute("draggable", "true");
    this.node.__spTrackView = this;

    var data = track.data;
    var F = Track.FIELD;
    var add = function (flag, name, build) {
        if (!(self.fields & flag)) return;
        var span = el("span", "sp-track-field-" + name);
        build(span);
        self.node.appendChild(span);
    };
    add(F.STAR, "star", function (s) {
        s.setAttribute("role", "button");
        s.setAttribute("title", "Star");
    });
    add(F.SHARE, "share", function (s) {
        s.setAttribute("role", "button");
        s.setAttribute("title", "Share");
    });
    add(F.NUMBER, "number", function (s) { s.textContent = data.number; });
    add(F.IMAGE, "image", function (s) { s.style.backgroundImage = cssUrl(data.album.cover); });
    add(F.NAME, "name", function (s) { s.textContent = data.name; s.title = data.name; });
    add(F.ARTIST, "artist", function (s) {
        data.artists.forEach(function (artist, i) {
            if (i) s.appendChild(document.createTextNode(", "));
            var a = el("a", null, artist.name);
            a.setAttribute("href", artist.uri);
            s.appendChild(a);
        });
    });
    add(F.DURATION, "duration", function (s) { s.textContent = formatDuration(data.duration); });
    add(F.ALBUM, "album", function (s) {
        var a = el("a", null, data.album.name);
        a.setAttribute("href", data.album.uri);
        s.appendChild(a);
    });
    add(F.POPULARITY, "popularity", function (s) {
        var bar = el("span", "sp-popularity-bar");
        bar.style.width = (data.popularity || 0) + "%";
        s.appendChild(bar);
    });
    this._updateStar();

    // Delegated on the row: apps may rewrite node.innerHTML after creation.
    this.node.addEventListener("click", function (e) {
        if (e.target.closest(".sp-track-field-star")) {
            e.preventDefault();
            e.stopPropagation();
            self.track.starred = !self.track.starred;
            self._updateStar();
            return;
        }
        if (e.target.closest(".sp-track-field-share")) {
            e.preventDefault();
            e.stopPropagation();
            self.node.dispatchEvent(new CustomEvent("sp-share", { bubbles: true, detail: { track: self.track } }));
            return;
        }
        self.select();
    });
    this.node.addEventListener("dblclick", function (e) {
        if (e.target.closest("a[href]")) return;
        e.preventDefault();
        self.play();
    });
    this.node.addEventListener("keydown", function (e) {
        if (e.key === "Enter") { e.preventDefault(); self.play(); }
    });
    this.node.addEventListener("dragstart", function (e) {
        e.dataTransfer.setData("text/uri-list", self.track.uri);
        e.dataTransfer.setData("text/plain", self.track.uri);
    });
}
Track.FIELD = {
    NAME: 1,
    ARTIST: 2,
    DURATION: 4,
    STAR: 8,
    SHARE: 16,
    ALBUM: 32,
    NUMBER: 64,
    POPULARITY: 128,
    IMAGE: 256
};
Track.FIELD.DEFAULT = Track.FIELD.STAR | Track.FIELD.SHARE | Track.FIELD.NAME |
                      Track.FIELD.ARTIST | Track.FIELD.DURATION | Track.FIELD.ALBUM;
Track.FIELD.ALL = 511;

Track.prototype._updateStar = function () {
    var star = this.node.querySelector(".sp-track-field-star");
    if (star) star.classList.toggle("starred", !!this.track.starred);
    this.node.classList.toggle("sp-track-starred", !!this.track.starred);
};
Track.prototype.select = function () {
    var parent = this.node.parentNode;
    if (parent) {
        Array.prototype.forEach.call(parent.querySelectorAll(".sp-selected"), function (n) {
            n.classList.remove("sp-selected");
        });
    }
    this.node.classList.add("sp-selected");
    this.node.focus({ preventScroll: true });
};
Track.prototype.play = function () {
    models.player.play(this.track, this.context || undefined);
};
Track.prototype.setPlaying = function (playing, paused) {
    this.node.classList.toggle("sp-track-playing", !!playing);
    this.node.classList.toggle("sp-track-paused", !!paused);
};

// ── List ─────────────────────────────────────────────────────────────────

/**
 * new List(collection, [render]) — rows for a Playlist, Album, Search or
 * array of tracks. render(track) returns a views.Track (or a DOM node);
 * it defaults to new Track(track, Track.FIELD.DEFAULT). Rows are redrawn
 * when the collection fires CHANGE.
 */
function List(collection, render) {
    var self = this;
    this.collection = collection;
    this.render = render || function (track) { return new Track(track, Track.FIELD.DEFAULT); };
    this.node = el("div", "sp-list");
    this.items = el("div", "sp-list-items");
    this.node.appendChild(this.items);
    this.views = [];

    if (collection && typeof collection.observe === "function") {
        collection.observe(models.EVENT.CHANGE, function () { self.refresh(); });
    }
    models.player.observe(models.EVENT.CHANGE, function () { self._updatePlaying(); });
    this.refresh();
}
List.prototype._tracks = function () {
    var c = this.collection;
    if (!c) return [];
    if (Array.isArray(c)) return c;
    return c.tracks || [];
};
List.prototype.refresh = function () {
    // Only remove rows this list created; apps may append their own nodes
    // (e.g. a table header) to the items element.
    this.views.forEach(function (view) {
        var node = view.node || view;
        if (node.parentNode === this.items) this.items.removeChild(node);
    }, this);
    this.views = [];

    var context = Array.isArray(this.collection) ? null : this.collection;
    var firstAppNode = this.items.firstChild;
    this._tracks().forEach(function (track, index) {
        if (!track) return;
        var view;
        try {
            view = this.render(track, index);
        } catch (e) {
            console.error("List render failed:", e);
        }
        if (!view) return;
        var node = view.node || view;
        if (!(node instanceof Node)) return;
        if (view instanceof Track) view.context = context;
        node.classList.add("sp-list-row");
        node.setAttribute("data-itemindex", index);
        node.setAttribute("data-viewindex", index);
        this.items.insertBefore(node, firstAppNode);
        this.views.push(view);
    }, this);
    this.node.classList.toggle("sp-list-empty", this.views.length === 0);
    this._updatePlaying();
};
List.prototype._updatePlaying = function () {
    var p = models.player;
    var context = Array.isArray(this.collection) ? null : this.collection;
    var sameContext = !context || sameUri(p.context, context);
    this.views.forEach(function (view) {
        if (!(view instanceof Track)) return;
        var current = sameContext && sameUri(p.track, view.track);
        view.setPlaying(current && p.playing, current && !p.playing);
    });
};
List.prototype.get = function (index) { return this.views[index]; };
Object.defineProperty(List.prototype, "length", { get: function () { return this.views.length; } });

exports.Image = Image;
exports.Player = Player;
exports.Track = Track;
exports.List = List;
exports.formatDuration = formatDuration;
