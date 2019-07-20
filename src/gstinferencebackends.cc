#include "gstinferencebackends.h"

GType gst_ml_backend_get_type(void) {
  static GType ml_type = 0;
  static const GEnumValue availible_backends[] = {
      {GST_ML_BACKEND_TENSORFLOW, "TensorFlow Machine Learning Framework", "tensorflow"}, {0, NULL, NULL}};

  if (!ml_type) {
    ml_type = g_enum_register_static("GstMlBackend", availible_backends);
  }
  return ml_type;
}
