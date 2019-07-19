#ifndef __GST_INFERENCE_BACKENDS_H__
#define __GST_INFERENCE_BACKENDS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_INFERENCE_BACKENDS (gst_inference_backends_get_type())

GType gst_inference_backends_get_type(void);
gchar* gst_inference_backends_get_string_properties(void);
guint16 gst_inference_backends_get_default_backend(void);
GType gst_inference_backends_search_type(guint);

G_END_DECLS
#endif  //__GST_INFERENCE_BACKENDS_H__
