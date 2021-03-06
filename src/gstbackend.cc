/*  Copyright (C) 2014-2019 FastoGT. All right reserved.
    This file is part of gst-fastoml.
    gst-fastoml is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    gst-fastoml is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with gst-fastoml. If not, see <http://www.gnu.org/licenses/>.
*/

#include <fastoml/gst/gstbackend.h>

#include <list>
#include <memory>

#include <fastoml/backend.h>

// implementations
#include <fastoml/gst/gsttensorflow.h>

GST_DEBUG_CATEGORY_STATIC(gst_backend_debug_category);
#define GST_CAT_DEFAULT gst_backend_debug_category

#define DEFAULT_PROP_MODEL NULL

enum { PROP_0, PROP_MODEL };

typedef struct _GstBackendPrivate GstBackendPrivate;
struct _GstBackendPrivate {
  fastoml::Backend* backend;
  gchar* model;
};

G_DEFINE_TYPE_WITH_CODE(GstBackend, gst_backend, G_TYPE_OBJECT, G_ADD_PRIVATE(GstBackend));

#define GST_BACKEND_PRIVATE(self) (GstBackendPrivate*)(gst_backend_get_instance_private(self))

static GQuark gst_backend_error_quark(void);
static void gst_backend_init(GstBackend* self);
static void gst_backend_finalize(GObject* obj);
static void gst_base_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void gst_base_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);

#define GST_BACKEND_ERROR gst_backend_error_quark()

static void gst_backend_class_init(GstBackendClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = gst_base_backend_set_property;
  gobject_class->get_property = gst_base_backend_get_property;

  g_object_class_install_property(gobject_class, PROP_MODEL,
                                  g_param_spec_string("model", "model", "Graph model path", DEFAULT_PROP_MODEL,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gobject_class->finalize = gst_backend_finalize;
}

void gst_backend_init(GstBackend* self) {
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  priv->model = g_strdup(DEFAULT_PROP_MODEL);
  priv->backend = NULL;
}

void gst_backend_finalize(GObject* object) {
  GstBackend* self = GST_BACKEND(object);
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_free(priv->model);
  priv->model = NULL;
  delete priv->backend;
  priv->backend = nullptr;
  G_OBJECT_CLASS(gst_backend_parent_class)->finalize(object);
}

void gst_base_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
  GstBackend* self = GST_BACKEND(object);
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);

  switch (property_id) {
    case PROP_MODEL: {
      g_free(priv->model);
      priv->model = g_value_dup_string(value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

void gst_base_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
  GstBackend* self = GST_BACKEND(object);
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);

  switch (property_id) {
    case PROP_MODEL: {
      g_value_set_string(value, priv->model);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

gboolean gst_backend_start(GstBackend* self, GError** error) {
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;
  if (!backend) {
    g_set_error(error, GST_BACKEND_ERROR, EINVAL, "Backend not set");
    GST_ERROR_OBJECT(self, "Backend not set");
    return FALSE;
  }

  if (!priv->model) {
    g_set_error(error, GST_BACKEND_ERROR, EINVAL, "Model path empty");
    GST_ERROR_OBJECT(self, "Model path empty");
    return FALSE;
  }

  common::ErrnoError err = backend->LoadGraph(common::file_system::ascii_file_string_path(priv->model));
  if (err) {
    g_set_error(error, GST_BACKEND_ERROR, err->GetErrorCode(), "Failed to load model graph");
    GST_ERROR_OBJECT(self, "Failed to load model graph");
    return FALSE;
  }

  err = backend->Start();
  if (err) {
    g_set_error(error, GST_BACKEND_ERROR, err->GetErrorCode(), "Failed to start the backend engine");
    GST_ERROR_OBJECT(self, "Failed to start the backend engine");
    return FALSE;
  }

  return TRUE;
}

gboolean gst_backend_stop(GstBackend* self, GError** error) {
  g_return_val_if_fail(error, FALSE);

  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(self);
  g_return_val_if_fail(priv, FALSE);
  fastoml::Backend* backend = priv->backend;
  if (!backend) {
    g_set_error(error, GST_BACKEND_ERROR, EINVAL, "Backend not set");
    GST_ERROR_OBJECT(self, "Backend not set");
    return FALSE;
  }

  common::ErrnoError err = backend->Stop();
  if (err) {
    g_set_error(error, GST_BACKEND_ERROR, err->GetErrorCode(), "Failed to stop the backend engine");
    GST_ERROR_OBJECT(self, "Failed to stop the backend engine");
    return FALSE;
  }

  return TRUE;
}

static size_t GetTopPrediction(fastoml::IPrediction* prediction) {
  size_t index = 0;
  float max = -1;
  size_t num_labels = prediction->GetResultSize() / sizeof(float);

  for (size_t i = 0; i < num_labels; ++i) {
    float current = 0;
    common::ErrnoError err = prediction->At(i, &current);
    if (err) {
      break;
    }

    if (current > max) {
      max = current;
      index = i;
    }
  }

  return index;
}

gboolean gst_backend_process_frame(GstBackend* self,
                                   GstVideoFrame* input_frame,
                                   matrix_data_t* matrix_data,
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
  common::ErrnoError err = backend->MakeFrame(size, fastoml::ImageFormat::RGB, matrix_data, &frame);
  if (err) {
    g_set_error(error, GST_BACKEND_ERROR, err->GetErrorCode(), "Failed to create frame");
    return FALSE;
  }

  fastoml::IPrediction* prediction = nullptr;
  err = backend->Predict(frame, &prediction);
  if (err) {
    delete frame;
    g_set_error(error, GST_BACKEND_ERROR, err->GetErrorCode(), "Failed to predict");
    return FALSE;
  }

  GetTopPrediction(prediction);
  *prediction_size = prediction->GetResultSize();

  /*could we avoid memory copy ?*/
  *prediction_data = g_malloc(*prediction_size);
  memcpy(*prediction_data, prediction->GetResultData(), *prediction_size);

  GST_LOG_OBJECT(self, "Size of prediction %p is %lu", *prediction_data, *prediction_size);

  delete frame;
  delete prediction;
  return TRUE;
}

GstBackend* gst_backend_new(fastoml::SupportedBackends code) {
  if (code == fastoml::TENSORFLOW) {
    return (GstBackend*)g_object_new(GST_TYPE_TENSORFLOW, NULL);
  }

  return NULL;
}

void gst_backend_free(GstBackend* backend) {
  g_clear_object(&backend);
}

GQuark gst_backend_error_quark(void) {
  static GQuark q = 0;
  if (0 == q) {
    q = g_quark_from_static_string("gst-backend-error-quark");
  }
  return q;
}

// subclass

gboolean gst_backend_set_code(GstBackend* backend, fastoml::SupportedBackends code) {
  g_return_val_if_fail(backend, FALSE);
  GstBackendPrivate* priv = GST_BACKEND_PRIVATE(backend);
  g_return_val_if_fail(backend, FALSE);

  fastoml::Backend* lbackend = nullptr;
  common::ErrnoError err = fastoml::Backend::MakeBackEnd(code, &lbackend);
  if (err) {
    return FALSE;
  }

  priv->backend = lbackend;
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
  common::ErrnoError err = back->GetProperty(property, &cvalue);
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
