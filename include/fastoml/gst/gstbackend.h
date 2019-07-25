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
#include <fastoml/types.h>

#define GST_TYPE_BACKEND gst_backend_get_type()
G_DECLARE_DERIVABLE_TYPE(GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass {
  GObjectClass parent_class;
};

GstBackend* gst_backend_new(fastoml::SupportedBackends code);
void gst_backend_free(GstBackend*);

gboolean gst_backend_start(GstBackend*, GError**);
gboolean gst_backend_stop(GstBackend*, GError**);
gboolean gst_backend_process_frame(GstBackend*, GstVideoFrame*, matrix_data_t*, gpointer*, gsize*, GError**);
