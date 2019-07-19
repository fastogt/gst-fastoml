#ifndef __GST_TENSORFLOW_H__
#define __GST_TENSORFLOW_H__

#include <gst/gst.h>

#include <gstbackend_fwd.h>

G_BEGIN_DECLS

#define GST_TYPE_TENSORFLOW gst_tensorflow_get_type()
G_DECLARE_FINAL_TYPE(GstTensorflow, gst_tensorflow, GST, TENSORFLOW, GstBackend);

G_END_DECLS

#endif  //__GST_TENSORFLOW_H__
