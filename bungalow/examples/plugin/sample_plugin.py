"""A complete Bungalow plugin in Python: one streaming service with a sign-in.

Drop a file like this into <app>/Apps or ~/Bungalow. It is the Python twin of
SamplePlugin.cpp, and is loaded by PythonPluginTest.

The service is a stand-in: it "plays" pysample: URIs by reporting playback
states, knows one recording and signs in without asking anything.
"""
import bungalow

ID = "se.bungalow.pysample"
NAME = "Python Sample Plugin"
VERSION = "1.0.0"

TRACK = bungalow.Track("pysample:track:1", "Python Song", "Python Artist")


class SampleStream(bungalow.Service):
    id = "pysample.stream"
    name = "Python Sample Stream"

    def __init__(self):
        self._position_ms = 0

    # Sign-in. A real service would open its sign-in page here and call
    # done.success() or done.error() later, from any thread.
    def login(self, done):
        done.success(bungalow.Session("u7", "Python User", {"token": "py-secret"}))

    # Playback
    def accepts_uri(self, uri):
        return uri.startswith("pysample:")

    def load_uri(self, uri):
        if uri != TRACK.uri:
            self.playback.error("No such sample track")
            return
        self.playback.state(bungalow.LOADING)
        self.playback.track(TRACK.title, TRACK.artist, "Python Album")

    def play(self):
        self.playback.state(bungalow.PLAYING)

    def pause(self):
        self.playback.state(bungalow.PAUSED)

    def stop(self):
        self.playback.state(bungalow.STOPPED)

    def seek(self, position_ms):
        self._position_ms = position_ms

    def position_ms(self):
        return self._position_ms

    def duration_ms(self):
        return 180000

    # Music lookups run on a background thread.
    def lookup(self, isrc):
        return TRACK if isrc == "PYSAMPLE00001" else None

    def lookup_by_title_artist(self, title, artist):
        return TRACK.uri if title == TRACK.title else None


def activate(context):
    context.register_service(SampleStream())


def deactivate():
    pass
