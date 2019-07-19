#ifndef __GST_VIDEO_INFERENCE_H__
#define __GST_VIDEO_INFERENCE_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_INFERENCE gst_video_inference_get_type()
G_DECLARE_DERIVABLE_TYPE(GstVideoInference, gst_video_inference, GST, VIDEO_INFERENCE, GstElement);

struct _GstVideoInferenceClass {
  GstElementClass parent_class;

  gboolean (*start)(GstVideoInference* self);
  gboolean (*stop)(GstVideoInference* self);
  gboolean (*preprocess)(GstVideoInference* self, GstVideoFrame* inframe, GstVideoFrame* outframe);
  gboolean (*postprocess)(GstVideoInference* self,
                          const gpointer prediction,
                          gsize size,
                          GstMeta* meta_model,
                          GstVideoInfo* info_model,
                          gboolean* valid_prediction);

  const GstMetaInfo* inference_meta_info;
};

G_END_DECLS

#endif  //__GST_VIDEO_INFERENCE_H__
