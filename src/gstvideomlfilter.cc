/*  Copyright (C) 2014-2019 FastoGT. All right reserved.
    This file is part of gst-fastoml.
    gst-fastoml is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    gst-fastoml is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with gst-fastoml. If not, see <http://www.gnu.org/licenses/>.
*/

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
enum { NEW_PREDICTION_SIGNAL, LAST_SIGNAL };

typedef struct _GstVideoMLFilterPrivate GstVideoMLFilterPrivate;
struct _GstVideoMLFilterPrivate {
  void (*process)(GstVideoMLFilter* balance, GstVideoFrame* in_frame, GstVideoFrame* out_frame);
  GstBackend* backend;
  GstVideoInfo* rgb_info;
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

static GstVideoInfo* create_rgb_video_info(GstVideoInfo* info) {
  GstCaps* caps = gst_video_info_to_caps(info);
  gst_caps_set_simple(caps, "format", G_TYPE_STRING, "RGB", NULL);
  GstVideoInfo* vinfo = gst_video_info_new();
  if (!gst_video_info_from_caps(vinfo, caps)) {
    return NULL;
  }

  return vinfo;
}

static GstVideoConverter* create_rgb_converter(GstVideoInfo* in_info, GstVideoInfo* out_info) {
  return gst_video_converter_new(
      in_info, out_info,
      gst_structure_new("GstVideoConvertConfig", GST_VIDEO_CONVERTER_OPT_THREADS, G_TYPE_UINT, 1, NULL));
}

#define gst_video_ml_filter_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstVideoMLFilter,
                        gst_video_ml_filter,
                        GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_video_ml_filter_debug_category,
                                                PLUGIN_NAME,
                                                0,
                                                "debug category for video ml filter element");
                        G_ADD_PRIVATE(GstVideoMLFilter));

static guint gst_video_inference_signals[LAST_SIGNAL] = {0};

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

  gst_video_inference_signals[NEW_PREDICTION_SIGNAL] =
      g_signal_new("new-prediction", GST_TYPE_VIDEO_ML_FILTER, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                   G_TYPE_POINTER);

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

static void gst_video_balance_packed_rgb_impl(GstVideoMLFilter* filter,
                                              GstVideoFrame* in_frame,
                                              GstVideoFrame* out_frame) {
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(filter);
  GstVideoInfo* info = &in_frame->info;
  size_t buffer_size = info->size * sizeof(matrix_data_t);
  matrix_data_t* matrix_data = (matrix_data_t*)g_malloc0(buffer_size);
  if (!matrix_data) {
    return;
  }

  if (filter->pre_process) {
    if (!filter->pre_process(filter, in_frame, matrix_data)) {
      g_free(matrix_data);
      return;
    }
  }

  gpointer prediction_data = NULL;
  gsize prediction_size = 0;
  GError* error = NULL;
  gboolean result =
      gst_backend_process_frame(priv->backend, in_frame, matrix_data, &prediction_data, &prediction_size, &error);
  if (!result) {
    g_error_free(error);
    g_free(matrix_data);
    return;
  }

  if (filter->post_process) {
    GstMeta* meta = gst_buffer_add_meta(out_frame->buffer, filter->inference_meta_info, NULL);
    gboolean is_valid;
    if (filter->post_process(filter, meta, prediction_data, prediction_size, &is_valid) && is_valid) {
      g_signal_emit(filter, gst_video_inference_signals[NEW_PREDICTION_SIGNAL], 0, meta);
    }
    // gst_buffer_remove_meta(out_frame->buffer, meta);
  }
  g_free(prediction_data);
  g_free(matrix_data);
}

static void gst_video_balance_packed_rgb(GstVideoMLFilter* filter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  gst_video_balance_packed_rgb_impl(filter, in_frame, out_frame);
  gst_video_frame_copy(out_frame, in_frame);
}

static GstVideoFrame* gst_create_rgb_frame(GstVideoMLFilter* filter, GstVideoFrame* in_frame, GstBuffer** out_buff) {
  if (!filter->convert) {
    return NULL;
  }

  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(filter);
  GstVideoFrame* converted_frame = g_slice_new0(GstVideoFrame);
  GstVideoInfo* info = priv->rgb_info;
  size_t buffer_size = info->size * sizeof(guchar);
  GstBuffer* dest = gst_buffer_new_allocate(NULL, buffer_size, NULL);
  if (!gst_video_frame_map(converted_frame, info, dest, GST_MAP_READWRITE)) {
    g_slice_free(GstVideoFrame, converted_frame);
    return NULL;
  }

  gst_video_converter_frame(filter->convert, in_frame, converted_frame);
  *out_buff = dest;
  return converted_frame;
}

static void gst_video_balance_planar_yuv(GstVideoMLFilter* filter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  GstBuffer* buff = NULL;
  GstVideoFrame* rgb_frame = gst_create_rgb_frame(filter, in_frame, &buff);
  if (!rgb_frame) {
    gst_video_frame_copy(out_frame, in_frame);
    return;
  }

  gst_video_balance_packed_rgb_impl(filter, rgb_frame, out_frame);
  gst_video_frame_unmap(rgb_frame);
  g_slice_free(GstVideoFrame, rgb_frame);
  gst_buffer_unref(buff);
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_semiplanar_yuv(GstVideoMLFilter* filter,
                                             GstVideoFrame* in_frame,
                                             GstVideoFrame* out_frame) {
  GstBuffer* buff = NULL;
  GstVideoFrame* rgb_frame = gst_create_rgb_frame(filter, in_frame, &buff);
  if (!rgb_frame) {
    gst_video_frame_copy(out_frame, in_frame);
    return;
  }

  gst_video_balance_packed_rgb_impl(filter, rgb_frame, out_frame);
  gst_video_frame_unmap(rgb_frame);
  g_slice_free(GstVideoFrame, rgb_frame);
  gst_buffer_unref(buff);
  gst_video_frame_copy(out_frame, in_frame);
}

static void gst_video_balance_packed_yuv(GstVideoMLFilter* filter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  GstBuffer* buff = NULL;
  GstVideoFrame* rgb_frame = gst_create_rgb_frame(filter, in_frame, &buff);
  if (!rgb_frame) {
    gst_video_frame_copy(out_frame, in_frame);
    return;
  }

  gst_video_balance_packed_rgb_impl(filter, rgb_frame, out_frame);
  gst_video_frame_unmap(rgb_frame);
  g_slice_free(GstVideoFrame, rgb_frame);
  gst_buffer_unref(buff);
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

  if (priv->rgb_info) {
    gst_video_info_free(priv->rgb_info);
    priv->rgb_info = NULL;
  }

  if (videobalance->convert) {
    gst_video_converter_free(videobalance->convert);
    videobalance->convert = NULL;
  }

  /* these must match */
  if (in_info->width != out_info->width || in_info->height != out_info->height || in_info->fps_n != out_info->fps_n ||
      in_info->fps_d != out_info->fps_d) {
    GST_ERROR_OBJECT(videobalance, "input and output formats do not match");
    return FALSE;
  }
  if (in_info->par_n != out_info->par_n || in_info->par_d != out_info->par_d) {
    GST_ERROR_OBJECT(videobalance, "input and output formats do not match");
    return FALSE;
  }

  if (in_info->interlace_mode != out_info->interlace_mode) {
    GST_ERROR_OBJECT(videobalance, "input and output formats do not match");
    return FALSE;
  }

  priv->process = NULL;

  switch (GST_VIDEO_INFO_FORMAT(in_info)) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
      priv->rgb_info = create_rgb_video_info(in_info);
      videobalance->convert = create_rgb_converter(in_info, priv->rgb_info);
      priv->process = gst_video_balance_planar_yuv;
      break;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_YVYU:
      priv->rgb_info = create_rgb_video_info(in_info);
      videobalance->convert = create_rgb_converter(in_info, priv->rgb_info);
      priv->process = gst_video_balance_packed_yuv;
      break;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      priv->rgb_info = create_rgb_video_info(in_info);
      videobalance->convert = create_rgb_converter(in_info, priv->rgb_info);
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

  GST_DEBUG("reconfigured %d %d", GST_VIDEO_INFO_FORMAT(in_info), GST_VIDEO_INFO_FORMAT(out_info));
  return TRUE;
}

GstFlowReturn gst_video_transform_frame(GstVideoFilter* vfilter, GstVideoFrame* in_frame, GstVideoFrame* out_frame) {
  GstVideoMLFilter* videobalance = GST_VIDEO_ML_FILTER(vfilter);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(videobalance);
  if (!priv->process) {
    GST_ERROR_OBJECT(videobalance, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }

  // GST_OBJECT_LOCK(videobalance);
  priv->process(videobalance, in_frame, out_frame);
  // GST_OBJECT_UNLOCK(videobalance);
  return GST_FLOW_OK;
}

void gst_video_ml_filter_init(GstVideoMLFilter* self) {
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(self);
  priv->process = NULL;
  self->convert = NULL;
  priv->backend = gst_backend_new(DEFAULT_PROP_BACKEND);
}

void gst_video_ml_filter_finalize(GObject* object) {
  GstVideoMLFilter* balance = GST_VIDEO_ML_FILTER(object);
  GstVideoMLFilterPrivate* priv = GST_VIDEO_ML_FILTER_PRIVATE(balance);

  priv->process = NULL;

  if (priv->rgb_info) {
    gst_video_info_free(priv->rgb_info);
    priv->rgb_info = NULL;
  }

  if (balance->convert) {
    gst_video_converter_free(balance->convert);
    balance->convert = NULL;
  }

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
