import md5, urllib, urllib2, time

debug = True

def log(msg):
    if debug: print "[scrobbler]", msg

class Submission:
    # TODO: set some defaults
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
        # TODO restore queue
        
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
        log(lines)
        
        status = lines.pop(0)
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
            # TODO: throw exception?
            return

    def submit(self, submission):
        # Silently ignore submissions less than 30 seconds
        if submission.length and submission.length < 30:
            return

        self.queue.append(submission)
        # TODO: persist queue
        # TODO: start timer for idle submission
        # TODO: if queue is long, submit in an idle
    
    def flush(self):
        if len(self.queue) == 0:
            return
        
        if self.sessionid is None:
            self.handshake()
        
        data = { 's': self.sessionid }

        for s in self.queue[:10]:
            i = self.queue.index(s)
            log("Sending song: %s - %s" % (s.artist, s.title))            
            data["a[%d]" % i] = s.artist.encode('utf-8')
            data["t[%d]" % i] = s.title.encode('utf-8')
            data["i[%d]" % i] = s.timestamp
            data["o[%d]" % i] = s.source
            data["r[%d]" % i] = ''
            data["l[%d]" % i] = s.length
            data["b[%d]" % i] = s.album.encode('utf-8')
            data["n[%d]" % i] = s.track
            data["m[%d]" % i] = s.musicbrainz
        
        try:
            resp = urllib2.urlopen(self.submit_url, urllib.urlencode(data))
        except:
            log("Audioscrobbler server not responding, will try later.")
            # throw exception
            return
        
        lines = [l.rstrip() for l in resp.readlines()]
        log (lines)

        status = lines.pop(0)
        if status == "OK":
            # Remove the submitted tracks
            self.queue = self.queue[10:]
        elif status == "BADSESSION":
            self.handshake()
            self.flush()
        elif status.startswith ("FAILED "):
            raise Exception(" ".join(status.split()[1:]))
        else:
            raise Exception("Unexpected response to submission: %s", status)
        
        # TODO if queue still items in, queue another submission in 10 minutes or so
        # TODO persist or wipe on disk queue
