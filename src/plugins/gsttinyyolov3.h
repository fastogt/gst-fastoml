#pragma once

#include <fastoml/gst/gstvideomlfilter.h>

G_BEGIN_DECLS

#define GST_TYPE_TINYYOLOV3 (gst_tinyyolov3_get_type())
#define GST_TINYYOLOV3(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_TINYYOLOV3, GstTinyYolov3))
#define GST_TINYYOLOV3_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_TINYYOLOV3, GstTinyYolov3Class))
#define GST_IS_TINYYOLOV3(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_TINYYOLOV3))
#define GST_IS_TINYYOLOV3_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_TINYYOLOV3))

typedef struct _GstTinyYolov3 GstTinyYolov3;
typedef struct _GstTinyYolov3Class GstTinyYolov3Class;

struct _GstTinyYolov3 {
  GstVideoMLFilter videofilter;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstTinyYolov3Class {
  GstVideoMLFilterClass parent_class;
};

GType gst_tinyyolov3_get_type(void);

G_END_DECLS
