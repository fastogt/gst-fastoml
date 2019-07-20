#include "gstbackend.h"

#include <list>
#include <memory>

#include <fastoml/backend.h>

// implementations
#include "gsttensorflow.h"

GST_DEBUG_CATEGORY_STATIC(gst_backend_debug_category);
#define GST_CAT_DEFAULT gst_backend_debug_category

enum { PROP_0, PROP_MODEL };

typedef struct _GstBackendPrivate GstBackendPrivate;
struct _GstBackendPrivate {
  GstMLBackends code;
  fastoml::Backend* backend;
};

G_DEFINE_TYPE_WITH_CODE(
    GstBackend,
    gst_backend,
    G_TYPE_OBJECT,
    GST_DEBUG_CATEGORY_INIT(gst_backend_debug_category, "backend", 0, "debug category for backend parameters");
    G_ADD_PRIVATE(GstBackend));

#define GST_BACKEND_PRIVATE(self) (GstBackendPrivate*)(gst_backend_get_instance_private(self))

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
  priv->code = GST_ML_BACKEND_TENSORFLOW;
}

void gst_backend_finalize(GObject* obj) {
  GstBackend* self = GST_BACKEND(obj);
  // GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  G_OBJECT_CLASS(gst_backend_parent_class)->finalize(obj);
}

void gst_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {}

void gst_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {}

gboolean gst_backend_start(GstBackend* self, const gchar* model_path, GError** error) {
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;

  g_return_val_if_fail(priv, FALSE);
  g_return_val_if_fail(error, FALSE);

  if (backend) {
    return TRUE;
  }

  fastoml::Backend* lbackend = nullptr;
  common::Error err = fastoml::Backend::MakeBackEnd(static_cast<fastoml::SupportedBackends>(priv->code), &lbackend);
  if (err) {
    return FALSE;
  }

  err = lbackend->LoadGraph(common::file_system::ascii_file_string_path(model_path));
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

  delete backend;
  priv->backend = nullptr;
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

GstMLBackends gst_backend_get_code(GstBackend* backend) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_val_if_fail(priv, GST_ML_BACKEND_TENSORFLOW);
  return priv->code;
}

GstBackend* gst_new_backend(GstMLBackends type) {
  if (type == GST_ML_BACKEND_TENSORFLOW) {
    return (GstBackend*)g_object_new(GST_TYPE_TENSORFLOW, NULL);
  }

  return NULL;
}

void gst_free_backend(GstBackend* backend) {
  g_clear_object(&backend);
}

// subclass

gboolean gst_backend_set_code(GstBackend* backend, GstMLBackends code) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_val_if_fail(priv, FALSE);
  priv->code = code;
  return TRUE;
}

void gst_backend_set_property(GstBackend* backend, const gchar* property, const GValue* value) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_if_fail(priv);
  fastoml::Backend* back = priv->backend;
  g_return_if_fail(back);

  if (G_VALUE_TYPE(value) == G_TYPE_STRING) {
    gchar* copy_value = g_value_dup_string(value);
    common::Value* value = common::Value::CreateStringValueFromBasicString(copy_value);
    back->SetProperty(property, value);
    delete value;
    g_free(copy_value);
  }
}

void gst_backend_get_property(GstBackend* backend, const gchar* property, GValue* value) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_if_fail(priv);
  fastoml::Backend* back = priv->backend;
  g_return_if_fail(back);

  common::Value* cvalue = nullptr;
  common::Error err = back->GetProperty(property, &cvalue);
  if (err) {
    return;
  }

  if (cvalue->GetType() == common::Value::TYPE_STRING) {
    common::Value::string_t val;
    if (cvalue->GetAsString(&val)) {
      g_value_set_string(value, val.data());
    }
  }
  delete cvalue;
}
