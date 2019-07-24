#pragma once

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_ML_FILTER (gst_video_ml_filter_get_type())
#define GST_VIDEO_ML_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_ML_FILTER, GstVideoMLFilter))
#define GST_VIDEO_ML_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_ML_FILTER, GstVideoMLFilterClass))
#define GST_IS_VIDEO_ML_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_ML_FILTER))
#define GST_IS_VIDEO_ML_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_ML_FILTER))

typedef struct _GstVideoMLFilter GstVideoMLFilter;
typedef struct _GstVideoMLFilterClass GstVideoMLFilterClass;

struct _GstVideoMLFilter {
  GstVideoFilter videofilter;

  GstVideoConverter* convert;
  const GstMetaInfo* inference_meta_info;

  gboolean (*pre_process)(GstVideoMLFilter* vi, GstVideoFrame* frame, gpointer matrix_data);
  gboolean (*post_process)(GstVideoMLFilter* videobalance,
                           GstMeta* meta,
                           const gpointer prediction,
                           gsize predsize,
                           gboolean* valid_prediction);
};

struct _GstVideoMLFilterClass {
  GstVideoFilterClass parent_class;
};

GType gst_video_ml_filter_get_type(void);

G_END_DECLS
