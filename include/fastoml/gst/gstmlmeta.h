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

#pragma once

#include <gst/gst.h>

#define GST_EMBEDDING_META_API_TYPE (gst_embedding_meta_api_get_type())
#define GST_EMBEDDING_META_INFO (gst_embedding_meta_get_info())
#define GST_CLASSIFICATION_META_API_TYPE (gst_classification_meta_api_get_type())
#define GST_CLASSIFICATION_META_INFO (gst_classification_meta_get_info())
#define GST_DETECTION_META_API_TYPE (gst_detection_meta_api_get_type())
#define GST_DETECTION_META_INFO (gst_detection_meta_get_info())

typedef struct _BBox BBox;
struct _BBox {
  gint label;
  gdouble prob;
  gdouble x;
  gdouble y;
  gdouble width;
  gdouble height;
};

typedef struct _GstEmbeddingMeta GstEmbeddingMeta;
struct _GstEmbeddingMeta {
  GstMeta meta;
  gint num_dimensions;
  gdouble* embedding;
};

typedef struct _GstClassificationMeta GstClassificationMeta;
struct _GstClassificationMeta {
  GstMeta meta;
  gint num_labels;
  gdouble* label_probs;
};

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
