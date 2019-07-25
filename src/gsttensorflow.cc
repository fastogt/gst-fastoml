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

#include <fastoml/gst/gsttensorflow.h>

#include <fastoml/gst/gstbackend_subclass.h>

GST_DEBUG_CATEGORY_STATIC(gst_tensorflow_debug_category);
#define GST_CAT_DEFAULT gst_tensorflow_debug_category

struct _GstTensorflow {
  GstBackend parent;
};

#define INPUT_LAYER "input-layer"
#define OUTPUT_LAYER "output-layer"

#define DEFAULT_PROP_INPUT NULL
#define DEFAULT_PROP_OUTPUT NULL

enum { PROP_0, PROP_INPUT, PROP_OUTPUT };

G_DEFINE_TYPE_WITH_CODE(GstTensorflow,
                        gst_tensorflow,
                        GST_TYPE_BACKEND,
                        GST_DEBUG_CATEGORY_INIT(gst_tensorflow_debug_category,
                                                "tensorflow",
                                                0,
                                                "debug category for tensorflow parameters"));

static void gst_tenserflow_backend_set_property(GObject* object,
                                                guint property_id,
                                                const GValue* value,
                                                GParamSpec* pspec);
static void gst_tenserflow_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);

static void gst_tensorflow_class_init(GstTensorflowClass* klass) {
  GstBackendClass* bclass = GST_BACKEND_CLASS(klass);
  GObjectClass* oclass = G_OBJECT_CLASS(klass);

  oclass->set_property = gst_tenserflow_backend_set_property;
  oclass->get_property = gst_tenserflow_backend_get_property;

  g_object_class_install_property(oclass, PROP_INPUT,
                                  g_param_spec_string("input", "input", "Input layer", DEFAULT_PROP_INPUT,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(oclass, PROP_OUTPUT,
                                  g_param_spec_string("output", "output", "Input layer", DEFAULT_PROP_OUTPUT,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void gst_tensorflow_init(GstTensorflow* self) {
  gst_backend_set_code(GST_BACKEND(self), fastoml::TENSORFLOW);
}

void gst_tenserflow_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
  GstTensorflow* self = GST_TENSORFLOW(object);

  switch (property_id) {
    case PROP_INPUT: {
      gst_backend_set_property(GST_BACKEND(self), INPUT_LAYER, value);
      break;
    }
    case PROP_OUTPUT: {
      gst_backend_set_property(GST_BACKEND(self), OUTPUT_LAYER, value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
      break;
  }
}

void gst_tenserflow_backend_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
  GstTensorflow* self = GST_TENSORFLOW(object);

  switch (property_id) {
    case PROP_INPUT: {
      gst_backend_set_property(GST_BACKEND(self), INPUT_LAYER, value);
      break;
    }
    case PROP_OUTPUT: {
      gst_backend_set_property(GST_BACKEND(self), OUTPUT_LAYER, value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}
