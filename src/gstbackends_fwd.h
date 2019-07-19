#ifndef __GST_BACKENDSUBCLASS_H__
#define __GST_BACKENDSUBCLASS_H__

#include <fastoml/types.h>

#include "gstbackend.h"

G_BEGIN_DECLS

void gst_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);
void gst_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);

void gst_backend_install_properties(GstBackendClass* klass, fastoml::SupportedBackends code);
gboolean gst_backend_set_framework_code(GstBackend* backend, fastoml::SupportedBackends code);

G_END_DECLS
#endif  //__GST_BACKENDSUBCLASS_H__
