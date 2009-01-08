#include <time.h>

#define EXTRA_URI_ENCODE_CHARS "&+"

typedef struct {
  char *artist;
  char *album;
  char *title;
  unsigned int length;
  char *mbid;
  time_t play_time;
} DEntry;

G_GNUC_DEPRECATED void d_entry_init (DEntry *entry);
void d_entry_free (DEntry *entry);
