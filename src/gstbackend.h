#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstinferencebackends.h"

G_BEGIN_DECLS
#define GST_TYPE_BACKEND gst_backend_get_type()
G_DECLARE_DERIVABLE_TYPE(GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass {
  GObjectClass parent_class;
};

gboolean gst_backend_start(GstBackend*, GError**);
gboolean gst_backend_stop(GstBackend*, GError**);
gboolean gst_backend_process_frame(GstBackend*, GstVideoFrame*, gpointer*, gsize*, GError**);

gboolean gst_backend_set_code(GstBackend* backend, GstMLBackends code);  // protected
GstMLBackends gst_backend_get_code(GstBackend* backend);

GstBackend* gst_new_backend(GstMLBackends type);
void gst_free_backend(GstBackend* backend);

G_END_DECLS
