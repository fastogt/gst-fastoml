#pragma once

#include <fastoml/gst/gstbackend.h>

gboolean gst_backend_set_code(GstBackend* backend, fastoml::SupportedBackends code);

void gst_backend_set_property(GstBackend* backend, const gchar* property, const GValue* value);
void gst_backend_get_property(GstBackend* backend, const gchar* property, GValue* value);
