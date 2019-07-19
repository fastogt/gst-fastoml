#ifndef __GST_BACKEND_H__
#define __GST_BACKEND_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS
#define GST_TYPE_BACKEND gst_backend_get_type()
G_DECLARE_DERIVABLE_TYPE(GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass {
  GObjectClass parent_class;
};

gboolean gst_backend_start(GstBackend*, const gchar*, GError**);
gboolean gst_backend_stop(GstBackend*, GError**);
guint gst_backend_get_framework_code(GstBackend*);
gboolean gst_backend_process_frame(GstBackend*, GstVideoFrame*, gpointer*, gsize*, GError**);

G_END_DECLS
#endif  //__GST_BACKEND_H__
