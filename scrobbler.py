import md5, urllib, urllib2, time

debug = True

def log(msg):
    if debug: print "[scrobbler]", msg

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
            log("Server not responding, handshake failed: %s", e)
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
        
    def submit(self, artist, title, album=u'', musicbrainzid=u'', length=0, track='', timestamp=0):
        if self.sessionid is None:
            # TODO: handle errors
            self.handshake()
        
        data = { 's': self.sessionid }
        
        log("Sending song: %s - %s" % (artist, title))
        
        data["a[0]"] = artist.encode('utf-8')
        data["t[0]"] = title.encode('utf-8')
        data["i[0]"] = timestamp
        data["o[0]"] = "P" # TODO
        data["r[0]"] = ''
        data["l[0]"] = str(length)
        data["b[0]"] = album.encode('utf-8')
        data["n[0]"] = track
        data["m[0]"] = musicbrainzid
        
        resp = None
        try:
            data_str = urllib.urlencode(data)
            print self.submit_url
            print data_str
            resp = urllib2.urlopen(self.submit_url, data_str)
        except:
            log("Audioscrobbler server not responding, will try later.")
            return
        
        lines = [l.rstrip() for l in resp.readlines()]
        print lines
