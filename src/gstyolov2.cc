#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstyolov2.h"
#include <fastoml/gst/gstbackend.h>

#define PLUGIN_NAME "yolov2"
#define PLUGIN_DESCRIPTION "FastoGT yolov2 plugin"

#define DEFAULT_PROP_BACKEND fastoml::TENSORFLOW

GST_DEBUG_CATEGORY_STATIC(videobalance_debug);
#define GST_CAT_DEFAULT videobalance_debug

enum { PROP_0, PROP_BACKEND };

typedef struct _GstYolov2Private GstYolov2Private;
struct _GstYolov2Private {
  GstBackend* backend;
};

#define GST_YOLOV2_PRIVATE(self) (GstYolov2Private*)(gst_yolov2_get_instance_private(self))

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

static void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);
static gboolean gst_video_set_info(GstVideoFilter* vfilter,
                                   GstCaps* incaps,
                                   GstVideoInfo* in_info,
                                   GstCaps* outcaps,
                                   GstVideoInfo* out_info);
static GstFlowReturn gst_video_transform_frame(GstVideoFilter* filter,
                                               GstVideoFrame* in_frame,
                                               GstVideoFrame* out_frame);
static void gst_yolov2_init(GstYolov2* self);
static void gst_yolov2_finalize(GObject* object);
static gboolean gst_yolov2_start(GstBaseTransform* trans);
static gboolean gst_yolov2_stop(GstBaseTransform* trans);

#define gst_yolov2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstYolov2,
    gst_yolov2,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT(videobalance_debug, PLUGIN_NAME, 0, "debug category for yolov2 element");
    G_ADD_PRIVATE(GstYolov2));

static void gst_yolov2_class_init(GstYolov2Class* klass) {
  GObjectClass* gobject_class = (GObjectClass*)klass;
  GstElementClass* gstelement_class = (GstElementClass*)klass;
  GstBaseTransformClass* trans_class = (GstBaseTransformClass*)klass;
  GstVideoFilterClass* vfilter_class = (GstVideoFilterClass*)klass;

  gobject_class->finalize = gst_yolov2_finalize;
  gobject_class->set_property = gst_yolov2_set_property;
  gobject_class->get_property = gst_yolov2_get_property;

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

  trans_class->start = GST_DEBUG_FUNCPTR(gst_yolov2_start);
  trans_class->stop = GST_DEBUG_FUNCPTR(gst_yolov2_stop);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR(gst_video_set_info);
  vfilter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_video_transform_frame);
}

// implementation

static void gst_video_balance_planar_yuv(GstYolov2* videobalance, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_semiplanar_yuv(GstYolov2* videobalance,
                                             GstVideoFrame* in_frame,
                                             GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_packed_yuv(GstYolov2* videobalance, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_packed_rgb(GstYolov2* videobalance, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  gst_video_frame_copy(out_frame, in_frame);
}

/* get notified of caps and plug in the correct process function */
gboolean gst_video_set_info(GstVideoFilter* vfilter,
                            GstCaps* incaps,
                            GstVideoInfo* in_info,
                            GstCaps* outcaps,
                            GstVideoInfo* out_info) {
  GstYolov2* videobalance = GST_YOLOV2(vfilter);

  GST_DEBUG_OBJECT(videobalance, "in %" GST_PTR_FORMAT " out %" GST_PTR_FORMAT, incaps, outcaps);

  videobalance->process = NULL;

  switch (GST_VIDEO_INFO_FORMAT(in_info)) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
      videobalance->process = gst_video_balance_planar_yuv;
      break;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_YVYU:
      videobalance->process = gst_video_balance_packed_yuv;
      break;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      videobalance->process = gst_video_balance_semiplanar_yuv;
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
      videobalance->process = gst_video_balance_packed_rgb;
      break;
    default:
      GST_ERROR_OBJECT(videobalance, "unknown format %" GST_PTR_FORMAT, incaps);
      return FALSE;
  }

  return TRUE;
}

GstFlowReturn gst_video_transform_frame(GstVideoFilter* vfilter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  GstYolov2* videobalance = GST_YOLOV2(vfilter);
  if (!videobalance->process) {
    GST_ERROR_OBJECT(videobalance, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }

  // GST_OBJECT_LOCK(videobalance);
  videobalance->process(videobalance, in_frame, out_frame);
  // GST_OBJECT_UNLOCK(videobalance);
  return GST_FLOW_OK;
}

void gst_yolov2_init(GstYolov2* self) {
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(self);

  self->process = NULL;
  priv->backend = gst_new_backend(DEFAULT_PROP_BACKEND);
}

void gst_yolov2_finalize(GObject* object) {
  GstYolov2* balance = GST_YOLOV2(object);
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(balance);

  balance->process = NULL;

  if (priv->backend) {
    gst_free_backend(priv->backend);
    priv->backend = NULL;
  }
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

gboolean gst_yolov2_start(GstBaseTransform* trans) {
  GstYolov2* self = GST_YOLOV2(trans);
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(self);
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

gboolean gst_yolov2_stop(GstBaseTransform* trans) {
  GstYolov2* self = GST_YOLOV2(trans);
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(self);
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

void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstYolov2* self = GST_YOLOV2(object);
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(self);

  GST_OBJECT_LOCK(self);
  switch (prop_id) {
    case PROP_BACKEND: {
      if (priv->backend) {
        gst_free_backend(priv->backend);
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

void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstYolov2* balance = GST_YOLOV2(object);
  GstYolov2Private* priv = GST_YOLOV2_PRIVATE(balance);

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

static gboolean plugin_init(GstPlugin* plugin) {
  return gst_element_register(plugin, PLUGIN_NAME, GST_RANK_NONE, GST_TYPE_YOLOV2);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  yolov2,
                  "Yolov2 plugin",
                  plugin_init,
                  PACKAGE_VERSION,
                  GST_LICENSE,
                  PLUGIN_DESCRIPTION,
                  GST_PACKAGE_ORIGIN)
