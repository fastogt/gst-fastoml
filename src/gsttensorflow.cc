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

static void gst_tensorflow_init(GstTensorflow* self) {}

void gst_tenserflow_backend_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
  GstTensorflow* self = GST_TENSORFLOW(object);

  switch (property_id) {
    case PROP_INPUT: {
      GstState actual_state;
      gst_element_get_state(GST_ELEMENT(self), &actual_state, NULL, GST_SECOND);
      if (actual_state <= GST_STATE_READY) {
        gst_backend_set_property(GST_BACKEND(self), INPUT_LAYER, value);
      } else {
        GST_ERROR_OBJECT(object, "Input-layer can only be set in the NULL or READY states");
      }
      break;
    }
    case PROP_OUTPUT: {
      GstState actual_state;
      gst_element_get_state(GST_ELEMENT(self), &actual_state, NULL, GST_SECOND);
      if (actual_state <= GST_STATE_READY) {
        gst_backend_set_property(GST_BACKEND(self), OUTPUT_LAYER, value);
      } else {
        GST_ERROR_OBJECT(self, "Output-layer can only be set in the NULL or READY states");
      }
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
