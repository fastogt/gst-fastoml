#include "gstinferencebackends.h"

#include <fastoml/backend.h>

#include "gstbackend.h"
#include "gstchildinspector.h"
#include "gsttensorflow.h"

#define DEFAULT_ALIGNMENT 32

static std::vector<std::pair<fastoml::SupportedBackends, GType>> backend_types = {
    {fastoml::TENSORFLOW, GST_TYPE_TENSORFLOW}};

static void gst_inference_backends_add_meta(const fastoml::BackendMeta* meta,
                                            gchar** backends_parameters,
                                            guint alignment);

GType gst_inference_backends_get_type(void) {
  static GType backend_type = 0;
  static const GEnumValue backend_desc[] = {
      {fastoml::TENSORFLOW, "TensorFlow Machine Learning Framework", "tensorflow"}, {0, NULL, NULL}};

  if (!backend_type) {
    backend_type = g_enum_register_static("GstInferenceBackends", (GEnumValue*)backend_desc);
  }
  return backend_type;
}

GType gst_inference_backends_search_type(guint id) {
  for (size_t i = 0; i < backend_types.size(); ++i) {
    if (backend_types[i].first == id) {
      return backend_types[i].second;
    }
  }

  return G_TYPE_INVALID;
}

static void gst_inference_backends_add_meta(const fastoml::BackendMeta* meta,
                                            gchar** backends_parameters,
                                            guint alignment) {
  GType backend_type = gst_inference_backends_search_type(meta->code);
  if (G_TYPE_INVALID == backend_type) {
    return;
  }

  GstBackend* backend = (GstBackend*)g_object_new(backend_type, NULL);

  gchar* backend_name = g_strdup_printf("%*s: %s. Version: %s\n", alignment, meta->name.c_str(),
                                        meta->description.c_str(), meta->version.c_str());

  gchar* parameters = gst_child_inspector_properties_to_string(G_OBJECT(backend), alignment, backend_name);

  if (NULL == *backends_parameters)
    *backends_parameters = parameters;
  else
    *backends_parameters = g_strconcat(*backends_parameters, "\n", parameters, NULL);

  g_object_unref(backend);
}

gchar* gst_inference_backends_get_string_properties(void) {
  gchar* backends_parameters = NULL;

  for (size_t i = 0; i < backend_types.size(); ++i) {
    const fastoml::BackendMeta* meta = nullptr;
    const fastoml::SupportedBackends type = backend_types[i].first;
    if (!fastoml::Backend::GetMeta(type, &meta)) {
      gst_inference_backends_add_meta(meta, &backends_parameters, DEFAULT_ALIGNMENT);
    }
  }

  return backends_parameters;
}

guint16 gst_inference_backends_get_default_backend(void) {
  return fastoml::TENSORFLOW;
}
