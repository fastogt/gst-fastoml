#pragma once

#include <fastoml/gst/gstvideomlfilter.h>

G_BEGIN_DECLS

#define GST_TYPE_TINYYOLOV2 (gst_tinyyolov2_get_type())
#define GST_TINYYOLOV2(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_TINYYOLOV2, GstTinyYolov2))
#define GST_TINYYOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_TINYYOLOV2, GstTinyYolov2Class))
#define GST_IS_TINYYOLOV2(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_TINYYOLOV2))
#define GST_IS_TINYYOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_TINYYOLOV2))

typedef struct _GstTinyYolov2 GstTinyYolov2;
typedef struct _GstTinyYolov2Class GstTinyYolov2Class;

struct _GstTinyYolov2 {
  GstVideoMLFilter videofilter;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstTinyYolov2Class {
  GstVideoMLFilterClass parent_class;
};

GType gst_tinyyolov2_get_type(void);

G_END_DECLS
