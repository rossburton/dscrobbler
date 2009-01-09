#include <glib.h>
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
