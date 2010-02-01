#include "dscrobbler.h"
#include "dscrobbler-dbus.h"
#include "mafw-lastfm-scrobbler.h"

static void dbus_iface_init (DScrobblerIfaceClass *iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (DScrobbler, d_scrobbler, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (D_TYPE_SCROBBLER_IFACE, dbus_iface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), D_TYPE_SCROBBLER, DScrobblerPrivate))

struct _DScrobblerPrivate {
  MafwLastfmScrobbler *scrobbler;
};

/*
 * DBus implementation
 */
static void
dbus_submit (DScrobblerIface *self,
             guint play_time,
             const char *artist,
             const char *title,
             guint track_no,
             guint length,
             const gchar *album,
             const char *musicbrainz,
             const char *source,
             DBusGMethodInvocation *context)
{
  DScrobbler *scrobbler = D_SCROBBLER (self);
  MafwLastfmTrack *track;

  track = mafw_lastfm_track_new ();

  track->artist = (char*)artist;
  track->title = (char*)title;
  track->timestamp = play_time;
  track->source = source[0];
  track->album = (char*)album;
  track->number = track_no;
  track->length = length;

  mafw_lastfm_scrobbler_enqueue_scrobble (scrobbler->priv->scrobbler, track);
  mafw_lastfm_track_free (track);

  d_scrobbler_iface_return_from_submit (context);
}


/*
 * Object implementation
 */

static void
d_scrobbler_dispose (GObject *object)
{
  DScrobblerPrivate *priv = D_SCROBBLER (object)->priv;

  if (priv->scrobbler) {
    mafw_lastfm_scrobbler_flush_queue (priv->scrobbler);
    g_object_unref (priv->scrobbler);
    priv->scrobbler = NULL;
  }

  G_OBJECT_CLASS (d_scrobbler_parent_class)->dispose (object);
}

static void
dbus_iface_init (DScrobblerIfaceClass *iface, gpointer iface_data)
{
  d_scrobbler_iface_implement_submit (iface, dbus_submit);
}

static void
d_scrobbler_constructed (GObject *object)
{
  DScrobbler *scrobbler = (DScrobbler*)object;
  DBusGConnection *connection;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (error) {
    g_warning ("Failed to open connection to DBus: %s\n", error->message);
    g_error_free (error);
  }

  dbus_g_connection_register_g_object (connection, "/com/burtonini/Scrobbler", object);
}

static void
d_scrobbler_class_init (DScrobblerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DScrobblerPrivate));

  object_class->constructed = d_scrobbler_constructed;
  object_class->dispose = d_scrobbler_dispose;
}

static void
d_scrobbler_init (DScrobbler *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->scrobbler = mafw_lastfm_scrobbler_new ();

  mafw_lastfm_scrobbler_set_credentials (self->priv->scrobbler,
                                         "username", "password");
  mafw_lastfm_scrobbler_handshake (self->priv->scrobbler);
}


/*
 * Public methods
 */

DScrobbler*
d_scrobbler_new (void)
{
  return g_object_new (D_TYPE_SCROBBLER, NULL);
}
