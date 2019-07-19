#include "gstbackend.h"

#include <list>
#include <memory>

#include <fastoml/backend.h>

GST_DEBUG_CATEGORY_STATIC(gst_backend_debug_category);
#define GST_CAT_DEFAULT gst_backend_debug_category

enum { PROP_0, PROP_MODEL };

typedef struct _GstBackendPrivate GstBackendPrivate;
struct _GstBackendPrivate {
  fastoml::SupportedBackends code;
  fastoml::Backend* backend;
};

G_DEFINE_TYPE_WITH_CODE(
    GstBackend,
    gst_backend,
    G_TYPE_OBJECT,
    GST_DEBUG_CATEGORY_INIT(gst_backend_debug_category, "backend", 0, "debug category for backend parameters");
    G_ADD_PRIVATE(GstBackend));

#define GST_BACKEND_PRIVATE(self) (GstBackendPrivate*)(gst_backend_get_instance_private(self))

static GParamSpec* gst_backend_param_to_spec(fastoml::ParameterMeta* param);
static int gst_backend_param_flags(int flags);
static void gst_backend_init(GstBackend* self);
static void gst_backend_finalize(GObject* obj);
static void gst_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void gst_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);

#define GST_BACKEND_ERROR gst_backend_error_quark()

static void gst_backend_class_init(GstBackendClass* klass) {
  GObjectClass* oclass = G_OBJECT_CLASS(klass);

  oclass->set_property = gst_backend_set_property;
  oclass->get_property = gst_backend_get_property;
  oclass->finalize = gst_backend_finalize;
}

void gst_backend_init(GstBackend* self) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  priv->code = fastoml::TENSORFLOW;
}

void gst_backend_finalize(GObject* obj) {
  GstBackend* self = GST_BACKEND(obj);
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  delete priv->backend;
  priv->backend = nullptr;

  G_OBJECT_CLASS(gst_backend_parent_class)->finalize(obj);
}

int gst_backend_param_flags(int flags) {
  int pflags = 0;

  if (fastoml::ParameterMeta::Flags::READ & flags) {
    pflags += G_PARAM_READABLE;
  }

  if (fastoml::ParameterMeta::Flags::WRITE & flags) {
    pflags += G_PARAM_WRITABLE;
  }

  return pflags;
}

GParamSpec* gst_backend_param_to_spec(fastoml::ParameterMeta* param) {
  GParamSpec* spec = NULL;

  switch (param->type) {
    case (common::Value::TYPE_STRING): {
      spec = g_param_spec_string(param->name.c_str(), param->name.c_str(), param->description.c_str(), NULL,
                                 (GParamFlags)gst_backend_param_flags(param->flags));
      break;
    }
    default:
      g_return_val_if_reached(NULL);
  }

  return spec;
}

void gst_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {}

void gst_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {}

gboolean gst_backend_start(GstBackend* self, const gchar* model_location, GError** error) {
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;

  g_return_val_if_fail(priv, FALSE);
  g_return_val_if_fail(model_location, FALSE);
  g_return_val_if_fail(error, FALSE);

  if (backend) {
    return TRUE;
  }

  fastoml::Backend* lbackend = nullptr;
  common::Error err = fastoml::Backend::MakeBackEnd(priv->code, &lbackend);
  if (err) {
    return FALSE;
  }

  err = lbackend->LoadGraph(common::file_system::ascii_file_string_path(model_location));
  if (err) {
    delete lbackend;
    GST_ERROR_OBJECT(self, "Failed to load model graph");
    return FALSE;
  }

  err = lbackend->Start();
  if (err) {
    delete lbackend;
    GST_ERROR_OBJECT(self, "Failed to start the backend engine");
    return FALSE;
  }

  priv->backend = lbackend;
  return TRUE;
}

gboolean gst_backend_stop(GstBackend* self, GError** error) {
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;
  g_return_val_if_fail(backend, FALSE);

  common::Error err = backend->Stop();
  if (err) {
    GST_ERROR_OBJECT(self, "Failed to stop the backend engine");
    return FALSE;
  }
  return TRUE;
}

gboolean gst_backend_process_frame(GstBackend* self,
                                   GstVideoFrame* input_frame,
                                   gpointer* prediction_data,
                                   gsize* prediction_size,
                                   GError** error) {
  g_return_val_if_fail(input_frame, FALSE);
  g_return_val_if_fail(prediction_data, FALSE);
  g_return_val_if_fail(prediction_size, FALSE);
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;
  g_return_val_if_fail(backend, FALSE);

  fastoml::IFrame* frame = nullptr;
  common::draw::Size size(input_frame->info.width, input_frame->info.height);
  GST_LOG_OBJECT(self, "Processing Frame of size %d x %d", input_frame->info.width, input_frame->info.height);
  common::Error err = backend->MakeFrame(size, fastoml::ImageFormat::RGB, input_frame->data, &frame);
  if (err) {
    return FALSE;
  }

  fastoml::IPrediction* prediction = nullptr;
  err = backend->Predict(frame, &prediction);
  if (err) {
    delete frame;
    return FALSE;
  }

  *prediction_size = prediction->GetResultSize();

  /*could we avoid memory copy ?*/
  *prediction_data = g_malloc(*prediction_size);
  memcpy(*prediction_data, prediction->GetResultData(), *prediction_size);

  GST_LOG_OBJECT(self, "Size of prediction %p is %lu", *prediction_data, *prediction_size);

  delete frame;
  delete prediction;
  return TRUE;
}

gboolean gst_backend_set_framework_code(GstBackend* backend, fastoml::SupportedBackends code) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_val_if_fail(priv, FALSE);
  priv->code = code;
  return TRUE;
}

guint gst_backend_get_framework_code(GstBackend* backend) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_val_if_fail(priv, FALSE);
  return priv->code;
}
