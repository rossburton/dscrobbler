dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Submit \
    uint32:`date +%s` string:"José González" string:"Killing For Love" uint32:3 uint32:182 string:"" string:"" string:"P"

#dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Flush
