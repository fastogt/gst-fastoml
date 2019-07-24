#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_ML_OVERLAY gst_video_ml_overlay_get_type()
G_DECLARE_DERIVABLE_TYPE(GstVideoMLOverlay, gst_video_ml_overlay, GST, VIDEO_ML_OVERLAY, GstVideoFilter);

struct _GstVideoMLOverlayClass {
  GstVideoFilterClass parent_class;

  GstFlowReturn (*process_meta)(GstVideoMLOverlay* overlay,
                                GstVideoFrame* frame,
                                GstMeta* meta,
                                gdouble font_scale,
                                gint thickness,
                                gchar** labels_list,
                                gint num_labels);

  GType meta_type;
};

G_END_DECLS
