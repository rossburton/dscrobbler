#include <glib.h>
#include "dentry.h"

void
d_entry_init (DEntry *entry)
{
  entry->artist = g_strdup ("");
  entry->album = g_strdup ("");
  entry->title = g_strdup ("");
  entry->length = 0;
  entry->play_time = 0;
  entry->mbid = g_strdup ("");
}

void
d_entry_free (DEntry *entry)
{
  g_free (entry->artist);
  g_free (entry->album);
  g_free (entry->title);
  g_free (entry->mbid);

  g_slice_free (DEntry, entry);
}
