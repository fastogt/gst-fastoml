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

#include "gsttinyyolov3.h"

#include <string.h>

#include <fastoml/gst/gstmlmeta.h>

#include <fastoml/gst/gstmlpostprocess.h>
#include <fastoml/gst/gstmlpreprocess.h>

#define PLUGIN_NAME "tinyyolov3"
#define PLUGIN_DESCRIPTION "FastoGT tinyyolov3 plugin"

#define MODEL_CHANNELS 3

/* Objectness threshold */
#define MAX_OBJ_THRESH 1
#define MIN_OBJ_THRESH 0
#define DEFAULT_OBJ_THRESH 0.50
/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.50
/* Intersection over union threshold */
#define MAX_IOU_THRESH 1
#define MIN_IOU_THRESH 0
#define DEFAULT_IOU_THRESH 0.40

GST_DEBUG_CATEGORY_STATIC(gst_tinyyolov3_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolov3_debug_category

enum {
  PROP_0,
  PROP_OBJ_THRESH,
  PROP_PROB_THRESH,
  PROP_IOU_THRESH,
};

/* prototypes */
static void gst_tinyyolov3_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void gst_tinyyolov3_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);

static void gst_tinyyolov3_init(GstTinyYolov3* self);
static void gst_tinyyolov3_dispose(GObject* object);
static void gst_tinyyolov3_finalize(GObject* object);

static gboolean gst_tinyyolov3_preprocess(GstVideoMLFilter* vi, GstVideoFrame* inframe, matrix_data_t* matrix_data);
static gboolean gst_tinyyolov3_postprocess(GstVideoMLFilter* vi,
                                           GstMeta* meta,
                                           const gpointer prediction,
                                           gsize predsize,
                                           gboolean* valid_prediction);

/* class initialization */

#define gst_tinyyolov3_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstTinyYolov3,
    gst_tinyyolov3,
    GST_TYPE_VIDEO_ML_FILTER,
    GST_DEBUG_CATEGORY_INIT(gst_tinyyolov3_debug_category, PLUGIN_NAME, 0, "debug category for tinyyolov3 element"));

static void gst_tinyyolov3_class_init(GstTinyYolov3Class* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstVideoMLFilterClass* vi_class = GST_VIDEO_ML_FILTER_CLASS(klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_tinyyolov3_finalize);
  gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_tinyyolov3_dispose);
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_tinyyolov3_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_tinyyolov3_get_property);
  gst_element_class_set_static_metadata(gstelement_class, "Tinyyolov3", "Tinyyolov3 detection", "Detect objects",
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

static void gst_tinyyolov3_init(GstTinyYolov3* self) {
  GstVideoMLFilter* vi_class = GST_VIDEO_ML_FILTER(self);
  self->obj_thresh = DEFAULT_OBJ_THRESH;
  self->prob_thresh = DEFAULT_PROB_THRESH;
  self->iou_thresh = DEFAULT_IOU_THRESH;

  vi_class->pre_process = GST_DEBUG_FUNCPTR(gst_tinyyolov3_preprocess);
  vi_class->post_process = GST_DEBUG_FUNCPTR(gst_tinyyolov3_postprocess);
  vi_class->inference_meta_info = gst_detection_meta_get_info();
}

void gst_tinyyolov3_dispose(GObject* object) {
  GstTinyYolov3* balance = GST_TINYYOLOV3(object);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

void gst_tinyyolov3_finalize(GObject* object) {
  GstTinyYolov3* balance = GST_TINYYOLOV3(object);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_tinyyolov3_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
  GstTinyYolov3* tinyyolov3 = GST_TINYYOLOV3(object);

  GST_DEBUG_OBJECT(tinyyolov3, "set_property");

  switch (property_id) {
    case PROP_OBJ_THRESH:
      tinyyolov3->obj_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(tinyyolov3, "Changed objectness threshold to %lf", tinyyolov3->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      tinyyolov3->prob_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(tinyyolov3, "Changed probability threshold to %lf", tinyyolov3->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      tinyyolov3->iou_thresh = g_value_get_double(value);
      GST_DEBUG_OBJECT(tinyyolov3, "Changed intersection over union threshold to %lf", tinyyolov3->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void gst_tinyyolov3_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
  GstTinyYolov3* tinyyolov3 = GST_TINYYOLOV3(object);

  switch (property_id) {
    case PROP_OBJ_THRESH:
      g_value_set_double(value, tinyyolov3->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      g_value_set_double(value, tinyyolov3->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      g_value_set_double(value, tinyyolov3->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static gboolean gst_tinyyolov3_preprocess(GstVideoMLFilter* vi, GstVideoFrame* inframe, matrix_data_t* matrix_data) {
  return gst_pixel_to_float(inframe, matrix_data, MODEL_CHANNELS);
}

static gboolean gst_tinyyolov3_postprocess(GstVideoMLFilter* vi,
                                           GstMeta* meta,
                                           const gpointer prediction,
                                           gsize predsize,
                                           gboolean* valid_prediction) {
  GstTinyYolov3* tinyyolov3 = GST_TINYYOLOV3(vi);
  GstDetectionMeta* detect_meta = (GstDetectionMeta*)(meta);
  gst_create_boxes_float(vi, prediction, detect_meta, valid_prediction, &detect_meta->boxes, &detect_meta->num_boxes,
                         tinyyolov3->obj_thresh, tinyyolov3->prob_thresh, tinyyolov3->iou_thresh);

  gboolean have_boxes = (detect_meta->num_boxes > 0) ? TRUE : FALSE;
  *valid_prediction = have_boxes;
  return TRUE;
}

static gboolean plugin_init(GstPlugin* plugin) {
  return gst_element_register(plugin, PLUGIN_NAME, GST_RANK_NONE, GST_TYPE_TINYYOLOV3);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  tinyyolov3,
                  "Tinyyolov3 plugin",
                  plugin_init,
                  PACKAGE_VERSION,
                  GST_LICENSE,
                  PLUGIN_DESCRIPTION,
                  GST_PACKAGE_ORIGIN)
