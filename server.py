#! /usr/bin/python

import gobject
import dbus, dbus.service, dbus.mainloop.glib
from dbus.exceptions import *
from scrobbler2 import Scrobbler

class Submission:
    def __init__(self, timestamp, artist, title, track, length, album, musicbrainz, source):
        if timestamp == 0:
            raise Exception("Timestamp must be set")
        if artist == "":
            raise Exception("Artist must be set")
        if title == "":
            raise Exception("Title must be set")
        if source == "P" and length == 0:
            raise Exception("Track length required when source is P")
        if source not in ("P", "R", "E", "U"):
            raise Exception("Unknown source '%s'" % source)

        self.timestamp = timestamp
        self.artist = artist
        self.title = title
        self.track = track
        self.length = length
        self.album = album
        self.musicbrainz = musicbrainz
        self.source = source
    
    def __str__(self):
        #self.timestamp, self.artist, self.album, self.title, self.track, self.length, self.musicbrainz, self.source
        return "Timestamp: %d\nArtist: %s\nTitle: %s" % (self.timestamp, self.artist, self.title)
        
class ScrobblerService(dbus.service.Object):
    def __init__(self, service, object_path="/com/burtonini/Scrobbler"):
        dbus.service.Object.__init__(self, service, object_path)
        self.scrobbler = Scrobbler("username", "password")
        self.scrobbler.handshake()
        self.queue = []
        
    @dbus.service.method(dbus_interface='com.burtonini.Scrobbler',
                         in_signature='usssuuss', out_signature='',
                         utf8_strings=False)
    def Submit(self, timestamp, artist, title, track, length=0, album="", musicbrainz="", source="U"):
        s = Submission(timestamp, artist, title, track, length, album, musicbrainz, source)
        print s

        # Silently ignore submissions less than 30 seconds
        if s.length and s.length < 30:
            return
        
        self.queue.append(s)

    @dbus.service.method(dbus_interface='com.burtonini.Scrobbler')
    def Flush(self):
        for s in self.queue:
            print "Sending %s" % s.title
            self.scrobbler.submit(s.artist, s.title, s.album, s.musicbrainz, s.length, s.track, s.timestamp)

if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("com.burtonini.Scrobbler", session_bus)
    s = ScrobblerService(name)
    
    gobject.MainLoop().run()
