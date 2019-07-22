#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_EMBEDDING_META_API_TYPE (gst_embedding_meta_api_get_type())
#define GST_EMBEDDING_META_INFO (gst_embedding_meta_get_info())
#define GST_CLASSIFICATION_META_API_TYPE (gst_classification_meta_api_get_type())
#define GST_CLASSIFICATION_META_INFO (gst_classification_meta_get_info())
#define GST_DETECTION_META_API_TYPE (gst_detection_meta_api_get_type())
#define GST_DETECTION_META_INFO (gst_detection_meta_get_info())
/**
 * Basic bounding box structure for detection
 */
typedef struct _BBox BBox;
struct _BBox {
  gint label;
  gdouble prob;
  gdouble x;
  gdouble y;
  gdouble width;
  gdouble height;
};

/**
 * Implements the placeholder for embedding information.
 */
typedef struct _GstEmbeddingMeta GstEmbeddingMeta;
struct _GstEmbeddingMeta {
  GstMeta meta;
  gint num_dimensions;
  gdouble* embedding;
};

/**
 * Implements the placeholder for classification information.
 */
typedef struct _GstClassificationMeta GstClassificationMeta;
struct _GstClassificationMeta {
  GstMeta meta;
  gint num_labels;
  gdouble* label_probs;
};

/**
 * Implements the placeholder for detection information.
 */
typedef struct _GstDetectionMeta GstDetectionMeta;
struct _GstDetectionMeta {
  GstMeta meta;
  gint num_boxes;
  BBox* boxes;
};

GType gst_embedding_meta_api_get_type(void);
const GstMetaInfo* gst_embedding_meta_get_info(void);

GType gst_classification_meta_api_get_type(void);
const GstMetaInfo* gst_classification_meta_get_info(void);

GType gst_detection_meta_api_get_type(void);
const GstMetaInfo* gst_detection_meta_get_info(void);

G_END_DECLS
