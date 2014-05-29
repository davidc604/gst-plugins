/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2013  <<user@hostname.org>>
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
 * SECTION:element-yuvtorgb
 *
 * Converts I420 to RGB using open source libyuv
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! yuvtorgb ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstyuvtorgb.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include <string.h>

/* include libyuv */
#include "libyuv.h"


GST_DEBUG_CATEGORY_STATIC (gst_yuv_to_rgb_debug);
#define GST_CAT_DEFAULT gst_yuv_to_rgb_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_PERFORMANCE);

GType gst_yuv_to_rgb_get_type (void);

static GQuark _colorspace_quark;

#define gst_yuv_to_rgb_parent_class parent_class
G_DEFINE_TYPE (GstYuvToRgb, gst_yuv_to_rgb, GST_TYPE_VIDEO_FILTER);

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

/* the capabilities of the inputs and outputs.
 */

#define SINK_CAPS_STR GST_VIDEO_CAPS_MAKE ("I420")
#define SRC_CAPS_STR GST_VIDEO_CAPS_MAKE ("ARGB")

static GstStaticPadTemplate gst_yuvtorgb_sink_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (SINK_CAPS_STR)
);

static GstStaticPadTemplate gst_yuvtorgb_src_template =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (SRC_CAPS_STR)
);


static void gst_yuv_to_rgb_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_yuv_to_rgb_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GObject vmethod implementations */
static GstCaps * gst_yuv_to_rgb_transform_caps (GstBaseTransform * btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);

static GstCaps * gst_yuv_to_rgb_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);

static gboolean gst_yuv_to_rgb_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params);

static gboolean gst_yuv_to_rgb_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf);

static gboolean gst_yuv_to_rgb_set_info (GstVideoFilter * filter,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info);

static GstFlowReturn gst_yuv_to_rgb_transform_frame (GstVideoFilter *filter,
  GstVideoFrame *in_frame, GstVideoFrame *out_frame);


/* initialize the yuvtorgb's class */
static void
gst_yuv_to_rgb_class_init (GstYuvToRgbClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *gstbasetransform_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *gstvideofilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_yuv_to_rgb_set_property;
  gobject_class->get_property = gst_yuv_to_rgb_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_yuvtorgb_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_yuvtorgb_sink_template));

  gst_element_class_set_static_metadata (gstelement_class, "I420 To RGB16/RGB32",
    "Filter/Converter/Video",
    "Converts I420 to RGB16/RGB32 using libyuv",
    "David Chen <david@remotium.com>");

  gstbasetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_transform_caps);
  gstbasetransform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_fixate_caps);
  gstbasetransform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_filter_meta);
  gstbasetransform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_transform_meta);

  gstbasetransform_class->passthrough_on_same_caps = TRUE;

  gstvideofilter_class->set_info =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_set_info);
  gstvideofilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_yuv_to_rgb_transform_frame);


  /* debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT (gst_yuv_to_rgb_debug, "yuvtorgb", 0, "Converts I420 to RGB16/RGB32 using libyuv");
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_yuv_to_rgb_init (GstYuvToRgb *filter)
{
}

static void
gst_yuv_to_rgb_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
}

static void
gst_yuv_to_rgb_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
}

/* GstBaseTransform vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn
gst_yuv_to_rgb_transform_frame (GstVideoFilter *filter, GstVideoFrame *in_frame, GstVideoFrame *out_frame)
{
  GstYuvToRgb *rgbtoyuv = GST_YUVTORGB_CAST (filter);
  gint width, height, stride;
  gint y_stride, uv_stride;
  guint32 *out_data;
  guint8 *y_in, *u_in, *v_in;

  y_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
  uv_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);

  y_in = (guint8*) GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  u_in = (guint8*) GST_VIDEO_FRAME_PLANE_DATA (in_frame, 1);
  v_in = (guint8*) GST_VIDEO_FRAME_PLANE_DATA (in_frame, 2);

  width = GST_VIDEO_FRAME_WIDTH (out_frame);
  height = GST_VIDEO_FRAME_HEIGHT (out_frame);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);

  out_data = (guint32*) GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  // GST_INFO ("DEBUG_INFO: rgbtoyuv::transform_frame: ");
  // GST_INFO ("in stride: %d; out stride: %d %d\n", stride, y_stride, uv_stride);

  libyuv::I420ToARGB (y_in, y_stride,
              u_in, uv_stride,
              v_in, uv_stride,
              (guint8*)out_data, stride,
              width, height);

  return GST_FLOW_OK;
}

static gboolean
gst_yuv_to_rgb_set_info (GstVideoFilter * filter,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info)
{
  GstYuvToRgb *yuvtorgb = GST_YUVTORGB_CAST (filter);

  if (in_info->width != out_info->width || in_info->height != out_info->height
      || in_info->fps_n != out_info->fps_n || in_info->fps_d != out_info->fps_d)
    goto format_mismatch;

  /* if present, these must match too */
  if (in_info->par_n != out_info->par_n || in_info->par_d != out_info->par_d)
    goto format_mismatch;

  /* if present, these must match too */
  if (in_info->interlace_mode != out_info->interlace_mode)
    goto format_mismatch;

  GST_DEBUG ("reconfigured %d %d", GST_VIDEO_INFO_FORMAT (in_info),
      GST_VIDEO_INFO_FORMAT (out_info));

  return TRUE;

    /* ERRORS */
format_mismatch:
  {
    GST_ERROR_OBJECT (yuvtorgb, "input and output formats do not match");
    return FALSE;
  }
}

/* copies the given caps */
static GstCaps *
gst_yuv_to_rgb_caps_remove_format_info (GstCaps * caps)
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

/* The caps can be transformed into any other caps with format info removed.
 * However, we should prefer passthrough, so if passthrough is possible,
 * put it first in the list. */
static GstCaps *
gst_yuv_to_rgb_transform_caps (GstBaseTransform * btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstYuvToRgb *yuvtorgb = GST_YUVTORGB_CAST (btrans);
  GstCaps *tmp, *tmp2;
  GstCaps *result;

  /* Get all possible caps that we can transform to */
  tmp = gst_yuv_to_rgb_caps_remove_format_info (caps);

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

static GstCaps *
gst_yuv_to_rgb_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps)
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
gst_yuv_to_rgb_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params)
{
  /* propose all metadata upstream */
  return TRUE;
}

static gboolean
gst_yuv_to_rgb_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
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
 GST_DEBUG_CATEGORY_INIT (gst_yuv_to_rgb_debug, "rgbtoyuv", 0, "Converts I420 to ARGB using libyuv");

 return gst_element_register (plugin, "yuvtorgb", GST_RANK_NONE,
     GST_TYPE_YUVTORGB);
}

/* gstreamer looks for this structure to register yuvtorgbs
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    yuvtorgb,
    "Converts I420 to ARGB using libyuv",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
