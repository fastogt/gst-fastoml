#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstyolov2.h"

#include <math.h>

#include <fastoml/gst/gstmlmeta.h>

#define PLUGIN_NAME "yolov2"
#define PLUGIN_DESCRIPTION "FastoGT yolov2 plugin"

#define TOTAL_BOXES_5 845
#define DEFAULT_OBJ_THRESH 0.08
#define DEFAULT_PROB_THRESH 0.08

GST_DEBUG_CATEGORY_STATIC(gst_tinyyolov2_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolov2_debug_category

enum { PROP_0 };

static void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_yolov2_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static void gst_yolov2_init(GstYolov2* self);
static void gst_yolov2_finalize(GObject* object);

static gboolean gst_tinyyolov2_postprocess(GstVideoMLFilter* videobalance,
                                           const gpointer prediction,
                                           gsize predsize,
                                           gboolean* valid_prediction);

#define gst_yolov2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstYolov2,
    gst_yolov2,
    GST_TYPE_VIDEO_ML_FILTER,
    GST_DEBUG_CATEGORY_INIT(gst_tinyyolov2_debug_category, PLUGIN_NAME, 0, "debug category for yolov2 element"));

static void gst_yolov2_class_init(GstYolov2Class* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstVideoMLFilterClass* vi_class = GST_VIDEO_ML_FILTER_CLASS(klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_yolov2_finalize);
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_yolov2_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_yolov2_get_property);
  gst_element_class_set_static_metadata(gstelement_class, "Yolov2", "Yolov2 detection", "Detect objects",
                                        "Alexandr Topilski <support@fastotgt.com>");

  vi_class->post_process = GST_DEBUG_FUNCPTR(gst_tinyyolov2_postprocess);
}

static gdouble gst_sigmoid(gdouble x) {
  return 1.0 / (1.0 + pow(M_E, -1.0 * x));
}

static void gst_box_to_pixels(BBox* normalized_box, gint row, gint col, gint box) {
  gint grid_size = 32;
  const gfloat box_anchors[] = {1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52};

  g_return_if_fail(normalized_box != NULL);

  /* adjust the box center according to its cell and grid dim */
  normalized_box->x = (col + gst_sigmoid(normalized_box->x)) * grid_size;
  normalized_box->y = (row + gst_sigmoid(normalized_box->y)) * grid_size;

  /* adjust the lengths and widths */
  normalized_box->width = pow(M_E, normalized_box->width) * box_anchors[2 * box] * grid_size;
  normalized_box->height = pow(M_E, normalized_box->height) * box_anchors[2 * box + 1] * grid_size;
}

static void gst_get_boxes_from_prediction(gfloat obj_thresh,
                                          gfloat prob_thresh,
                                          gpointer prediction,
                                          BBox* boxes,
                                          gint* elements,
                                          gint grid_h,
                                          gint grid_w,
                                          gint boxes_size) {
  gint i, j, c, b;
  gint index;
  gdouble obj_prob;
  gdouble cur_class_prob, max_class_prob;
  gint max_class_prob_index;
  gint counter = 0;
  gint box_dim = 5;
  gint classes = 20;

  g_return_if_fail(boxes != NULL);
  g_return_if_fail(elements != NULL);

  /* Iterate rows */
  for (i = 0; i < grid_h; i++) {
    /* Iterate colums */
    for (j = 0; j < grid_w; j++) {
      /* Iterate boxes */
      for (b = 0; b < boxes_size; b++) {
        index = ((i * grid_w + j) * boxes_size + b) * (box_dim + classes);
        obj_prob = ((gfloat*)prediction)[index + 4];
        /* If the Objectness score is over the threshold add it to the boxes list */
        if (obj_prob > obj_thresh) {
          max_class_prob = 0;
          max_class_prob_index = 0;
          for (c = 0; c < classes; c++) {
            cur_class_prob = ((gfloat*)prediction)[index + box_dim + c];
            if (cur_class_prob > max_class_prob) {
              max_class_prob = cur_class_prob;
              max_class_prob_index = c;
            }
          }
          if (max_class_prob > prob_thresh) {
            BBox result;
            result.label = max_class_prob_index;
            result.prob = max_class_prob;
            result.x = ((gfloat*)prediction)[index];
            result.y = ((gfloat*)prediction)[index + 1];
            result.width = ((gfloat*)prediction)[index + 2];
            result.height = ((gfloat*)prediction)[index + 3];
            gst_box_to_pixels(&result, i, j, b);
            result.x = result.x - result.width * 0.5;
            result.y = result.y - result.height * 0.5;
            boxes[counter] = result;
            counter = counter + 1;
          }
        }
      }
    }
    *elements = counter;
  }
}

static gboolean gst_create_boxes(GstYolov2* vi,
                                 const gpointer prediction,
                                 GstDetectionMeta* detect_meta,
                                 gboolean* valid_prediction,
                                 BBox** resulting_boxes,
                                 gint* elements,
                                 gfloat obj_thresh,
                                 gfloat prob_thresh) {
  gint grid_h = 13;
  gint grid_w = 13;
  gint boxes_size = 5;
  BBox boxes[TOTAL_BOXES_5];
  *elements = 0;

  g_return_val_if_fail(vi != NULL, FALSE);
  g_return_val_if_fail(prediction != NULL, FALSE);
  g_return_val_if_fail(detect_meta != NULL, FALSE);
  g_return_val_if_fail(valid_prediction != NULL, FALSE);
  g_return_val_if_fail(resulting_boxes != NULL, FALSE);
  g_return_val_if_fail(elements != NULL, FALSE);

  gst_get_boxes_from_prediction(obj_thresh, prob_thresh, prediction, boxes, elements, grid_h, grid_w, boxes_size);

  *resulting_boxes = (BBox*)g_malloc(*elements * sizeof(BBox));
  memcpy(*resulting_boxes, boxes, *elements * sizeof(BBox));
  return TRUE;
}

gboolean gst_tinyyolov2_postprocess(GstVideoMLFilter* vm,
                                    const gpointer prediction,
                                    gsize predsize,
                                    gboolean* valid_prediction) {
  GstYolov2* self = GST_YOLOV2(vm);
  GstMeta* meta = gst_buffer_add_meta(NULL, self->inference_meta_info, NULL);
  GstDetectionMeta* detect_meta = (GstDetectionMeta*)meta;
  detect_meta->num_boxes = 0;

  gst_create_boxes(self, prediction, detect_meta, valid_prediction, &detect_meta->boxes, &detect_meta->num_boxes,
                   DEFAULT_OBJ_THRESH, DEFAULT_PROB_THRESH);
  *valid_prediction = (detect_meta->num_boxes > 0) ? TRUE : FALSE;
  return TRUE;
}

void gst_yolov2_init(GstYolov2* self) {
  self->inference_meta_info = gst_detection_meta_get_info();
}

void gst_yolov2_finalize(GObject* object) {
  GstYolov2* balance = GST_YOLOV2(object);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

void gst_yolov2_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstYolov2* self = GST_YOLOV2(object);
  GST_OBJECT_LOCK(self);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(self);
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
                  PLUGIN_DESCRIPTION,
                  GST_PACKAGE_ORIGIN)
