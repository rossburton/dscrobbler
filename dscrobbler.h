#ifndef _D_SCROBBLER
#define _D_SCROBBLER

#include <glib-object.h>

G_BEGIN_DECLS

#define D_TYPE_SCROBBLER d_scrobbler_get_type()

#define D_SCROBBLER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), D_TYPE_SCROBBLER, DScrobbler))

#define D_SCROBBLER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), D_TYPE_SCROBBLER, DScrobblerClass))

#define D_IS_SCROBBLER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), D_TYPE_SCROBBLER))

#define D_IS_SCROBBLER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), D_TYPE_SCROBBLER))

#define D_SCROBBLER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), D_TYPE_SCROBBLER, DScrobblerClass))

typedef struct _DScrobblerPrivate DScrobblerPrivate;

typedef struct {
  GObject parent;
  DScrobblerPrivate *priv;
} DScrobbler;

typedef struct {
  GObjectClass parent_class;
} DScrobblerClass;

GType d_scrobbler_get_type (void);

DScrobbler* d_scrobbler_new (void);

G_END_DECLS

#endif /* _D_SCROBBLER */
