#pragma once

#include <fastoml/gst/gstvideomlfilter.h>

G_BEGIN_DECLS

#define GST_TYPE_YOLOV2 (gst_yolov2_get_type())
#define GST_YOLOV2(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_YOLOV2, GstYolov2))
#define GST_YOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_YOLOV2, GstYolov2Class))
#define GST_IS_YOLOV2(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_YOLOV2))
#define GST_IS_YOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_YOLOV2))

typedef struct _GstYolov2 GstYolov2;
typedef struct _GstYolov2Class GstYolov2Class;

struct _GstYolov2 {
  GstVideoMLFilter videofilter;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstYolov2Class {
  GstVideoMLFilterClass parent_class;
};

GType gst_yolov2_get_type(void);

G_END_DECLS
