#pragma once

#include <fastoml/gst/gstvideomloverlay.h>

G_BEGIN_DECLS

#define GST_TYPE_DETECTION_OVERLAY (gst_detection_overlay_get_type())
#define GST_DETECTION_OVERLAY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DETECTION_OVERLAY, GstDetectionOverlay))
#define GST_DETECTION_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DETECTION_OVERLAY, GstDetectionOverlayClass))
#define GST_IS_DETECTION_OVERLAY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DETECTION_OVERLAY))
#define GST_IS_DETECTION_OVERLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_DETECTION_OVERLAY))

typedef struct _GstDetectionOverlay GstDetectionOverlay;
typedef struct _GstDetectionOverlayClass GstDetectionOverlayClass;

struct _GstDetectionOverlay {
  GstVideoMLOverlay overlay;
};

struct _GstDetectionOverlayClass {
  GstVideoMLOverlayClass parent_class;
};

GType gst_detection_overlay_get_type(void);

G_END_DECLS
