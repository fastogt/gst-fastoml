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

#include "gstdetectionoverlay.h"

#ifdef OCV_VERSION_LT_3_2
#include <opencv2/highgui/highgui.hpp>
#else
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#endif

#include <fastoml/gst/gstmlmeta.h>

#define PLUGIN_NAME "detectionoverlay"
#define PLUGIN_DESCRIPTION "FastoGT detectionoverlay plugin"

static const cv::Scalar colors[] = {
    cv::Scalar(254, 254, 254), cv::Scalar(239, 211, 127), cv::Scalar(225, 169, 0),   cv::Scalar(211, 127, 254),
    cv::Scalar(197, 84, 127),  cv::Scalar(183, 42, 0),    cv::Scalar(169, 0.0, 254), cv::Scalar(155, 42, 127),
    cv::Scalar(141, 84, 0),    cv::Scalar(127, 254, 254), cv::Scalar(112, 211, 127), cv::Scalar(98, 169, 0),
    cv::Scalar(84, 127, 254),  cv::Scalar(70, 84, 127),   cv::Scalar(56, 42, 0),     cv::Scalar(42, 0, 254),
    cv::Scalar(28, 42, 127),   cv::Scalar(14, 84, 0),     cv::Scalar(0, 254, 254),   cv::Scalar(14, 211, 127)};
#define N_C (sizeof(colors) / sizeof(cv::Scalar))

GST_DEBUG_CATEGORY_STATIC(gst_detection_overlay_debug_category);
#define GST_CAT_DEFAULT gst_detection_overlay_debug_category

/* prototypes */
static GstFlowReturn gst_detection_overlay_process_meta(GstVideoMLOverlay* overlay,
                                                        GstVideoFrame* frame,
                                                        GstMeta* meta,
                                                        gdouble font_scale,
                                                        gint thickness,
                                                        gchar** labels_list,
                                                        gint num_labels);

enum { PROP_0 };

/* class initialization */

#define gst_detection_overlay_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstDetectionOverlay,
                        gst_detection_overlay,
                        GST_TYPE_VIDEO_ML_OVERLAY,
                        GST_DEBUG_CATEGORY_INIT(gst_detection_overlay_debug_category,
                                                PLUGIN_NAME,
                                                0,
                                                "debug category for detection_overlay element"));

static void gst_detection_overlay_class_init(GstDetectionOverlayClass* klass) {
  GstVideoMLOverlayClass* io_class = GST_VIDEO_ML_OVERLAY_CLASS(klass);

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "detectionoverlay", "Filter",
                                        "Overlays detection metadata on input buffer",
                                        "Alexandr Topilski <support@fastotgt.com>");

  io_class->process_meta = GST_DEBUG_FUNCPTR(gst_detection_overlay_process_meta);
  io_class->meta_type = GST_DETECTION_META_API_TYPE;
}

static void gst_detection_overlay_init(GstDetectionOverlay* detection_overlay) {}

static GstFlowReturn gst_detection_overlay_process_meta(GstVideoMLOverlay* overlay,
                                                        GstVideoFrame* frame,
                                                        GstMeta* meta,
                                                        gdouble font_scale,
                                                        gint thickness,
                                                        gchar** labels_list,
                                                        gint num_labels) {
  gint channels;
  gint width = GST_VIDEO_FRAME_WIDTH(frame);
  gint height = GST_VIDEO_FRAME_HEIGHT(frame);
  char* frame_data = (char*)frame->data[0];

  cv::Mat cv_mat;
  GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(frame);
  switch (format) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
      cv_mat = cv::Mat(height * 3 / 2, width, CV_8UC1, frame_data);
      break;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_YVYU:
      return GST_FLOW_ERROR;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      return GST_FLOW_ERROR;
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRx:
      channels = 4;
      cv_mat = cv::Mat(height, width, CV_MAKETYPE(CV_8U, channels), frame_data);
      break;
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
      channels = 3;
      cv_mat = cv::Mat(height, width, CV_MAKETYPE(CV_8U, channels), frame_data);
      break;
    default:
      return GST_FLOW_ERROR;
  }

  GstDetectionMeta* detect_meta = (GstDetectionMeta*)meta;
  for (gint i = 0; i < detect_meta->num_boxes; ++i) {
    cv::String str;
    BBox box = detect_meta->boxes[i];
    if (num_labels > box.label) {
      str = labels_list[box.label];
    } else {
      str = cv::format("Label #%d", box.label);
    }

    cv::putText(cv_mat, str, cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_PLAIN, font_scale, colors[box.label % N_C],
                thickness);
    cv::rectangle(cv_mat, cv::Point(box.x, box.y), cv::Point(box.x + box.width, box.y + box.height),
                  colors[box.label % N_C], thickness);
  }
  return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin* plugin) {
  return gst_element_register(plugin, PLUGIN_NAME, GST_RANK_NONE, GST_TYPE_DETECTION_OVERLAY);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  detectionoverlay,
                  "ML detection overlay plugin",
                  plugin_init,
                  PACKAGE_VERSION,
                  GST_LICENSE,
                  PLUGIN_DESCRIPTION,
                  GST_PACKAGE_ORIGIN)
