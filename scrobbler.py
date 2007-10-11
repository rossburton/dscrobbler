import md5, urllib, urllib2, time

debug = True

def log(msg):
    if debug: print "[scrobbler]", msg

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
        return "%s by %s\nTimestamp: %d" % (self.title, self.artist, self.timestamp)
        
class Scrobbler:
    PROTOCOL_VERSION = "1.2"

    sessionid = None
    nowplaying_url = None
    submit_url = None
    
    queue = []

    def __init__(self, username, password, client_name="tst", client_version="1.0"):
        self.username = username
        self.password = password
        self.client_name = client_name
        self.client_version = client_version
    
    def handshake(self):
        timestamp = int(time.time())
        
        token = md5.new(md5.new(self.password).hexdigest() + str(timestamp)).hexdigest()
        
        data = {
            "hs": "true",
            "p": self.PROTOCOL_VERSION,
            "c": self.client_name,
            "v": self.client_version,
            "u": self.username,
            "t": timestamp,
            "a": token
            }
        
        url = "http://post.audioscrobbler.com/?" + urllib.urlencode(data)

        try:
            resp = urllib2.urlopen(url);
        except Exception, e:
            log("Server not responding, handshake failed: %s" % e)
            # TODO throw exception
            return
        
        lines = [l.rstrip() for l in resp.readlines()]

        status = lines.pop(0)
        log("Handshake status: %s" % status)
        if status == "OK":
            self.sessionid = lines.pop(0)
            log("Session: %s" % self.sessionid)

            self.nowplaying_url = lines.pop(0)
            log("Now Playing URL: %s" % self.nowplaying_url)

            self.submit_url = lines.pop(0)
            log("Submit URL: %s" % self.submit_url)

            self.challenge_sent = True
        else:
            # TODO BADAUTH, BANNED, BADTIME
            # TODO: throw exception
            return
        
    def submit(self, s):
        if self.sessionid is None:
            # TODO: handle errors
            self.handshake()
        
        data = { 's': self.sessionid }
        
        log("Sending song: %s - %s" % (s.artist, s.title))
        
        data["a[0]"] = s.artist.encode('utf-8')
        data["t[0]"] = s.title.encode('utf-8')
        data["i[0]"] = s.timestamp
        data["o[0]"] = s.source
        data["r[0]"] = ''
        data["l[0]"] = str(s.length)
        data["b[0]"] = s.album.encode('utf-8')
        data["n[0]"] = s.track
        data["m[0]"] = s.musicbrainz
        
        resp = None
        try:
            data_str = urllib.urlencode(data)
            resp = urllib2.urlopen(self.submit_url, data_str)
        except:
            log("Audioscrobbler server not responding, will try later.")
            return
        
        lines = [l.rstrip() for l in resp.readlines()]
        print lines
