#! /usr/bin/python

# TODO: version interfaces based on audioscrobbler protocol?

import gobject
import dbus, dbus.service, dbus.mainloop.glib
from dbus.exceptions import *
from scrobbler import Scrobbler, Submission

class ScrobblerService(dbus.service.Object):
    def __init__(self, service, object_path="/com/burtonini/Scrobbler"):
        dbus.service.Object.__init__(self, service, object_path)
        self.scrobbler = Scrobbler("username", "password")
        self.scrobbler.handshake()
        self.queue = []
        # TODO: restore queue from disk
        
    @dbus.service.method(
        dbus_interface='com.burtonini.Scrobbler',
        in_signature='usssuuss',
        out_signature='')
    def Submit(self, timestamp, artist, title, track, length=0, album="", musicbrainz="", source="P"):
        s = Submission(int(timestamp),
                       artist, title,
                       int(track), int(length),
                       album, musicbrainz,
                       source)

        # Silently ignore submissions less than 30 seconds
        if s.length and s.length < 30:
            return

        self.queue.append(s)
        # TODO: persist queue
        
    @dbus.service.method(dbus_interface='com.burtonini.Scrobbler')
    def Flush(self):
        self.scrobbler.submit(self.queue)


if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("com.burtonini.Scrobbler", session_bus)
    s = ScrobblerService(name)
    
    gobject.MainLoop().run()
