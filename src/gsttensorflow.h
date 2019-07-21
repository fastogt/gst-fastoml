#pragma once

#include <fastoml/gst/gstbackend.h>

G_BEGIN_DECLS

#define GST_TYPE_TENSORFLOW gst_tensorflow_get_type()
G_DECLARE_FINAL_TYPE(GstTensorflow, gst_tensorflow, GST, TENSORFLOW, GstBackend);

G_END_DECLS
