#! /usr/bin/python

# TODO: version interfaces based on audioscrobbler protocol?

import gobject, gconf
import dbus, dbus.service, dbus.mainloop.glib
from scrobbler import Scrobbler, Submission

class ScrobblerService(dbus.service.Object):
    def __init__(self, service, object_path="/com/burtonini/Scrobbler"):
        dbus.service.Object.__init__(self, service, object_path)

        # TODO: listen for changes and reconfigure the scrobbler
        client = gconf.client_get_default ()
        username = client.get_string("/apps/dscrobbler/username")
        password = client.get_string("/apps/dscrobbler/password")

        self.scrobbler = Scrobbler(username, password)

        self.timeout_id = 0

    # TODO: def NowPlaying()

    @dbus.service.method(
        dbus_interface='com.burtonini.Scrobbler',
        in_signature='usssuuss',
        out_signature='')
    def Submit(self, timestamp, artist, title, track, length=0, album="", musicbrainz="", source="P"):
        # This will throw an exception if the submitted data is invalid
        self.scrobbler.submit(timestamp=int(timestamp),
                              artist=unicode(artist),
                              title=unicode(title),
                              track=int(track),
                              length=int(length),
                              album=unicode(album),
                              musicbrainz=unicode(musicbrainz),
                              source=unicode(source))
        self.maybe_flush()

    @dbus.service.method(dbus_interface='com.burtonini.Scrobbler')
    def Flush(self):
        if self.timeout_id:
            gobject.source_remove(self.timeout_id)
            self.timeout_id = 0
        self.scrobbler.flush()
        self.maybe_flush()

    def maybe_flush(self):
        def idle():
            self.Flush()
            return False
        # TODO: if we make the scrobbler threadsafe, this could run in a thread
        # to avoid blocking the service.
        if len(self.scrobbler) > 10:
            gobject.idle_add (idle)
        elif not self.timeout_id:
            self.timeout_id = gobject.timeout_add(60*1000, idle)


if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("com.burtonini.Scrobbler", session_bus)
    s = ScrobblerService(name)

    gobject.MainLoop().run()
