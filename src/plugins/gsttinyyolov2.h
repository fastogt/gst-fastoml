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

#include <fastoml/gst/gstvideomlfilter.h>

#define GST_TYPE_TINYYOLOV2 (gst_tinyyolov2_get_type())
#define GST_TINYYOLOV2(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_TINYYOLOV2, GstTinyYolov2))
#define GST_TINYYOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_TINYYOLOV2, GstTinyYolov2Class))
#define GST_IS_TINYYOLOV2(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_TINYYOLOV2))
#define GST_IS_TINYYOLOV2_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_TINYYOLOV2))

typedef struct _GstTinyYolov2 GstTinyYolov2;
typedef struct _GstTinyYolov2Class GstTinyYolov2Class;

struct _GstTinyYolov2 {
  GstVideoMLFilter videofilter;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstTinyYolov2Class {
  GstVideoMLFilterClass parent_class;
};

GType gst_tinyyolov2_get_type(void);
