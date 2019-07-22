#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fastoml/gst/gstvideomlfilter.h>

#include <fastoml/gst/gstbackend.h>

#define DEFAULT_PROP_BACKEND fastoml::TENSORFLOW
#define PLUGIN_NAME "videomlfilter"

GST_DEBUG_CATEGORY_STATIC(gst_video_ml_filter_debug_category);
#define GST_CAT_DEFAULT gst_video_ml_filter_debug_category

enum { PROP_0, PROP_BACKEND };

typedef struct _GstVideoMLFilterPrivate GstVideoMLFilterPrivate;
struct _GstVideoMLFilterPrivate {
  void (*process)(GstVideoMLFilter* balance,
                  GstVideoMLFilterClass* klass,
                  GstVideoFrame* in_frame,
                  GstVideoFrame* out_frame);
  GstBackend* backend;
};

#define GST_VIDEO_ML_FILTER_PRIVATE(self) (GstVideoMLFilterPrivate*)(gst_video_ml_filter_get_instance_private(self))

#define PROCESSING_CAPS                                \
  "{ AYUV, ARGB, BGRA, ABGR, RGBA, Y444, xRGB, RGBx, " \
  "xBGR, BGRx, RGB, BGR, Y42B, YUY2, UYVY, YVYU, "     \
  "I420, YV12, IYUV, Y41B, NV12, NV21 }"

static GstStaticPadTemplate gst_video_balance_src_template =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(PROCESSING_CAPS) ";"
                                                                                 "video/x-raw(ANY)"));

static GstStaticPadTemplate gst_video_balance_sink_template =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(PROCESSING_CAPS) ";"
                                                                                 "video/x-raw(ANY)"));

static void gst_video_ml_filter_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_video_ml_filter_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);
static gboolean gst_video_set_info(GstVideoFilter* vfilter,
                                   GstCaps* incaps,
                                   GstVideoInfo* in_info,
                                   GstCaps* outcaps,
                                   GstVideoInfo* out_info);
static GstFlowReturn gst_video_transform_frame(GstVideoFilter* filter,
                                               GstVideoFrame* in_frame,
                                               GstVideoFrame* out_frame);
static void gst_video_ml_filter_init(GstVideoMLFilter* self);
static void gst_video_ml_filter_finalize(GObject* object);
static gboolean gst_video_ml_filter_start(GstBaseTransform* trans);
static gboolean gst_video_ml_filter_stop(GstBaseTransform* trans);

#define gst_video_ml_filter_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstVideoMLFilter,
                        gst_video_ml_filter,
                        GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_video_ml_filter_debug_category,
                                                PLUGIN_NAME,
                                                0,
                                                "debug category for video ml filter element");
                        G_ADD_PRIVATE(GstVideoMLFilter));

static void gst_video_ml_filter_class_init(GstVideoMLFilterClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstBaseTransformClass* trans_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstVideoFilterClass* vfilter_class = GST_VIDEO_FILTER_CLASS(klass);

  gobject_class->finalize = gst_video_ml_filter_finalize;
  gobject_class->set_property = gst_video_ml_filter_set_property;
  gobject_class->get_property = gst_video_ml_filter_get_property;

  GParamSpec* backend_spec =
      g_param_spec_object("backend", "backend",
                          "Type of predefined backend to use (NULL = default tensorflow).\n"
                          "\t\t\tAccording to the selected backend "
                          "different properties will be available.",
                          GST_TYPE_BACKEND, GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property(gobject_class, PROP_BACKEND, backend_spec);

  gst_element_class_set_static_metadata(gstelement_class, "Video analitics", "Filter/Effect/Video", "Detect objects",
                                        "Alexandr Topilski <support@fastotgt.com>");

  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_sink_template);
  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR(gst_video_ml_filter_start);
  trans_class->stop = GST_DEBUG_FUNCPTR(gst_video_ml_filter_stop);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR(gst_video_set_info);
  vfilter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_video_transform_frame);
}

// implementation

static void gst_video_balance_planar_yuv(GstVideoMLFilter* balance,
                                         GstVideoMLFilterClass* klass,
                                         GstVideoFrame* in_frame,
                                         GstVideoFrame* out_frame) {
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(balance);
  gpointer prediction_data = NULL;
  gsize prediction_size = 0;
  GError* error = NULL;
  gboolean result = gst_backend_process_frame(priv->backend, in_frame, &prediction_data, &prediction_size, &error);
  if (result) {
    gboolean is_valid;
    if (klass->post_process) {
      klass->post_process(balance, prediction_data, prediction_size, &is_valid);
    }
  } else {
    g_error_free(error);
  }
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_semiplanar_yuv(GstVideoMLFilter* videobalance,
                                             GstVideoMLFilterClass* klass,
                                             GstVideoFrame* in_frame,
                                             GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_packed_yuv(GstVideoMLFilter* videobalance,
                                         GstVideoMLFilterClass* klass,
                                         GstVideoFrame* in_frame,
                                         GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_packed_rgb(GstVideoMLFilter* videobalance,
                                         GstVideoMLFilterClass* klass,
                                         GstVideoFrame* in_frame,
                                         GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

/* get notified of caps and plug in the correct process function */
gboolean gst_video_set_info(GstVideoFilter* vfilter,
                            GstCaps* incaps,
                            GstVideoInfo* in_info,
                            GstCaps* outcaps,
                            GstVideoInfo* out_info) {
  GstVideoMLFilter* videobalance = GST_VIDEO_ML_FILTER(vfilter);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(videobalance);

  GST_DEBUG_OBJECT(videobalance, "in %" GST_PTR_FORMAT " out %" GST_PTR_FORMAT, incaps, outcaps);

  priv->process = NULL;

  switch (GST_VIDEO_INFO_FORMAT(in_info)) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
      priv->process = gst_video_balance_planar_yuv;
      break;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_YVYU:
      priv->process = gst_video_balance_packed_yuv;
      break;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      priv->process = gst_video_balance_semiplanar_yuv;
      break;
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
      priv->process = gst_video_balance_packed_rgb;
      break;
    default:
      GST_ERROR_OBJECT(videobalance, "unknown format %" GST_PTR_FORMAT, incaps);
      return FALSE;
  }

  return TRUE;
}

GstFlowReturn gst_video_transform_frame(GstVideoFilter* vfilter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  GstVideoMLFilter* videobalance = GST_VIDEO_ML_FILTER(vfilter);
  GstVideoMLFilterClass* klass = GST_VIDEO_ML_FILTER_CLASS(videobalance);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(videobalance);
  if (!priv->process) {
    GST_ERROR_OBJECT(videobalance, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }

  // GST_OBJECT_LOCK(videobalance);
  priv->process(videobalance, klass, in_frame, out_frame);
  // GST_OBJECT_UNLOCK(videobalance);
  return GST_FLOW_OK;
}

void gst_video_ml_filter_init(GstVideoMLFilter* self) {
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(self);
  priv->process = NULL;
  priv->backend = gst_backend_new(DEFAULT_PROP_BACKEND);
}

void gst_video_ml_filter_finalize(GObject* object) {
  GstVideoMLFilter* balance = GST_VIDEO_ML_FILTER(object);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(balance);

  priv->process = NULL;

  if (priv->backend) {
    gst_backend_free(priv->backend);
    priv->backend = NULL;
  }
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

gboolean gst_video_ml_filter_start(GstBaseTransform* trans) {
  GstVideoMLFilter* self = GST_VIDEO_ML_FILTER(trans);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(self);
  if (!priv->backend) {
    return FALSE;
  }

  GError* err = NULL;
  if (!gst_backend_start(priv->backend, &err)) {
    g_error_free(err);
    return FALSE;
  }

  return TRUE;
}

gboolean gst_video_ml_filter_stop(GstBaseTransform* trans) {
  GstVideoMLFilter* self = GST_VIDEO_ML_FILTER(trans);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(self);
  if (!priv->backend) {
    return FALSE;
  }

  GError* err = NULL;
  if (!gst_backend_stop(priv->backend, &err)) {
    g_error_free(err);
    return FALSE;
  }
  return TRUE;
}

void gst_video_ml_filter_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstVideoMLFilter* self = GST_VIDEO_ML_FILTER(object);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(self);

  GST_OBJECT_LOCK(self);
  switch (prop_id) {
    case PROP_BACKEND: {
      if (priv->backend) {
        gst_backend_free(priv->backend);
      }
      priv->backend = (GstBackend*)g_value_get_object(value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(self);
}

void gst_video_ml_filter_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstVideoMLFilter* balance = GST_VIDEO_ML_FILTER(object);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(balance);

  GST_OBJECT_LOCK(balance);
  switch (prop_id) {
    case PROP_BACKEND: {
      g_value_set_object(value, priv->backend);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(balance);
}
