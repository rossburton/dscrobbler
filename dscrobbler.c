#include "dscrobbler.h"
#include "dscrobbler-dbus.h"

static void dbus_iface_init (DScrobblerIfaceClass *iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (DScrobbler, d_scrobbler, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (D_TYPE_SCROBBLER_IFACE, dbus_iface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), D_TYPE_SCROBBLER, DScrobblerPrivate))

struct _DScrobblerPrivate {
  int dummy;
};

static void
dbus_submit (DScrobblerIface *self,
             guint in_time,
             const gchar *in_artist,
             const gchar *in_title,
             guint in_track,
             guint in_length,
             const gchar *in_album,
             const gchar *in_musicbrainz,
             const gchar *in_source,
             DBusGMethodInvocation *context)
{
}

static void
d_scrobbler_dispose (GObject *object)
{
  G_OBJECT_CLASS (d_scrobbler_parent_class)->dispose (object);
}

static void
d_scrobbler_finalize (GObject *object)
{
  G_OBJECT_CLASS (d_scrobbler_parent_class)->finalize (object);
}

static void
dbus_iface_init (DScrobblerIfaceClass *iface, gpointer iface_data)
{
  d_scrobbler_iface_implement_submit (iface, dbus_submit);
}

static void
d_scrobbler_constructed (GObject *object)
{
  DScrobblerPrivate *priv = D_SCROBBLER (object)->priv;
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
  object_class->finalize = d_scrobbler_finalize;
}

static void
d_scrobbler_init (DScrobbler *self)
{
  self->priv = GET_PRIVATE (self);
}

DScrobbler*
d_scrobbler_new (void)
{
  return g_object_new (D_TYPE_SCROBBLER, NULL);
}
