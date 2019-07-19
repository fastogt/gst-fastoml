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

#define gst_yolov2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstYolov2,
    gst_yolov2,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT(videobalance_debug, PLUGIN_NAME, 0, "debug category for yolov2 element"));

static void gst_video_balance_planar_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {
  gint x, y;
  guint8* ydata;
  guint8 *udata, *vdata;
  gint ystride, ustride, vstride;
  gint width, height;
  gint width2, height2;
  guint8* tabley = videobalance->tabley;
  guint8** tableu = videobalance->tableu;
  guint8** tablev = videobalance->tablev;

  width = GST_VIDEO_FRAME_WIDTH(frame);
  height = GST_VIDEO_FRAME_HEIGHT(frame);

  ydata = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 0);
  ystride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);

  for (y = 0; y < height; y++) {
    guint8* yptr;

    yptr = ydata + y * ystride;
    for (x = 0; x < width; x++) {
      *yptr = tabley[*yptr];
      yptr++;
    }
  }

  width2 = GST_VIDEO_FRAME_COMP_WIDTH(frame, 1);
  height2 = GST_VIDEO_FRAME_COMP_HEIGHT(frame, 1);

  udata = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 1);
  vdata = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 2);
  ustride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 1);
  vstride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 2);

  for (y = 0; y < height2; y++) {
    guint8 *uptr, *vptr;
    guint8 u1, v1;

    uptr = udata + y * ustride;
    vptr = vdata + y * vstride;

    for (x = 0; x < width2; x++) {
      u1 = *uptr;
      v1 = *vptr;

      *uptr++ = tableu[u1][v1];
      *vptr++ = tablev[u1][v1];
    }
  }
}

static void gst_video_balance_semiplanar_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {
  gint x, y;
  guint8* ydata;
  guint8* uvdata;
  gint ystride, uvstride;
  gint width, height;
  gint width2, height2;
  guint8* tabley = videobalance->tabley;
  guint8** tableu = videobalance->tableu;
  guint8** tablev = videobalance->tablev;
  gint upos, vpos;

  width = GST_VIDEO_FRAME_WIDTH(frame);
  height = GST_VIDEO_FRAME_HEIGHT(frame);

  ydata = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 0);
  ystride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);

  for (y = 0; y < height; y++) {
    guint8* yptr;

    yptr = ydata + y * ystride;
    for (x = 0; x < width; x++) {
      *yptr = tabley[*yptr];
      yptr++;
    }
  }

  width2 = GST_VIDEO_FRAME_COMP_WIDTH(frame, 1);
  height2 = GST_VIDEO_FRAME_COMP_HEIGHT(frame, 1);

  uvdata = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 1);
  uvstride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 1);

  upos = GST_VIDEO_INFO_FORMAT(&frame->info) == GST_VIDEO_FORMAT_NV12 ? 0 : 1;
  vpos = GST_VIDEO_INFO_FORMAT(&frame->info) == GST_VIDEO_FORMAT_NV12 ? 1 : 0;

  for (y = 0; y < height2; y++) {
    guint8* uvptr;
    guint8 u1, v1;

    uvptr = uvdata + y * uvstride;

    for (x = 0; x < width2; x++) {
      u1 = uvptr[upos];
      v1 = uvptr[vpos];

      uvptr[upos] = tableu[u1][v1];
      uvptr[vpos] = tablev[u1][v1];
      uvptr += 2;
    }
  }
}

static void gst_video_balance_packed_yuv(GstYolov2* videobalance, GstVideoFrame* frame) {
  gint x, y, stride;
  guint8 *ydata, *udata, *vdata;
  gint yoff, uoff, voff;
  gint width, height;
  gint width2, height2;
  guint8* tabley = videobalance->tabley;
  guint8** tableu = videobalance->tableu;
  guint8** tablev = videobalance->tablev;

  width = GST_VIDEO_FRAME_WIDTH(frame);
  height = GST_VIDEO_FRAME_HEIGHT(frame);

  stride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);
  ydata = GST_VIDEO_FRAME_COMP_DATA(frame, 0);
  yoff = GST_VIDEO_FRAME_COMP_PSTRIDE(frame, 0);

  for (y = 0; y < height; y++) {
    guint8* yptr;

    yptr = ydata + y * stride;
    for (x = 0; x < width; x++) {
      *yptr = tabley[*yptr];
      yptr += yoff;
    }
  }

  width2 = GST_VIDEO_FRAME_COMP_WIDTH(frame, 1);
  height2 = GST_VIDEO_FRAME_COMP_HEIGHT(frame, 1);

  udata = GST_VIDEO_FRAME_COMP_DATA(frame, 1);
  vdata = GST_VIDEO_FRAME_COMP_DATA(frame, 2);
  uoff = GST_VIDEO_FRAME_COMP_PSTRIDE(frame, 1);
  voff = GST_VIDEO_FRAME_COMP_PSTRIDE(frame, 2);

  for (y = 0; y < height2; y++) {
    guint8 *uptr, *vptr;
    guint8 u1, v1;

    uptr = udata + y * stride;
    vptr = vdata + y * stride;

    for (x = 0; x < width2; x++) {
      u1 = *uptr;
      v1 = *vptr;

      *uptr = tableu[u1][v1];
      *vptr = tablev[u1][v1];

      uptr += uoff;
      vptr += voff;
    }
  }
}

static const int cog_ycbcr_to_rgb_matrix_8bit_sdtv[] = {
    298, 0, 409, -57068, 298, -100, -208, 34707, 298, 516, 0, -70870,
};

static const gint cog_rgb_to_ycbcr_matrix_8bit_sdtv[] = {
    66, 129, 25, 4096, -38, -74, 112, 32768, 112, -94, -18, 32768,
};

#define APPLY_MATRIX(m, o, v1, v2, v3) ((m[o * 4] * v1 + m[o * 4 + 1] * v2 + m[o * 4 + 2] * v3 + m[o * 4 + 3]) >> 8)

static void gst_video_balance_packed_rgb(GstYolov2* videobalance, GstVideoFrame* frame) {
  gint i, j, height;
  gint width, stride, row_wrap;
  gint pixel_stride;
  guint8* data;
  gint offsets[3];
  gint r, g, b;
  gint y, u, v;
  gint u_tmp, v_tmp;
  guint8* tabley = videobalance->tabley;
  guint8** tableu = videobalance->tableu;
  guint8** tablev = videobalance->tablev;

  width = GST_VIDEO_FRAME_WIDTH(frame);
  height = GST_VIDEO_FRAME_HEIGHT(frame);

  offsets[0] = GST_VIDEO_FRAME_COMP_OFFSET(frame, 0);
  offsets[1] = GST_VIDEO_FRAME_COMP_OFFSET(frame, 1);
  offsets[2] = GST_VIDEO_FRAME_COMP_OFFSET(frame, 2);

  data = (guint8*)GST_VIDEO_FRAME_PLANE_DATA(frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);

  pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE(frame, 0);
  row_wrap = stride - pixel_stride * width;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      r = data[offsets[0]];
      g = data[offsets[1]];
      b = data[offsets[2]];

      y = APPLY_MATRIX(cog_rgb_to_ycbcr_matrix_8bit_sdtv, 0, r, g, b);
      u_tmp = APPLY_MATRIX(cog_rgb_to_ycbcr_matrix_8bit_sdtv, 1, r, g, b);
      v_tmp = APPLY_MATRIX(cog_rgb_to_ycbcr_matrix_8bit_sdtv, 2, r, g, b);

      y = CLAMP(y, 0, 255);
      u_tmp = CLAMP(u_tmp, 0, 255);
      v_tmp = CLAMP(v_tmp, 0, 255);

      y = tabley[y];
      u = tableu[u_tmp][v_tmp];
      v = tablev[u_tmp][v_tmp];

      r = APPLY_MATRIX(cog_ycbcr_to_rgb_matrix_8bit_sdtv, 0, y, u, v);
      g = APPLY_MATRIX(cog_ycbcr_to_rgb_matrix_8bit_sdtv, 1, y, u, v);
      b = APPLY_MATRIX(cog_ycbcr_to_rgb_matrix_8bit_sdtv, 2, y, u, v);

      data[offsets[0]] = CLAMP(r, 0, 255);
      data[offsets[1]] = CLAMP(g, 0, 255);
      data[offsets[2]] = CLAMP(b, 0, 255);
      data += pixel_stride;
    }
    data += row_wrap;
  }
}

/* get notified of caps and plug in the correct process function */
static gboolean gst_video_balance_set_info(GstVideoFilter* vfilter,
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

static void gst_video_balance_before_transform(GstBaseTransform* base, GstBuffer* buf) {
  GstYolov2* balance = GST_YOLOV2(base);
  GstClockTime timestamp, stream_time;

  timestamp = GST_BUFFER_TIMESTAMP(buf);
  stream_time = gst_segment_to_stream_time(&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT(balance, "sync to %" GST_TIME_FORMAT, GST_TIME_ARGS(timestamp));

  if (GST_CLOCK_TIME_IS_VALID(stream_time))
    gst_object_sync_values(GST_OBJECT(balance), stream_time);
}

static GstCaps* gst_video_balance_transform_caps(GstBaseTransform* trans,
                                                 GstPadDirection direction,
                                                 GstCaps* caps,
                                                 GstCaps* filter) {
  GstYolov2* balance = GST_YOLOV2(trans);
  GstCaps* ret;

  if (filter) {
    ret = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
  } else {
    ret = gst_caps_ref(caps);
  }

  return ret;
}

static GstFlowReturn gst_video_balance_transform_frame_ip(GstVideoFilter* vfilter, GstVideoFrame* frame) {
  GstYolov2* videobalance = GST_YOLOV2(vfilter);

  if (!videobalance->process)
    goto not_negotiated;

  GST_OBJECT_LOCK(videobalance);
  videobalance->process(videobalance, frame);
  GST_OBJECT_UNLOCK(videobalance);

  return GST_FLOW_OK;

  /* ERRORS */
not_negotiated : {
  GST_ERROR_OBJECT(videobalance, "Not negotiated yet");
  return GST_FLOW_NOT_NEGOTIATED;
}
}

static void gst_yolov2_finalize(GObject* object) {
  GList* channels = NULL;
  GstYolov2* balance = GST_YOLOV2(object);

  g_free(balance->tableu[0]);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_yolov2_class_init(GstYolov2Class* klass) {
  GObjectClass* gobject_class = (GObjectClass*)klass;
  GstElementClass* gstelement_class = (GstElementClass*)klass;
  GstBaseTransformClass* trans_class = (GstBaseTransformClass*)klass;
  GstVideoFilterClass* vfilter_class = (GstVideoFilterClass*)klass;

  gobject_class->finalize = gst_yolov2_finalize;
  gobject_class->set_property = gst_yolov2_set_property;
  gobject_class->get_property = gst_yolov2_get_property;

  gst_element_class_set_static_metadata(gstelement_class, "Video analitics", "Filter/Effect/Video", "Detect objects",
                                        "Alexandr Topilski <support@fastotgt.com>");

  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_sink_template);
  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_balance_src_template);

  trans_class->before_transform = GST_DEBUG_FUNCPTR(gst_video_balance_before_transform);
  trans_class->transform_ip_on_passthrough = FALSE;
  trans_class->transform_caps = GST_DEBUG_FUNCPTR(gst_video_balance_transform_caps);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR(gst_video_balance_set_info);
  vfilter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_video_balance_transform_frame_ip);
}

static void gst_yolov2_init(GstYolov2* videobalance) {
  gint i;

  /* Initialize propertiews */
  videobalance->tableu[0] = g_new(guint8, 256 * 256 * 2);
  for (i = 0; i < 256; i++) {
    videobalance->tableu[i] = videobalance->tableu[0] + i * 256 * sizeof(guint8);
    videobalance->tablev[i] = videobalance->tableu[0] + 256 * 256 * sizeof(guint8) + i * 256 * sizeof(guint8);
  }
}

static void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstYolov2* balance = GST_YOLOV2(object);
  GST_OBJECT_LOCK(balance);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK(balance);
}

static void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstYolov2* balance = GST_YOLOV2(object);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
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
