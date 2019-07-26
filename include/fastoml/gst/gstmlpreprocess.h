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

#include <gst/video/video.h>

#include <fastoml/gst/gsttypes.h>

gboolean gst_normalize(GstVideoFrame* inframe,
                       matrix_data_t* matrix_data,
                       gfloat mean,
                       gfloat std,
                       gint model_channels);

gboolean gst_normalize_face(GstVideoFrame* inframe, matrix_data_t* matrix_data, gint model_channels);

gboolean gst_subtract_mean(GstVideoFrame* inframe,
                           matrix_data_t* matrix_data,
                           gfloat mean_red,
                           gfloat mean_green,
                           gfloat mean_blue,
                           gint model_channels);

gboolean gst_pixel_to_float(GstVideoFrame* inframe, matrix_data_t* matrix_data, gint model_channels);
