#pragma once

#include <fastoml/gst/gstmlmeta.h>
#include <fastoml/gst/gstvideomlfilter.h>

G_BEGIN_DECLS

/**
 * \brief Fill all the classification meta with predictions
 *
 * \param class_meta Meta to fill
 * \param prediction Value of the prediction
 * \param predsize Size of the prediction
 */

gboolean gst_fill_classification_meta(GstClassificationMeta* class_meta, const gpointer prediction, gsize predsize);

/**
 * \brief Fill all the detection meta with the boxes
 *
 * \param vi Father object of every architecture
 * \param prediction Value of the prediction
 * \param detect_meta Meta to fill
 * \param valid_prediction Check if the prediction is valid
 * \param resulting_boxes The output boxes of the prediction
 * \param elements The number of objects
 * \param obj_thresh Objectness threshold
 * \param prob_thresh Class probability threshold
 * \param iou_thresh Intersection over union threshold
 */
gboolean gst_create_boxes(GstVideoMLFilter* vi,
                          const gpointer prediction,
                          GstDetectionMeta* detect_meta,
                          gboolean* valid_prediction,
                          BBox** resulting_boxes,
                          gint* elements,
                          gfloat obj_thresh,
                          gfloat prob_thresh,
                          gfloat iou_thresh);

/**
 * \brief Fill all the detection meta with the boxes
 *
 * \param vi Father object of every architecture
 * \param prediction Value of the prediction
 * \param detect_meta Meta to fill
 * \param valid_prediction Check if the prediction is valid
 * \param resulting_boxes The output boxes of the prediction
 * \param elements The number of objects
 * \param obj_thresh Objectness threshold
 * \param prob_thresh Class probability threshold
 * \param iou_thresh Intersection over union threshold
 */
gboolean gst_create_boxes_float(GstVideoMLFilter* vi,
                                const gpointer prediction,
                                GstDetectionMeta* detect_meta,
                                gboolean* valid_prediction,
                                BBox** resulting_boxes,
                                gint* elements,
                                gdouble obj_thresh,
                                gdouble prob_thresh,
                                gdouble iou_thresh);

G_END_DECLS
