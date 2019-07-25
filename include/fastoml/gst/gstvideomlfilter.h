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

#include <fastoml/gst/gsttypes.h>
#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video.h>

#define GST_TYPE_VIDEO_ML_FILTER (gst_video_ml_filter_get_type())
#define GST_VIDEO_ML_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_ML_FILTER, GstVideoMLFilter))
#define GST_VIDEO_ML_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_ML_FILTER, GstVideoMLFilterClass))
#define GST_IS_VIDEO_ML_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_ML_FILTER))
#define GST_IS_VIDEO_ML_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_ML_FILTER))

typedef struct _GstVideoMLFilter GstVideoMLFilter;
typedef struct _GstVideoMLFilterClass GstVideoMLFilterClass;

struct _GstVideoMLFilter {
  GstVideoFilter videofilter;

  GstVideoConverter* convert;
  const GstMetaInfo* inference_meta_info;

  gboolean (*pre_process)(GstVideoMLFilter* vi, GstVideoFrame* frame, matrix_data_t* matrix_data);
  gboolean (*post_process)(GstVideoMLFilter* videobalance,
                           GstMeta* meta,
                           const gpointer prediction,
                           gsize predsize,
                           gboolean* valid_prediction);
};

struct _GstVideoMLFilterClass {
  GstVideoFilterClass parent_class;
};

GType gst_video_ml_filter_get_type(void);
