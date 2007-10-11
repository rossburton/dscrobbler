dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Submit \
    uint32:1192094708 string:"Boards of Canada" string:"Dandelion" uint32:5 uint32:75

dbus-send  --print-reply --dest=com.burtonini.Scrobbler /com/burtonini/Scrobbler com.burtonini.Scrobbler.Flush
