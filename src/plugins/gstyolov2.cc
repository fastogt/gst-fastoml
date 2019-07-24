#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstyolov2.h"

#include <math.h>

#include <fastoml/gst/gstmlmeta.h>

#include <fastoml/gst/gstmlpostprocess.h>
#include <fastoml/gst/gstmlpreprocess.h>

#define PLUGIN_NAME "yolov2"
#define PLUGIN_DESCRIPTION "FastoGT yolov2 plugin"

#define MEAN 0
#define STD 1 / 255.0
#define MODEL_CHANNELS 3

/* Objectness threshold */
#define MAX_OBJ_THRESH 1
#define MIN_OBJ_THRESH 0
#define DEFAULT_OBJ_THRESH 0.08
/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.08
/* Intersection over union threshold */
#define MAX_IOU_THRESH 1
#define MIN_IOU_THRESH 0
#define DEFAULT_IOU_THRESH 0.30

GST_DEBUG_CATEGORY_STATIC(gst_yolov2_debug_category);
#define GST_CAT_DEFAULT gst_yolov2_debug_category

enum {
  PROP_0,
  PROP_OBJ_THRESH,
  PROP_PROB_THRESH,
  PROP_IOU_THRESH,
};

static void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static void gst_yolov2_init(GstYolov2* self);
static void gst_yolov2_dispose(GObject* object);
static void gst_yolov2_finalize(GObject* object);

static gboolean gst_yolov2_preprocess(GstVideoMLFilter* vi, GstVideoFrame* inframe);
static gboolean gst_yolov2_postprocess(GstVideoMLFilter* videobalance,
                                       GstMeta* meta,
                                       const gpointer prediction,
                                       gsize predsize,
                                       gboolean* valid_prediction);

/* class initialization */

#define gst_yolov2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstYolov2,
    gst_yolov2,
    GST_TYPE_VIDEO_ML_FILTER,
    GST_DEBUG_CATEGORY_INIT(gst_yolov2_debug_category, PLUGIN_NAME, 0, "debug category for yolov2 element"));

static void gst_yolov2_class_init(GstYolov2Class* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstVideoMLFilterClass* vi_class = GST_VIDEO_ML_FILTER_CLASS(klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_yolov2_finalize);
  gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_yolov2_dispose);
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_yolov2_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_yolov2_get_property);
  gst_element_class_set_static_metadata(gstelement_class, "Yolov2", "Yolov2 detection", "Detect objects",
                                        "Alexandr Topilski <support@fastotgt.com>");

  g_object_class_install_property(
      gobject_class, PROP_OBJ_THRESH,
      g_param_spec_double("object-threshold", "obj-thresh", "Objectness threshold", MIN_OBJ_THRESH, MAX_OBJ_THRESH,
                          DEFAULT_OBJ_THRESH, G_PARAM_READWRITE));
  g_object_class_install_property(
      gobject_class, PROP_PROB_THRESH,
      g_param_spec_double("probability-threshold", "prob-thresh", "Class probability threshold", MIN_PROB_THRESH,
                          MAX_PROB_THRESH, DEFAULT_PROB_THRESH, G_PARAM_READWRITE));
  g_object_class_install_property(
      gobject_class, PROP_IOU_THRESH,
      g_param_spec_double("iou-threshold", "iou-thresh", "Intersection over union threshold to merge similar boxes",
                          MIN_IOU_THRESH, MAX_IOU_THRESH, DEFAULT_IOU_THRESH, G_PARAM_READWRITE));
}

gboolean gst_yolov2_preprocess(GstVideoMLFilter* vi, GstVideoFrame* inframe) {
  return gst_normalize(inframe, MEAN, STD, MODEL_CHANNELS);
}

gboolean gst_yolov2_postprocess(GstVideoMLFilter* vm,
                                GstMeta* meta,
                                const gpointer prediction,
                                gsize predsize,
                                gboolean* valid_prediction) {
  GstYolov2* self = GST_YOLOV2(vm);
  GstVideoMLFilter* vi = GST_VIDEO_ML_FILTER(self);
  GstDetectionMeta* detect_meta = (GstDetectionMeta*)(meta);
  gst_create_boxes(vi, prediction, detect_meta, valid_prediction, &detect_meta->boxes, &detect_meta->num_boxes,
                   self->obj_thresh, self->prob_thresh, self->iou_thresh);
  gboolean have_boxes = (detect_meta->num_boxes > 0) ? TRUE : FALSE;
  *valid_prediction = have_boxes;
  return TRUE;
}

void gst_yolov2_init(GstYolov2* self) {
  GstVideoMLFilter* vi_class = GST_VIDEO_ML_FILTER(self);
  self->obj_thresh = DEFAULT_OBJ_THRESH;
  self->prob_thresh = DEFAULT_PROB_THRESH;

  vi_class->pre_process = GST_DEBUG_FUNCPTR(gst_yolov2_preprocess);
  vi_class->post_process = GST_DEBUG_FUNCPTR(gst_yolov2_postprocess);
  vi_class->inference_meta_info = gst_detection_meta_get_info();
}

void gst_yolov2_dispose(GObject* object) {
  GstYolov2* balance = GST_YOLOV2(object);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

void gst_yolov2_finalize(GObject* object) {
  GstYolov2* balance = GST_YOLOV2(object);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstYolov2* self = GST_YOLOV2(object);
  GST_OBJECT_LOCK(self);
  switch (prop_id) {
    case PROP_OBJ_THRESH:
      self->obj_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(self, "Changed objectness threshold to %lf", self->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      self->prob_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(self, "Changed probability threshold to %lf", self->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      self->iou_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(self, "Changed intersection over union threshold to %lf", self->iou_thresh);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(self);
}

void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstYolov2* self = GST_YOLOV2(object);
  GST_OBJECT_LOCK(self);
  switch (prop_id) {
    case PROP_OBJ_THRESH:
      g_value_set_double(value, self->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      g_value_set_double(value, self->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      g_value_set_double(value, self->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(self);
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
