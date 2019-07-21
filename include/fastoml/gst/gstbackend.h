#pragma once

#include <gst/video/video.h>

#include <fastoml/types.h>

G_BEGIN_DECLS
#define GST_TYPE_BACKEND gst_backend_get_type()
G_DECLARE_DERIVABLE_TYPE(GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass {
  GObjectClass parent_class;
};

gboolean gst_backend_start(GstBackend*, GError**);
gboolean gst_backend_stop(GstBackend*, GError**);
gboolean gst_backend_process_frame(GstBackend*, GstVideoFrame*, gpointer*, gsize*, GError**);

GstBackend* gst_new_backend(fastoml::SupportedBackends code);
void gst_free_backend(GstBackend*);

G_END_DECLS
