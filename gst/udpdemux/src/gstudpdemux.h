/*
 * UDP Demux Element
 *
 * Copyright (C) 2014 Remotium Inc.
 * @author David Chen <david@remotium.com>
 *
 * Loosely based on GStreamer gstrtpptdemux
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_UDPDEMUX_H__
#define __GST_UDPDEMUX_H__

#include <gst/gst.h>

#define GST_TYPE_UDPDEMUX            (gst_udpdemux_get_type())
#define GST_UDPDEMUX(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_UDPDEMUX,GstUdpDemux))
#define GST_UDPDEMUX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_UDPDEMUX,GstUdpDemuxClass))
#define GST_IS_UDPDEMUX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_UDPDEMUX))
#define GST_IS_UDPDEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_UDPDEMUX))

enum {
  TYPE_CONTROL = 0,
  TYPE_VIDEO,
  TYPE_AUDIO
};

typedef struct _GstUdpDemux GstUdpDemux;
typedef struct _GstUdpDemuxClass GstUdpDemuxClass;
typedef struct _GstUdpDemuxPad GstUdpDemuxPad;

/*
 * Item for storing GstPad<->data_type pairs.
 */
struct _GstUdpDemuxPad
{
  GstPad *pad;
  gint8 data_type;
};

struct _GstUdpDemux
{
  GstElement parent;  /**< parent class */

  GstPad *sink;           /* sink pad */
  GstPad* ctrlpad;        /* rtcp src pad */
  GstPad* videopad;       /* video src pad */
  GstPad* audiopad;       /* audio src pad */

  GstCaps* caps_control;
  GstCaps* caps_video;
  GstCaps* caps_audio;

  GSList *ls_srcpads;   /* linked list of GstUdpDemuxPad objects */
};

struct _GstUdpDemuxClass
{
  GstElementClass parent_class;
};

GType gst_udpdemux_get_type (void);

#endif /* __GST_UDPDEMUX_H__ */
