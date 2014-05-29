/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2014 David C. Chen <david@remotium.com>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include "gstrtcpsender.h"

#define RTCP_MTU_SIZE 1200


/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

/* Filter signals and args */
enum
{
  /* FILL ME */
  SIGNAL_SEND_RTCP,
  LAST_SIGNAL
};

enum
{
  PROP_0
};

GST_DEBUG_CATEGORY_STATIC (gst_rtcpsender_debug);
#define GST_CAT_DEFAULT gst_rtcpsender_debug

#define gst_rtcpsender_parent_class parent_class
G_DEFINE_TYPE (GstRtcpSender, gst_rtcpsender, GST_TYPE_ELEMENT);

static GstFlowReturn gst_rtcpsender_src_event (GstPad * pad, GstObject * parent, GstEvent *event);
static GstFlowReturn gst_rtcpsender_send_rtcp (GstRtcpSender * rtcpsender, guint32 ssrc);

static guint gst_rtcpsender_signals[LAST_SIGNAL] = { 0 };

/* initialize the rtcpsender's class */
static void
gst_rtcpsender_class_init (GstRtcpSenderClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class     = (GObjectClass *) klass;
  gstelement_class  = (GstElementClass *) klass;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));

  gst_rtcpsender_signals[SIGNAL_SEND_RTCP] =
      g_signal_new ("send-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtcpSenderClass,
          send_rtcp), NULL, NULL, g_cclosure_marshal_VOID__UINT,
      GST_TYPE_FLOW_RETURN, 1, G_TYPE_UINT);

  gst_element_class_set_static_metadata (gstelement_class, "Rtcp Sender",
    "Network/RTCP",
    "Send custom RTCP packets when triggered",
    "David Chen <david@remotium.com>");

  klass->send_rtcp = gst_rtcpsender_send_rtcp;

  GST_DEBUG_CATEGORY_INIT (gst_rtcpsender_debug, "rtcpsender", 0, "RTCP Sender");
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_rtcpsender_init (GstRtcpSender * filter)
{
  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);

  gst_pad_set_event_function(filter->srcpad, gst_rtcpsender_src_event);

  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->started = FALSE;
}

static GstFlowReturn
gst_rtcpsender_src_event (GstPad * pad, GstObject * parent, GstEvent *event)
{

  return gst_pad_event_default (pad, parent, event);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_rtcpsender_send_rtcp (GstRtcpSender * rtcpsender, guint32 ssrc)
{
  gchar* stream_id;
  GstSegment* segment;
  GstBuffer *rtcpbuf;
  GstRTCPPacket packet;
  GstRTCPBuffer rtcp = GST_RTCP_BUFFER_INIT;

  if (!rtcpsender->started) {
    GST_DEBUG ("Need to send stream start event");

    stream_id = gst_pad_create_stream_id (rtcpsender->srcpad, &rtcpsender->parent, NULL);
    gst_pad_push_event (rtcpsender->srcpad, gst_event_new_stream_start(stream_id));

    gst_pad_use_fixed_caps (rtcpsender->srcpad);
    gst_pad_set_active(rtcpsender->srcpad, TRUE);
    gst_pad_set_caps (rtcpsender->srcpad, gst_caps_from_string ("application/x-rtcp"));

    segment = gst_segment_new();
    gst_segment_init(segment, GST_FORMAT_TIME);
    gst_pad_push_event (rtcpsender->srcpad, gst_event_new_segment(segment));

    rtcpsender->started = TRUE;
  }

  GST_DEBUG ("Got signal action. Preparing RTCP packet");

  rtcpbuf = gst_rtcp_buffer_new (RTCP_MTU_SIZE);
  gst_rtcp_buffer_map(rtcpbuf, GST_MAP_READWRITE, &rtcp);

  gst_rtcp_buffer_get_first_packet (&rtcp, &packet);

  gst_rtcp_buffer_add_packet (&rtcp, GST_RTCP_TYPE_RR, &packet);
  gst_rtcp_packet_rr_set_ssrc(&packet, ssrc);

  gst_rtcp_buffer_unmap(&rtcp);

  return gst_pad_push (rtcpsender->srcpad, rtcpbuf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  /* debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT (gst_rtcpsender_debug, "rtcpsender", 0, "RTCP Sender");

  if (!gst_element_register (plugin, "rtcpsender", GST_RANK_NONE, GST_TYPE_RTCPSENDER))
    return FALSE;

  return TRUE;
}

/* gstreamer looks for this structure to register rtcpsenders
 *
 * exchange the string 'Template rtcpsender' with your rtcpsender description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rtcpsender,
    "RTCP Sender",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
