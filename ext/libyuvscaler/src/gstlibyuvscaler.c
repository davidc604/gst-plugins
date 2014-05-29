/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2013  <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-libyuvscaler
 *
 * Performs I420 scaling using the libyuv
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! libyuvscaler ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>

#include "gstlibyuvscaler.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include <string.h>

// open source libyuv
#include "libyuv.h"

GST_DEBUG_CATEGORY_STATIC (gst_libyuvscaler_debug);
#define GST_CAT_DEFAULT gst_libyuvscaler_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_PERFORMANCE);

GType gst_libyuvscaler_get_type (void);

static GQuark _colorspace_quark;

#define gst_libyuvscaler_parent_class parent_class
G_DEFINE_TYPE (Gstlibyuvscaler, gst_libyuvscaler, GST_TYPE_VIDEO_FILTER);

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

#define SINK_CAPS_STR GST_VIDEO_CAPS_MAKE ("I420")
#define SRC_CAPS_STR GST_VIDEO_CAPS_MAKE ("I420")

 /* FIXME: if we can do NV12, NV21 shouldn't be so hard */
#define GST_VIDEO_FORMATS "{ I420 }"


static GstStaticCaps gst_libyuvscaler_format_caps =
GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS));

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
};


static GstStaticPadTemplate gst_libyuvscaler_src_template =
GST_STATIC_PAD_TEMPLATE (
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS_STR)
);


static GstStaticPadTemplate gst_libyuvscaler_sink_template =
GST_STATIC_PAD_TEMPLATE (
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS_STR)
);


static void gst_libyuvscaler_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_libyuvscaler_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GObject vmethod implementations */
static GstCaps * gst_libyuvscaler_transform_caps (GstBaseTransform * btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);

static GstCaps * gst_libyuvscaler_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);

static gboolean gst_libyuvscaler_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params);

static gboolean gst_libyuvscaler_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf);

static gboolean gst_libyuvscaler_set_info (GstVideoFilter * filter,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info);

static GstFlowReturn gst_libyuvscaler_transform_frame (GstVideoFilter *filter,
  GstVideoFrame *in_frame, GstVideoFrame *out_frame);


/* initialize the libyuvscaler's class */
static void
gst_libyuvscaler_class_init (GstlibyuvscalerClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *gstbasetransform_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *gstvideofilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_libyuvscaler_set_property;
  gobject_class->get_property = gst_libyuvscaler_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_libyuvscaler_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_libyuvscaler_sink_template));

  gst_element_class_set_static_metadata (gstelement_class, "I420 Scaler",
    "Filter/Converter/Video",
    "Scales I420 frames using libyuv",
    "David Chen <david@remotium.com>");

  gstbasetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_transform_caps);

  gstbasetransform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_fixate_caps);

  gstbasetransform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_filter_meta);

  gstbasetransform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_transform_meta);

  gstbasetransform_class->passthrough_on_same_caps = TRUE;

  gstvideofilter_class->set_info =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_set_info);

  gstvideofilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_libyuvscaler_transform_frame);

  /* debug category for fltering log messages
   *
   * FIXME:exchange the string 'Template libyuvscaler' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_libyuvscaler_debug, "libyuvscaler", 0, "Scales I420 frames using libyuv");
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_libyuvscaler_init (Gstlibyuvscaler * filter)
{
}

static void
gst_libyuvscaler_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
/*
  Gstlibyuvscaler *filter = GST_LIBYUVSCALER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
*/
}

static void
gst_libyuvscaler_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
/*
  Gstlibyuvscaler *filter = GST_LIBYUVSCALER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
*/
}

/* GstElement vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn
gst_libyuvscaler_transform_frame (GstVideoFilter *filter, GstVideoFrame *in_frame, GstVideoFrame *out_frame)
{
  Gstlibyuvscaler *scaler = GST_LIBYUVSCALER_CAST (filter);
  gint in_width, in_height, in_stride, in_uv_stride;
  gint out_width, out_height, out_stride, out_uv_stride;
  guint8 *in[3];
  guint8 *out[3];
  gint i;

  in_width = GST_VIDEO_FRAME_WIDTH (in_frame);
  in_height = GST_VIDEO_FRAME_HEIGHT (in_frame);
  in_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
  in_uv_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);

  out_width = GST_VIDEO_FRAME_WIDTH (out_frame);
  out_height = GST_VIDEO_FRAME_HEIGHT (out_frame);
  out_stride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);
  out_uv_stride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 1);

  for (i = 0; i < GST_VIDEO_FRAME_N_PLANES (in_frame); i++) {
    in[i] = GST_VIDEO_FRAME_PLANE_DATA (in_frame, i);
    out[i] = GST_VIDEO_FRAME_PLANE_DATA (out_frame, i);
  }

  GST_INFO ("input: width %d height %d", in_width, in_height);
  GST_INFO ("output: width %d height %d", out_width, out_height);

  I420Scale(in[0], in_stride,
            in[1], in_uv_stride,
            in[2], in_uv_stride,
            in_width, in_height,
            out[0], out_stride,
            out[1], out_uv_stride,
            out[2], out_uv_stride,
            out_width, out_height,
            2);

  return GST_FLOW_OK;
}

static gboolean
gst_libyuvscaler_set_info (GstVideoFilter * filter,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info)
{
  Gstlibyuvscaler *scaler = GST_LIBYUVSCALER_CAST (filter);

  if (in_info->fps_n != out_info->fps_n || in_info->fps_d != out_info->fps_d)
    goto format_mismatch;

  /* if present, these must match too */
  if (in_info->par_n != out_info->par_n || in_info->par_d != out_info->par_d)
    goto format_mismatch;

  /* if present, these must match too */
  if (in_info->interlace_mode != out_info->interlace_mode)
    goto format_mismatch;

  if (in_info->width == out_info->width && in_info->height == out_info->height) {
    gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (filter), TRUE);
  }

  return TRUE;

      /* ERRORS */
format_mismatch:
    {
      GST_ERROR_OBJECT (scaler, "input and output formats do not match");
      return FALSE;
    }
}

/* copies the given caps */
static GstCaps *
gst_libyuvscaler_caps_remove_format_info (GstCaps * caps)
{
  GstStructure *st;
  gint i, n;
  GstCaps *res;

  res = gst_caps_new_empty ();

  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    st = gst_caps_get_structure (caps, i);

    /* If this is already expressed by the existing caps
     * skip this structure */
    if (i > 0 && gst_caps_is_subset_structure (res, st))
      continue;

    st = gst_structure_copy (st);
    gst_structure_remove_fields (st, "format",
        "colorimetry", "chroma-site", NULL);

    gst_caps_append_structure (res, st);
  }

  return res;
}

#if 0
static GstCaps *
gst_libyuvscaler_transform_caps (GstBaseTransform * btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  Gstlibyuvscaler *scaler = GST_LIBYUVSCALER_CAST (btrans);
  GstCaps *tmp, *tmp2;
  GstCaps *result;

  /* Get all possible caps that we can transform to */
  tmp = gst_libyuvscaler_caps_remove_format_info (caps);

  if (filter) {
    tmp2 = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
    tmp = tmp2;
  }

  result = tmp;

  GST_DEBUG_OBJECT (btrans, "transformed %" GST_PTR_FORMAT " into %"
      GST_PTR_FORMAT, caps, result);

  return result;
}
#endif

static GstCaps *
gst_libyuvscaler_transform_caps (GstBaseTransform *btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps *filter)
{
  Gstlibyuvscaler *scaler = GST_LIBYUVSCALER_CAST (btrans);
  //GstVideoScaleMethod method;
  GstCaps *ret; //, *mfilter;
  GstStructure *structure;
  gint i, n;

  GST_DEBUG_OBJECT (btrans,
      "Transforming caps %" GST_PTR_FORMAT " in direction %s", caps,
      (direction == GST_PAD_SINK) ? "sink" : "src");

/*
  GST_OBJECT_LOCK (videoscale);
  method = videoscale->method;
  GST_OBJECT_UNLOCK (videoscale);
*/

  /* filter the supported formats */
/*
  if ((mfilter = get_formats_filter (method))) {
    caps = gst_caps_intersect_full (caps, mfilter, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (mfilter);
  } else {
    gst_caps_ref (caps);
  }
*/
  gst_caps_ref (caps);

  ret = gst_caps_new_empty ();
  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    structure = gst_caps_get_structure (caps, i);

    /* If this is already expressed by the existing caps
     * skip this structure */
    if (i > 0 && gst_caps_is_subset_structure (ret, structure))
      continue;

    /* make copy */
    structure = gst_structure_copy (structure);
    gst_structure_set (structure,
        "width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
        "height", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);

    /* if pixel aspect ratio, make a range of it */
#if 0
    if (gst_structure_has_field (structure, "pixel-aspect-ratio")) {
      gst_structure_set (structure, "pixel-aspect-ratio",
          GST_TYPE_FRACTION_RANGE, 1, G_MAXINT, G_MAXINT, 1, NULL);
    }
#endif
    gst_caps_append_structure (ret, structure);
  }

  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, ret, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (ret);
    ret = intersection;
  }

  gst_caps_unref (caps);
  GST_DEBUG_OBJECT (btrans, "returning caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

static GstCaps *
gst_libyuvscaler_fixate_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * othercaps)
{
  GstCaps *result;

  GST_DEBUG_OBJECT (trans, "fixating caps %" GST_PTR_FORMAT, othercaps);

  result = gst_caps_intersect (othercaps, caps);
  if (gst_caps_is_empty (result)) {
   gst_caps_unref (result);
   result = othercaps;
  } else {
   gst_caps_unref (othercaps);
  }

  /* fixate remaining fields */
  result = gst_caps_fixate (result);

  return result;
}

static gboolean
gst_libyuvscaler_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params)
{
  /* propose all metadata upstream */
  return TRUE;
}

static gboolean
gst_libyuvscaler_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf)
{
  const GstMetaInfo *info = meta->info;
  gboolean ret;

  if (gst_meta_api_type_has_tag (info->api, _colorspace_quark)) {
    /* don't copy colorspace specific metadata, FIXME, we need a MetaTransform
     * for the colorspace metadata. */
    ret = FALSE;
  } else {
    /* copy other metadata */
    ret = TRUE;
  }
  return ret;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */

static gboolean
plugin_init (GstPlugin * plugin)
{
  /* initialize gst controller library */
  GST_DEBUG_CATEGORY_INIT (gst_libyuvscaler_debug, "libyuvscaler", 0, "Scales I420 frames using libyuv");

  return gst_element_register (plugin, "libyuvscaler", GST_RANK_NONE,
      GST_TYPE_LIBYUVSCALER);
}


/* gstreamer looks for this structure to register libyuvscalers
 *
 * exchange the string 'Template libyuvscaler' with your libyuvscaler description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    libyuvscaler,
    "Scales I420 frames using libyuv",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
