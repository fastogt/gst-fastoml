#include "gsttensorflow.h"

GST_DEBUG_CATEGORY_STATIC(gst_tensorflow_debug_category);
#define GST_CAT_DEFAULT gst_tensorflow_debug_category

struct _GstTensorflow {
  GstBackend parent;
};

G_DEFINE_TYPE_WITH_CODE(GstTensorflow,
                        gst_tensorflow,
                        GST_TYPE_BACKEND,
                        GST_DEBUG_CATEGORY_INIT(gst_tensorflow_debug_category,
                                                "tensorflow",
                                                0,
                                                "debug category for tensorflow parameters"));

static void gst_tensorflow_class_init(GstTensorflowClass* klass) {
  GstBackendClass* bclass = GST_BACKEND_CLASS(klass);
  GObjectClass* oclass = G_OBJECT_CLASS(klass);

  oclass->set_property = gst_backend_set_property;
  oclass->get_property = gst_backend_get_property;
  gst_backend_install_properties(bclass, fastoml::TENSORFLOW);
}

static void gst_tensorflow_init(GstTensorflow* self) {
  gst_backend_set_framework_code(GST_BACKEND(self), fastoml::TENSORFLOW);
}
