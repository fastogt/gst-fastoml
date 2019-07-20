#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum { GST_ML_BACKEND_TENSORFLOW = 0 } GstMLBackends;
#define GST_TYPE_ML_BACKEND (gst_ml_backend_get_type())

GType gst_ml_backend_get_type(void);

G_END_DECLS
