#pragma once

#include <gst/video/video.h>

#include <fastoml/gst/gsttypes.h>

/**
 * \brief Normalization with values between 0 and 1
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param mean The mean value of the channel
 * \param std  The standart deviation of the channel
 * \param model_channels The number of channels of the model
 */

gboolean gst_normalize(GstVideoFrame* inframe, matrix_data_t* matrix_data, gdouble mean, gdouble std, gint model_channels);

/**
 * \brief Especial normalization used for facenet
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param model_channels The number of channels of the model
 */

gboolean gst_normalize_face(GstVideoFrame* inframe, matrix_data_t* matrix_data, gint model_channels);

/**
 * \brief Substract the mean value to every pixel
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param mean_red The mean value of the channel red
 * \param mean_green The mean value of the channel green
 * \param mean_blue The mean value of the channel blue
 * \param model_channels The number of channels of the model
 */

gboolean gst_subtract_mean(GstVideoFrame* inframe,
                           matrix_data_t* matrix_data,
                           gdouble mean_red,
                           gdouble mean_green,
                           gdouble mean_blue,
                           gint model_channels);

/**
 * \brief Change every pixel value to float
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param model_channels The number of channels of the model
 */

gboolean gst_pixel_to_float(GstVideoFrame* inframe, matrix_data_t* matrix_data, gint model_channels);
