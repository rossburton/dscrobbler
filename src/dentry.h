#include <time.h>

#define EXTRA_URI_ENCODE_CHARS "&+"

typedef enum {
  SOURCE_UNKNOWN,
  SOURCE_USER,
  SOURCE_BROADCAST,
  SOURCE_PERSONAL,
  SOURCE_LASTFM,
} DEntrySource;

typedef struct {
  char *artist;
  char *album;
  char *title;
  unsigned int track;
  unsigned int length;
  char *mbid;
  DEntrySource source;
  time_t play_time;
} DEntry;

DEntry * d_entry_new (const char *artist,
                      const char *album,
                      const char *title,
                      unsigned int track,
                      unsigned int length,
                      const char *musicbrainz,
                      DEntrySource source,
                      time_t play_time);

char * d_entry_encode (DEntry *entry, const int i);

void d_entry_free (DEntry *entry);
