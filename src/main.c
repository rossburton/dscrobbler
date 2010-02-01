#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "dscrobbler.h"

static gboolean
request_name (void)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy, "com.burtonini.Scrobbler",
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE, &request_status,
                                          &error)) {
    g_printerr ("Failed to request name: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

int
main (int argc, char **argv)
{
  DScrobbler *scrobbler;
  GMainLoop *loop;

  g_thread_init (NULL);
  g_type_init ();

  loop = g_main_loop_new (NULL, TRUE);

  scrobbler = d_scrobbler_new ();

  if (!request_name ())
    return 0;

  g_main_loop_run (loop);

  g_object_unref (scrobbler);
  g_object_unref (loop);

  return 0;
}
