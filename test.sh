dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Submit \
    uint32:`date +%s` string:"José González" string:"How Low" uint32:1 uint32:159

dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Flush
