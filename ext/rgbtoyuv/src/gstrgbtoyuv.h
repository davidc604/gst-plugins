/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2013 David Chen <david@remotium.com>
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

#ifndef __GST_RGBTOYUV_H__
#define __GST_RGBTOYUV_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>


G_BEGIN_DECLS

#define GST_TYPE_RGBTOYUV 				(gst_rgb_to_yuv_get_type())
#define GST_RGBTOYUV(obj) 				(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RGBTOYUV,GstRgbToYuv))
#define GST_RGBTOYUV_CLASS(klass) 		(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RGBTOYUV,GstRgbToYuvClass))
#define GST_IS_RGBTOYUV(obj) 			(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RGBTOYUV))
#define GST_IS_RGBTOYUV_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RGBTOYUV))
#define GST_RGBTOYUV_CAST(obj)			((GstRgbToYuv *)(obj))


typedef struct _GstRgbToYuv      GstRgbToYuv;
typedef struct _GstRgbToYuvClass GstRgbToYuvClass;

struct _GstRgbToYuv {
  GstVideoFilter element;
};

struct _GstRgbToYuvClass {
  GstVideoFilterClass parent_class;
};


G_END_DECLS

#endif /* __GST_RGBTOYUV_H__ */
