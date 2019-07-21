#pragma once

#include <fastoml/gst/gstbackend.h>

void gst_backend_set_property(GstBackend* backend, const gchar* property, const GValue* value);
void gst_backend_get_property(GstBackend* backend, const gchar* property, GValue* value);
