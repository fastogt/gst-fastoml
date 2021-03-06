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
#include <gst/video/video.h>

#define GST_TYPE_VIDEO_ML_OVERLAY gst_video_ml_overlay_get_type()
G_DECLARE_DERIVABLE_TYPE(GstVideoMLOverlay, gst_video_ml_overlay, GST, VIDEO_ML_OVERLAY, GstVideoFilter);

struct _GstVideoMLOverlayClass {
  GstVideoFilterClass parent_class;

  GstFlowReturn (*process_meta)(GstVideoMLOverlay* overlay,
                                GstVideoFrame* frame,
                                GstMeta* meta,
                                gdouble font_scale,
                                gint thickness,
                                gchar** labels_list,
                                gint num_labels);

  GType meta_type;
};
