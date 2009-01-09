#include <glib.h>
#include <libsoup/soup.h>
#include "dentry.h"

void
d_entry_free (DEntry *entry)
{
  g_free (entry->artist);
  g_free (entry->album);
  g_free (entry->title);
  g_free (entry->mbid);
  g_free (entry->source);

  g_slice_free (DEntry, entry);
}

#define SCROBBLER_DATE_FORMAT "%Y%%2D%m%%2D%d%%20%H%%3A%M%%3A%S"
#define SCROBBLER_DATE_FORMAT_LEN 30

static char *
build_audioscrobbler_date (time_t t)
{
  char *s;
  s = g_new0 (gchar, SCROBBLER_DATE_FORMAT_LEN);
  strftime (s, SCROBBLER_DATE_FORMAT_LEN, SCROBBLER_DATE_FORMAT, gmtime (&t));
  return s;
}

char *
d_entry_encode (DEntry *entry, const int i)
{
  GString *str;
  char *s;

  g_return_val_if_fail (entry, NULL);

  str = g_string_new (NULL);

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

  s = build_audioscrobbler_date (entry->play_time);
  g_string_append_printf (str, "i[%d]=%s&", i, s);
  g_free (s);

  g_string_append_printf (str, "n[%d]=%d&", i, entry->track);

  g_string_append_printf (str, "l[%d]=%d&", i, entry->length);

  return g_string_free (str, FALSE);
}
