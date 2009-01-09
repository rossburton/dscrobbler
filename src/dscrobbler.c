#include <string.h>
#include <time.h>
#include <libsoup/soup.h>

#include "dscrobbler.h"
#include "dscrobbler-dbus.h"
#include "dentry.h"

static void dbus_iface_init (DScrobblerIfaceClass *iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (DScrobbler, d_scrobbler, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (D_TYPE_SCROBBLER_IFACE, dbus_iface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), D_TYPE_SCROBBLER, DScrobblerPrivate))

struct _DScrobblerPrivate {
  GQueue *queue;
  char *username;
  char *password;
  guint failures;
  gboolean handshake;
  time_t handshake_next;
  time_t submit_next;
  time_t submit_interval;
  SoupSession *soup_session;
  char *md5_challenge;
  char *submit_url;
  GQueue *submission;
  enum {
    STATUS_OK = 0,
    HANDSHAKING,
    REQUEST_FAILED,
    BAD_USERNAME,
    BAD_PASSWORD,
    HANDSHAKE_FAILED,
    CLIENT_UPDATE_REQUIRED,
    SUBMIT_FAILED,
    QUEUE_TOO_LONG,
    GIVEN_UP,
  } status;
  /* remove these or add GetStats method? */
  guint submit_count;
  guint queue_count;
  gboolean queue_changed;
};

#define CLIENT_ID "tst" /* TODO: get real id */
#define CLIENT_VERSION VERSION
#define MAX_QUEUE_SIZE 1000
#define MAX_SUBMIT_SIZE	10
#define SCROBBLER_URL "http://post.audioscrobbler.com/"
#define SCROBBLER_VERSION "1.1" /* TODO: upgrade */

/*
 * Private methods
 */

static void
parse_response (DScrobbler *scrobbler, SoupMessage *msg)
{
	gboolean successful;
	g_debug ("Parsing response, status=%d", msg->status_code);

	successful = FALSE;
	if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code) && msg->response_body->length != 0)
		successful = TRUE;
	if (successful) {
		gchar **breaks;
		int i;
		breaks = g_strsplit (msg->response_body->data, "\n", 4);

		scrobbler->priv->status = STATUS_OK;
		for (i = 0; breaks[i] != NULL; i++) {
			g_debug ("RESPONSE: %s", breaks[i]);
			if (g_str_has_prefix (breaks[i], "UPTODATE")) {
				g_debug ("UPTODATE");

				if (breaks[i+1] != NULL) {
					g_free (scrobbler->priv->md5_challenge);
					scrobbler->priv->md5_challenge = g_strdup (breaks[i+1]);
					g_debug ("MD5 challenge: \"%s\"", scrobbler->priv->md5_challenge);

					if (breaks[i+2] != NULL) {
						g_free (scrobbler->priv->submit_url);
						scrobbler->priv->submit_url = g_strdup (breaks[i+2]);
						g_debug ("Submit URL: \"%s\"", scrobbler->priv->submit_url);
						i++;
					}
					i++;
				}

			} else if (g_str_has_prefix (breaks[i], "UPDATE")) {
				g_debug ("UPDATE");
				scrobbler->priv->status = CLIENT_UPDATE_REQUIRED;

				if (breaks[i+1] != NULL) {
					g_free (scrobbler->priv->md5_challenge);
					scrobbler->priv->md5_challenge = g_strdup (breaks[i+1]);
					g_debug ("MD5 challenge: \"%s\"", scrobbler->priv->md5_challenge);

					if (breaks[i+2] != NULL) {
						g_free (scrobbler->priv->submit_url);
						scrobbler->priv->submit_url = g_strdup (breaks[i+2]);
						g_debug ("Submit URL: \"%s\"", scrobbler->priv->submit_url);
						i++;
					}
					i++;
				}

			} else if (g_str_has_prefix (breaks[i], "FAILED")) {
				scrobbler->priv->status = HANDSHAKE_FAILED;

				if (strlen (breaks[i]) > 7) {
					g_debug ("FAILED: \"%s\"", breaks[i] + 7);
				} else {
					g_debug ("FAILED");
				}


			} else if (g_str_has_prefix (breaks[i], "BADUSER")) {
				g_debug ("BADUSER");
				scrobbler->priv->status = BAD_USERNAME;
			} else if (g_str_has_prefix (breaks[i], "BADAUTH")) {
				g_debug ("BADAUTH");
				scrobbler->priv->status = BAD_PASSWORD;
			} else if (g_str_has_prefix (breaks[i], "OK")) {
				g_debug ("OK");
			} else if (g_str_has_prefix (breaks[i], "INTERVAL ")) {
				scrobbler->priv->submit_interval = g_ascii_strtod(breaks[i] + 9, NULL);
				g_debug ("INTERVAL: %s", breaks[i] + 9);
			}
		}

		/* respect the last submit interval we were given */
		if (scrobbler->priv->submit_interval > 0)
			scrobbler->priv->submit_next = time(NULL) + scrobbler->priv->submit_interval;

		g_strfreev (breaks);
	} else {
		scrobbler->priv->status = REQUEST_FAILED;
	}
}

/* TODO: understand and hopefully remove */
static gboolean
idle_unref_cb (GObject *object)
{
  g_object_unref (object);
  return FALSE;
}

/*
 * NOTE: the caller *must* unref the audioscrobbler object in an idle
 * handler created in the callback.
 */
static void
perform (DScrobbler *scrobbler,
			   char *url,
			   char *post_data,
			   SoupSessionCallback response_handler)
{
  SoupMessage *msg;

  msg = soup_message_new (post_data == NULL ? "GET" : "POST", url);
  soup_message_headers_append (msg->request_headers, "User-Agent", "DScrobbler/" VERSION);

  if (post_data != NULL) {
    g_debug ("Submitting to Audioscrobbler: %s", post_data);
    soup_message_set_request (msg,
                              "application/x-www-form-urlencoded",
                              SOUP_MEMORY_TAKE,
                              post_data,
                              strlen (post_data));
  }

  if (!scrobbler->priv->soup_session) {
    /* TODO: proxy */
    scrobbler->priv->soup_session = soup_session_async_new ();
  }

  soup_session_queue_message (scrobbler->priv->soup_session,
                              msg,
                              response_handler,
                              g_object_ref (scrobbler));
}

static void
do_handshake_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
  DScrobbler *scrobbler = D_SCROBBLER(user_data);

  g_debug ("Handshake response");
  parse_response (scrobbler, msg);

  switch (scrobbler->priv->status) {
  case STATUS_OK:
  case CLIENT_UPDATE_REQUIRED:
    scrobbler->priv->handshake = TRUE;
    scrobbler->priv->failures = 0;
    break;
  default:
    g_debug ("Handshake failed");
    ++scrobbler->priv->failures;
    break;
  }

  g_idle_add ((GSourceFunc) idle_unref_cb, scrobbler);
}

static gboolean
should_handshake (DScrobbler *scrobbler)
{
  /* Perform handshake if necessary. Only perform handshake if
   *   - we have no current handshake; AND
   *   - we have waited the appropriate amount of time between
   *     handshakes; AND
   *   - we have a non-empty username
   */
  if (scrobbler->priv->handshake) {
    return FALSE;
  }

  if (time (NULL) < scrobbler->priv->handshake_next) {
    g_debug ("Too soon; time=%lu, handshake_next=%lu",
             time (NULL),
             scrobbler->priv->handshake_next);
    return FALSE;
  }

  if ((scrobbler->priv->username == NULL) ||
      (strcmp (scrobbler->priv->username, "") == 0)) {
    g_debug ("No username set");
    return FALSE;
  }

  return TRUE;
}

static void
do_handshake (DScrobbler *scrobbler)
{
  char *username;
  char *url;

  if (!should_handshake (scrobbler)) {
    return;
  }

  username = soup_uri_encode (scrobbler->priv->username, EXTRA_URI_ENCODE_CHARS);
  url = g_strdup_printf ("%s?hs=true&p=%s&c=%s&v=%s&u=%s",
                         SCROBBLER_URL,
                         SCROBBLER_VERSION,
                         CLIENT_ID,
                         CLIENT_VERSION,
                         username);
  g_free (username);

  /* Make sure we wait at least 30 minutes between handshakes. */
  scrobbler->priv->handshake_next = time (NULL) + 1800;

  g_debug ("Performing handshake with Audioscrobbler server: %s", url);

  scrobbler->priv->status = HANDSHAKING;

  perform (scrobbler, url, NULL, do_handshake_cb);

  g_free (url);
}

static gchar *
mkmd5 (char *string)
{
	GChecksum *checksum;
	gchar *md5_result;

	checksum = g_checksum_new(G_CHECKSUM_MD5);
	g_checksum_update(checksum, (guchar *)string, -1);

	md5_result = g_strdup(g_checksum_get_string(checksum));
	g_checksum_free(checksum);

	return (md5_result);
}

static GString *
build_authentication_data (DScrobbler *scrobbler)
{
  GString *str;
  gchar *md5_password;
  gchar *md5_temp;
  gchar *md5_response;
  gchar *username;
  gchar *post_data;
  time_t now;

  /* Conditions:
   *   - Must have username and password
   *   - Must have md5_challenge
   *   - Queue must not be empty
   */
  if ((scrobbler->priv->username == NULL) || (*scrobbler->priv->username == '\0')) {
    g_debug ("No username set");
    return NULL;
  }

  if ((scrobbler->priv->password == NULL) || (*scrobbler->priv->password == '\0')) {
    g_debug ("No password set");
    return NULL;
  }

  if (*scrobbler->priv->md5_challenge == '\0') {
    g_debug ("No md5 challenge");
    return NULL;
  }

  time(&now);
  if (now < scrobbler->priv->submit_next) {
    g_debug ("Too soon (next submission in %ld seconds)",
             scrobbler->priv->submit_next - now);
    return NULL;
  }

  if (g_queue_is_empty (scrobbler->priv->queue)) {
    g_debug ("No queued songs to submit");
    return NULL;
  }

  str = g_string_new (NULL);

  md5_password = mkmd5 (scrobbler->priv->password);
  md5_temp = g_strconcat (md5_password,
                          scrobbler->priv->md5_challenge,
                          NULL);
  md5_response = mkmd5 (md5_temp);

  username = soup_uri_encode (scrobbler->priv->username, EXTRA_URI_ENCODE_CHARS);

  g_string_append_printf (str, "u=%s&s=%s&", username, md5_response);

  g_free (md5_password);
  g_free (md5_temp);
  g_free (md5_response);
  g_free (username);

  return str;
}

static void
d_g_queue_concat (GQueue *q1, GQueue *q2)
{
  GList *elem;

  while (!g_queue_is_empty (q2)) {
    elem = g_queue_pop_head_link (q2);
    g_queue_push_tail_link (q1, elem);
  }
}

static void
free_queue_entries (DScrobbler *scrobbler, GQueue **queue)
{
	g_queue_foreach (*queue, (GFunc)d_entry_free, NULL);
	g_queue_free (*queue);
	*queue = NULL;

	scrobbler->priv->queue_changed = TRUE;
}

static void
save_queue (DScrobbler *scrobbler)
{
  g_debug ("TODO: save queue");
}

static void
submit_queue_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	DScrobbler *scrobbler = D_SCROBBLER (user_data);

	g_debug ("Submission response");
	parse_response (scrobbler, msg);

	if (scrobbler->priv->status == STATUS_OK) {
		g_debug ("Queue submitted successfully");
		free_queue_entries (scrobbler, &scrobbler->priv->submission);
		scrobbler->priv->submission = g_queue_new ();
		save_queue (scrobbler);
		scrobbler->priv->submit_count += scrobbler->priv->queue_count;
		scrobbler->priv->queue_count = 0;
	} else {
		++scrobbler->priv->failures;

		/* add failed submission entries back to queue */
		d_g_queue_concat (scrobbler->priv->submission,
				   scrobbler->priv->queue);
		g_assert (g_queue_is_empty (scrobbler->priv->queue));
		g_queue_free (scrobbler->priv->queue);
		scrobbler->priv->queue = scrobbler->priv->submission;
		scrobbler->priv->submission = g_queue_new ();;
		save_queue (scrobbler);
		if (scrobbler->priv->failures >= 3) {
			g_debug ("Queue submission has failed %d times; caching tracks locally",
				  scrobbler->priv->failures);

			scrobbler->priv->handshake = FALSE;
			scrobbler->priv->status = GIVEN_UP;
		} else {
			g_debug ("Queue submission failed %d times", scrobbler->priv->failures);
		}
	}

	g_idle_add ((GSourceFunc) idle_unref_cb, scrobbler);
}

#define SCROBBLER_DATE_FORMAT "%Y%%2D%m%%2D%d%%20%H%%3A%M%%3A%S"
#define SCROBBLER_DATE_FORMAT_LEN 30

static char *
build_scrobbler_date (time_t t)
{
  char *s;
  s = g_new0 (gchar, SCROBBLER_DATE_FORMAT_LEN);
  strftime (s, SCROBBLER_DATE_FORMAT_LEN, SCROBBLER_DATE_FORMAT, gmtime (&t));
  return s;
}

static GString *
build_post_data (DScrobbler *scrobbler, GString *str)
{
  g_return_val_if_fail (!g_queue_is_empty (scrobbler->priv->queue), NULL);

  int i = 0;
  do {
    DEntry *entry;
    char *s;

    entry = g_queue_pop_head (scrobbler->priv->queue);

    s = soup_uri_encode (entry->artist, EXTRA_URI_ENCODE_CHARS);
    g_string_append_printf (str, "a[%d]=%s&", i, s);
    g_free (s);

    s = soup_uri_encode (entry->title, EXTRA_URI_ENCODE_CHARS);
    g_string_append_printf (str, "t[%d]=%s&", i, s);
    g_free (s);

    s = soup_uri_encode (entry->album, EXTRA_URI_ENCODE_CHARS);
    g_string_append_printf (str, "b[%d]=%s&", i, s);
    g_free (s);

    s = soup_uri_encode (entry->mbid, EXTRA_URI_ENCODE_CHARS);
    g_string_append_printf (str, "m[%d]=%s&", i, s);
    g_free (s);

    s = build_scrobbler_date (entry->play_time);
    g_string_append_printf (str, "i[%d]=%s&", i, s);
    g_free (s);

    g_string_append_printf (str, "n[%d]=%d&", i, entry->track);

    g_string_append_printf (str, "l[%d]=%d&", i, entry->length);

    g_queue_push_tail (scrobbler->priv->submission, entry);
    i++;
  } while ((!g_queue_is_empty(scrobbler->priv->queue)) && (i < MAX_SUBMIT_SIZE));

  return str;
}

static void
submit_queue (DScrobbler *scrobbler)
{
  GString *str;

  str = build_authentication_data (scrobbler);
  if (str) {
    build_post_data (scrobbler, str);
    g_debug ("Submitting queue to Audioscrobbler");

    perform (scrobbler,
             scrobbler->priv->submit_url,
             g_string_free (str, FALSE),
             submit_queue_cb);
    /* libsoup will free str when the request is finished */
  }
}

/*
 * DBus implementation
 */
static void
dbus_submit (DScrobblerIface *self,
             guint time,
             const gchar *artist,
             const gchar *title,
             guint track,
             guint length,
             const gchar *album,
             const gchar *musicbrainz,
             const gchar *source,
             DBusGMethodInvocation *context)
{
  DScrobbler *scrobbler = D_SCROBBLER (self);
  DEntry *entry;

  entry = g_slice_new0 (DEntry);

  entry->artist = g_strdup (artist);
  entry->album = g_strdup (album);
  entry->title = g_strdup (title);
  entry->length = length;
  entry->mbid = g_strdup (musicbrainz);
  entry->play_time = time;
  entry->track = track;
  entry->source = g_strdup (source);

  g_queue_push_tail (scrobbler->priv->queue, entry);

  submit_queue (scrobbler);

  d_scrobbler_iface_return_from_submit (context);
}


/*
 * Object implementation
 */
static void
d_scrobbler_dispose (GObject *object)
{
  G_OBJECT_CLASS (d_scrobbler_parent_class)->dispose (object);
}

static void
d_scrobbler_finalize (GObject *object)
{
  DScrobblerPrivate *priv = D_SCROBBLER (object)->priv;

  /* TODO: free the elements */
  g_queue_free (priv->queue);

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
  DScrobbler *scrobbler = (DScrobbler*)object;
  DBusGConnection *connection;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (error) {
    g_warning ("Failed to open connection to DBus: %s\n", error->message);
    g_error_free (error);
  }

  dbus_g_connection_register_g_object (connection, "/com/burtonini/Scrobbler", object);

  /* TODO: do this on demand */
  do_handshake (scrobbler);
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

  self->priv->queue = g_queue_new ();
  self->priv->submission = g_queue_new ();

  /* TODO: read from gconf */
  self->priv->username = g_strdup ("username");
  self->priv->password = g_strdup ("password");
}


/*
 * Public methods
 */

DScrobbler*
d_scrobbler_new (void)
{
  return g_object_new (D_TYPE_SCROBBLER, NULL);
}
