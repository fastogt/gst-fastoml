#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/math-compat.h>

#include <string.h>
#include "gstyolov2.h"

#define PLUGIN_NAME "yolov2"

GST_DEBUG_CATEGORY_STATIC(videobalance_debug);
#define GST_CAT_DEFAULT videobalance_debug

enum { PROP_0 };

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
static GstFlowReturn gst_video_transform(GstVideoFilter* vfilter, GstVideoFrame* frame);
static void gst_yolov2_init(GstYolov2* videobalance);
static void gst_yolov2_finalize(GObject* object);

#define gst_yolov2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstYolov2,
    gst_yolov2,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT(videobalance_debug, PLUGIN_NAME, 0, "debug category for yolov2 element"));

static void gst_yolov2_class_init(GstYolov2Class* klass) {
  GObjectClass* gobject_class = (GObjectClass*)klass;
  GstElementClass* gstelement_class = (GstElementClass*)klass;
  GstVideoFilterClass* vfilter_class = (GstVideoFilterClass*)klass;

  gobject_class->finalize = gst_yolov2_finalize;
  gobject_class->set_property = gst_yolov2_set_property;
  gobject_class->get_property = gst_yolov2_get_property;

  gst_element_class_set_static_metadata(gstelement_class, "Video analitics", "Filter/Effect/Video", "Detect objects",
                                        "Alexandr Topilski <support@fastotgt.com>");

  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_sink_template);
  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_src_template);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR(gst_video_set_info);
  vfilter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_video_transform);
}

// implementation

static void gst_video_balance_planar_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {}
static void gst_video_balance_semiplanar_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {}
static void gst_video_balance_packed_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {}
static void gst_video_balance_packed_rgb(GstYolov2* videobalance, GstVideoFrame* frame) {}

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
      goto unknown_format;
      break;
  }

  return TRUE;

  /* ERRORS */
unknown_format : {
  GST_ERROR_OBJECT(videobalance, "unknown format %" GST_PTR_FORMAT, incaps);
  return FALSE;
}
}

GstFlowReturn gst_video_transform(GstVideoFilter* vfilter, GstVideoFrame* frame) {
  GstYolov2* videobalance = GST_YOLOV2(vfilter);

  if (!videobalance->process) {
    GST_ERROR_OBJECT(videobalance, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }

  GST_OBJECT_LOCK(videobalance);
  videobalance->process(videobalance, frame);
  GST_OBJECT_UNLOCK(videobalance);
  return GST_FLOW_OK;
}

void gst_yolov2_init(GstYolov2* videobalance) {
  videobalance->process = NULL;
}

void gst_yolov2_finalize(GObject* object) {
  // GstYolov2* balance = GST_YOLOV2(object);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstYolov2* balance = GST_YOLOV2(object);
  GST_OBJECT_LOCK(balance);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(balance);
}

void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstYolov2* balance = GST_YOLOV2(object);
  GST_OBJECT_LOCK(balance);
  switch (prop_id) {
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
                  GST_PACKAGE_NAME,
                  GST_PACKAGE_ORIGIN)
