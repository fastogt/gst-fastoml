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

#include <fastoml/gst/gstvideomloverlay.h>

#define PROCESSING_CAPS                                \
  "{ AYUV, ARGB, BGRA, ABGR, RGBA, Y444, xRGB, RGBx, " \
  "xBGR, BGRx, RGB, BGR, Y42B, YUY2, UYVY, YVYU, "     \
  "I420, YV12, IYUV, Y41B, NV12, NV21 }"

static GstStaticPadTemplate gst_video_overlay_src_template =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(PROCESSING_CAPS) ";"
                                                                                 "video/x-raw(ANY)"));

static GstStaticPadTemplate gst_video_overlay_sink_template =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(PROCESSING_CAPS) ";"
                                                                                 "video/x-raw(ANY)"));

GST_DEBUG_CATEGORY_STATIC(gst_video_ml_overlay_debug_category);
#define GST_CAT_DEFAULT gst_video_ml_overlay_debug_category

#define MIN_FONT_SCALE 0
#define DEFAULT_FONT_SCALE 2
#define MAX_FONT_SCALE 100
#define MIN_THICKNESS 1
#define DEFAULT_THICKNESS 2
#define MAX_THICKNESS 100
#define DEFAULT_LABELS NULL
#define DEFAULT_NUM_LABELS 0

enum { PROP_0, PROP_FONT_SCALE, PROP_THICKNESS, PROP_LABELS };

typedef struct _GstVideoMLOverlayPrivate GstVideoMLOverlayPrivate;
struct _GstVideoMLOverlayPrivate {
  gdouble font_scale;
  gint thickness;
  gchar* labels;
  gchar** labels_list;
  gint num_labels;
};
/* prototypes */
static void gst_video_ml_overlay_set_property(GObject* object,
                                              guint property_id,
                                              const GValue* value,
                                              GParamSpec* pspec);
static void gst_video_ml_overlay_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);
static void gst_video_ml_overlay_dispose(GObject* object);
static void gst_video_ml_overlay_finalize(GObject* object);

static gboolean gst_video_ml_overlay_start(GstBaseTransform* trans);
static gboolean gst_video_ml_overlay_stop(GstBaseTransform* trans);
static GstFlowReturn gst_video_ml_overlay_transform_frame_ip(GstVideoFilter* trans, GstVideoFrame* frame);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstVideoMLOverlay,
                        gst_video_ml_overlay,
                        GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_video_ml_overlay_debug_category,
                                                "videomloverlay",
                                                0,
                                                "debug category for videomloverlay class");
                        G_ADD_PRIVATE(GstVideoMLOverlay));

#define GST_VIDEO_ML_OVERLAY_PRIVATE(self) (GstVideoMLOverlayPrivate*)(gst_video_ml_overlay_get_instance_private(self))

static void gst_video_ml_overlay_class_init(GstVideoMLOverlayClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstVideoFilterClass* video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_overlay_sink_template);
  gst_element_class_add_static_pad_template(gstelement_class, &gst_video_overlay_src_template);

  gobject_class->set_property = gst_video_ml_overlay_set_property;
  gobject_class->get_property = gst_video_ml_overlay_get_property;
  gobject_class->dispose = gst_video_ml_overlay_dispose;
  gobject_class->finalize = gst_video_ml_overlay_finalize;

  g_object_class_install_property(gobject_class, PROP_FONT_SCALE,
                                  g_param_spec_double("font-scale", "font", "Font scale", MIN_FONT_SCALE,
                                                      MAX_FONT_SCALE, DEFAULT_FONT_SCALE, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_THICKNESS,
                                  g_param_spec_int("thickness", "thickness", "Box line thickness in pixels",
                                                   MIN_THICKNESS, MAX_THICKNESS, DEFAULT_THICKNESS, G_PARAM_READWRITE));

  g_object_class_install_property(
      gobject_class, PROP_LABELS,
      g_param_spec_string("labels", "labels", "Semicolon separated string containing videoml labels", DEFAULT_LABELS,
                          G_PARAM_READWRITE));

  base_transform_class->start = GST_DEBUG_FUNCPTR(gst_video_ml_overlay_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_video_ml_overlay_stop);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_video_ml_overlay_transform_frame_ip);
}

static void gst_video_ml_overlay_init(GstVideoMLOverlay* overlay) {
  GstVideoMLOverlayPrivate* priv = GST_VIDEO_ML_OVERLAY_PRIVATE(overlay);
  priv->font_scale = DEFAULT_FONT_SCALE;
  priv->thickness = DEFAULT_THICKNESS;
  priv->labels = DEFAULT_LABELS;
  priv->labels_list = DEFAULT_LABELS;
  priv->num_labels = DEFAULT_NUM_LABELS;
}

void gst_video_ml_overlay_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(object);
  GstVideoMLOverlayPrivate* priv = GST_VIDEO_ML_OVERLAY_PRIVATE(overlay);

  switch (property_id) {
    case PROP_FONT_SCALE:
      priv->font_scale = g_value_get_double(value);
      break;
    case PROP_THICKNESS:
      priv->thickness = g_value_get_int(value);
      break;
    case PROP_LABELS:
      if (priv->labels != NULL) {
        g_free(priv->labels);
      }
      if (priv->labels_list != NULL) {
        g_strfreev(priv->labels_list);
      }
      priv->labels = g_value_dup_string(value);
      priv->labels_list = g_strsplit(g_value_get_string(value), ";", 0);
      priv->num_labels = g_strv_length(priv->labels_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

void gst_video_ml_overlay_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(object);
  GstVideoMLOverlayPrivate* priv = GST_VIDEO_ML_OVERLAY_PRIVATE(overlay);

  switch (property_id) {
    case PROP_FONT_SCALE:
      g_value_set_double(value, priv->font_scale);
      break;
    case PROP_THICKNESS:
      g_value_set_int(value, priv->thickness);
      break;
    case PROP_LABELS:
      g_value_set_string(value, priv->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

void gst_video_ml_overlay_dispose(GObject* object) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(object);
  GstVideoMLOverlayPrivate* priv = GST_VIDEO_ML_OVERLAY_PRIVATE(overlay);

  if (priv->labels_list != NULL) {
    g_strfreev(priv->labels_list);
  }
  if (priv->labels != NULL) {
    g_free(priv->labels);
  }

  G_OBJECT_CLASS(gst_video_ml_overlay_parent_class)->dispose(object);
}

void gst_video_ml_overlay_finalize(GObject* object) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(object);
  G_OBJECT_CLASS(gst_video_ml_overlay_parent_class)->finalize(object);
}

static gboolean gst_video_ml_overlay_start(GstBaseTransform* trans) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(trans);
  return TRUE;
}

static gboolean gst_video_ml_overlay_stop(GstBaseTransform* trans) {
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(trans);
  return TRUE;
}

/* transform */
static GstFlowReturn gst_video_ml_overlay_transform_frame_ip(GstVideoFilter* trans, GstVideoFrame* frame) {
  GstVideoMLOverlayClass* io_class = GST_VIDEO_ML_OVERLAY_GET_CLASS(trans);
  GstVideoMLOverlay* overlay = GST_VIDEO_ML_OVERLAY(trans);
  GstVideoMLOverlayPrivate* priv = GST_VIDEO_ML_OVERLAY_PRIVATE(overlay);

  GstMeta* meta = gst_buffer_get_meta(frame->buffer, io_class->meta_type);
  if (!meta) {
    return GST_FLOW_OK;
  }

  if (io_class->process_meta) {
    return io_class->process_meta(overlay, frame, meta, priv->font_scale, priv->thickness, priv->labels_list,
                                  priv->num_labels);
  }
  return GST_FLOW_ERROR;
}
