#include <time.h>

#define EXTRA_URI_ENCODE_CHARS "&+"

typedef struct {
  char *artist;
  char *album;
  char *title;
  unsigned int track;
  unsigned int length;
  char *mbid;
  char *source;
  time_t play_time;
} DEntry;

void d_entry_free (DEntry *entry);
