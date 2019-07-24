#pragma once

#include <gst/video/video.h>

#include <fastoml/types.h>

G_BEGIN_DECLS
#define GST_TYPE_BACKEND gst_backend_get_type()
G_DECLARE_DERIVABLE_TYPE(GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass {
  GObjectClass parent_class;
};

GstBackend* gst_backend_new(fastoml::SupportedBackends code);
void gst_backend_free(GstBackend*);

gboolean gst_backend_start(GstBackend*, GError**);
gboolean gst_backend_stop(GstBackend*, GError**);
gboolean gst_backend_process_frame(GstBackend*, GstVideoFrame*, gpointer, gpointer*, gsize*, GError**);

G_END_DECLS
