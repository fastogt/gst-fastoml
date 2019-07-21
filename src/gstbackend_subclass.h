#pragma once

#include "gstbackend.h"

gboolean gst_backend_set_code(GstBackend* backend, GstMLBackends code);

void gst_backend_set_property(GstBackend* backend, const gchar* property, const GValue* value);
void gst_backend_get_property(GstBackend* backend, const gchar* property, GValue* value);
