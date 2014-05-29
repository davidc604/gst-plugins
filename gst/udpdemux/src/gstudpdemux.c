/*
 * UDP Demux Element
 *
 * Copyright (C) 2014 Remotium Inc.
 * @author David Chen <david@remotium.com>
 *
 * Loosely based on GStreamer gstrtpptdemux
 * Copyright (C) <2004> Wim Taymans <wim.taymans@gmail.com>
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

#define VERSION_MAJOR 0
#define VERSION_MINOR 2

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <gst/gst.h>
#include "gstudpdemux.h"

/* generic templates */
static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_control_template =
GST_STATIC_PAD_TEMPLATE ("src_0",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_video_template =
GST_STATIC_PAD_TEMPLATE ("src_1",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_audio_template =
GST_STATIC_PAD_TEMPLATE ("src_2",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_udpdemux_debug);
#define GST_CAT_DEFAULT gst_udpdemux_debug

#define gst_udpdemux_parent_class parent_class
G_DEFINE_TYPE (GstUdpDemux, gst_udpdemux, GST_TYPE_ELEMENT);

static void gst_udpdemux_finalize (GObject * object);

static void gst_udpdemux_release (GstUdpDemux * udpdemux);
static gboolean gst_udpdemux_setup (GstUdpDemux * udpdemux);

static gboolean gst_udpdemux_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_udpdemux_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_udpdemux_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf);
static GstStateChangeReturn gst_udpdemux_change_state (GstElement * element,
    GstStateChange transition);

static void gst_udpdemux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_udpdemux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

enum
{
  PROP_0,
  PROP_CAPS_CONTROL,
  PROP_CAPS_VIDEO,
  PROP_CAPS_AUDIO,
};

static void
gst_udpdemux_class_init (GstUdpDemuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gstelement_class = GST_ELEMENT_CLASS(klass);

  gobject_class->set_property = gst_udpdemux_set_property;
  gobject_class->get_property = gst_udpdemux_get_property;
  gobject_class->finalize = gst_udpdemux_finalize;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_udpdemux_change_state);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_control_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_video_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_audio_template));


  gst_element_class_set_static_metadata (gstelement_class, "UDP Demux",
      "Demux/Network/UDP",
      "Demux UDP packets according to data type",
      "David Chen");

  g_object_class_install_property (gobject_class, PROP_CAPS_CONTROL,
      g_param_spec_boxed ("caps-control", "Control Filter caps",
          "Filter caps for control src", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAPS_VIDEO,
      g_param_spec_boxed ("caps-video", "Video Filter caps",
          "Filter caps for video src", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAPS_AUDIO,
      g_param_spec_boxed ("caps-audio", "Audio Filter caps",
          "Filter caps for audio control src", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (gst_udpdemux_debug, "udpdemux", 0, "UDP demuxer");
}

static void
gst_udpdemux_init (GstUdpDemux * udpdemux)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (udpdemux);

  udpdemux->sink =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  g_assert (udpdemux->sink != NULL);

  gst_pad_set_chain_function (udpdemux->sink, gst_udpdemux_chain);
  gst_pad_set_event_function (udpdemux->sink, gst_udpdemux_sink_event);

  gst_element_add_pad (GST_ELEMENT (udpdemux), udpdemux->sink);

  udpdemux->ctrlpad = gst_pad_new_from_static_template(&src_control_template, "src_0");
  udpdemux->videopad = gst_pad_new_from_static_template(&src_video_template, "src_1");
  udpdemux->audiopad = gst_pad_new_from_static_template(&src_audio_template, "src_2");

  gst_element_add_pad (GST_ELEMENT (udpdemux), udpdemux->ctrlpad);
  gst_element_add_pad (GST_ELEMENT (udpdemux), udpdemux->videopad);
  gst_element_add_pad (GST_ELEMENT (udpdemux), udpdemux->audiopad);
}

static void
gst_udpdemux_finalize (GObject * object)
{
  gst_udpdemux_release (GST_UDPDEMUX (object));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GST_DEBUG ("Forwarding sticky events");
  gboolean res = TRUE;
  GstPad *srcpad = GST_PAD_CAST (user_data);

  if (!gst_pad_has_current_caps (srcpad)) {
    GST_DEBUG ("No pads with caps available. Dropping event");
    gst_event_unref (*event);
    *event = NULL;
    res = TRUE;
  }

  /* Stream start and caps have already been pushed */
  if (GST_EVENT_TYPE (*event) >= GST_EVENT_SEGMENT)
    res = gst_pad_push_event (srcpad, gst_event_ref (*event));

  return res;
}

static GstPad*
find_pad_for_data_type (GstUdpDemux * udpdemux, guint8 type)
{
  GstPad* rpad = NULL;
  GSList* parse;

  GST_OBJECT_LOCK(udpdemux);
  for (parse = udpdemux->ls_srcpads; parse; parse = g_slist_next (parse)) {
    GstUdpDemuxPad *pad = parse->data;

    if (pad->data_type == type) {
      GST_DEBUG_OBJECT (udpdemux, "found pad for buffer type: %d", type);
      rpad = gst_object_ref (pad->pad);
      break;
    }
  }
  GST_OBJECT_UNLOCK(udpdemux);

  return rpad;
}

static GstFlowReturn
gst_udpdemux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstUdpDemux *udpdemux;
  GstMapInfo info;
  GstPad *srcpad;
  GstCaps *caps;
  gint8 type;
  gsize offset;
  gssize res;

  udpdemux = GST_UDPDEMUX (parent);

  // get type of data
  gst_buffer_extract(buf, 0, &type, sizeof(gint8));

  GST_DEBUG_OBJECT (udpdemux, "received buffer type %d", type);

  srcpad = find_pad_for_data_type (udpdemux, type);
  if (NULL == srcpad) {
    GstUdpDemuxPad *udpdemuxpad;

    /* add new srcpad to list */
    if (type == TYPE_CONTROL)
      srcpad = udpdemux->ctrlpad;
    else if (type == TYPE_VIDEO)
      srcpad = udpdemux->videopad;
    else if (type == TYPE_AUDIO)
      srcpad = udpdemux->audiopad;
    else {
      GST_DEBUG ("Error unsupported type: %d", type);
      return GST_FLOW_ERROR;
    }

    gst_pad_use_fixed_caps (srcpad);
    gst_pad_set_active (srcpad, TRUE);

    gst_pad_push_event (srcpad,
        gst_pad_get_sticky_event (udpdemux->sink, GST_EVENT_STREAM_START, 0));

    if (type == TYPE_CONTROL)
      gst_pad_set_caps (srcpad, udpdemux->caps_control);
    else if (type == TYPE_VIDEO)
      gst_pad_set_caps (srcpad, udpdemux->caps_video);
    else if (type == TYPE_AUDIO)
      gst_pad_set_caps (srcpad, udpdemux->caps_audio);

    gst_pad_set_event_function(srcpad, gst_udpdemux_src_event);

    gst_pad_sticky_events_foreach (udpdemux->sink, forward_sticky_events,
        srcpad);

    GST_DEBUG ("Adding type=%d to the list.", type);
    udpdemuxpad = g_slice_new0 (GstUdpDemuxPad);
    udpdemuxpad->data_type = type;
    udpdemuxpad->pad = srcpad;

    gst_object_ref (srcpad);
    GST_OBJECT_LOCK (udpdemux);
    udpdemux->ls_srcpads = g_slist_append (udpdemux->ls_srcpads, udpdemuxpad);
    GST_OBJECT_UNLOCK(udpdemux);
  }

  gst_buffer_map (buf, &info, GST_MAP_WRITE);
  offset = sizeof (gint8);
  res = info.size - offset;

  gst_buffer_unmap (buf, &info);
  gst_buffer_resize (buf, offset, res);

  /* push to srcpad */
  ret = gst_pad_push (srcpad, buf);

  gst_object_unref (srcpad);

  return ret;
}

static gboolean
gst_udpdemux_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstUdpDemux *udpdemux;
  udpdemux = GST_UDPDEMUX (parent);

  return gst_pad_event_default (pad, parent, event);
}


static gboolean
gst_udpdemux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstUdpDemux *udpdemux;
  gboolean res = FALSE;
  udpdemux = GST_UDPDEMUX (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      GST_DEBUG ("Got SEGMENT event");
      if (gst_pad_has_current_caps (udpdemux->ctrlpad)) {
        GST_DEBUG ("Control Pad Caps exists. Pushing event to Control Pad");
        gst_event_ref (event);
        res |= gst_pad_push_event (udpdemux->ctrlpad, event);
      }

      if (gst_pad_has_current_caps(udpdemux->videopad)) {
        GST_DEBUG ("Video Pad Caps exists. Pushing event to Video Pad");
        gst_event_ref (event);
        res |= gst_pad_push_event (udpdemux->videopad, event);
      }

      if (gst_pad_has_current_caps (udpdemux->audiopad)) {
        GST_DEBUG ("Audio Pad Caps exists. Pushing event to Audio Pad");
        gst_event_ref (event);
        res |= gst_pad_push_event (udpdemux->audiopad, event);
      }
      else {
        GST_DEBUG ("No Pads with Caps exists. Dropping event");
        gst_event_unref (event);
        res = TRUE;
      }
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}


static void
gst_udpdemux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstUdpDemux *udpdemux = GST_UDPDEMUX (object);

  switch (prop_id) {
    case PROP_CAPS_CONTROL:
    {
      GST_OBJECT_LOCK (udpdemux);
      if (udpdemux->caps_control)
        gst_caps_unref (udpdemux->caps_control);
      udpdemux->caps_control = gst_caps_copy (gst_value_get_caps (value));
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    }
    case PROP_CAPS_VIDEO:
    {
      GST_OBJECT_LOCK (udpdemux);
      if (udpdemux->caps_video)
        gst_caps_unref (udpdemux->caps_video);
      udpdemux->caps_video = gst_caps_copy (gst_value_get_caps (value));
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    }
    case PROP_CAPS_AUDIO:
    {
      GST_OBJECT_LOCK (udpdemux);
      if (udpdemux->caps_audio)
        gst_caps_unref (udpdemux->caps_audio);
      udpdemux->caps_audio = gst_caps_copy (gst_value_get_caps (value));
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_udpdemux_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstUdpDemux *udpdemux = GST_UDPDEMUX (object);

  switch (prop_id) {
    case PROP_CAPS_CONTROL:
      GST_OBJECT_LOCK (udpdemux);
      gst_value_set_caps (value, udpdemux->caps_control);
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    case PROP_CAPS_VIDEO:
      GST_OBJECT_LOCK (udpdemux);
      gst_value_set_caps (value, udpdemux->caps_video);
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    case PROP_CAPS_AUDIO:
      GST_OBJECT_LOCK (udpdemux);
      gst_value_set_caps (value, udpdemux->caps_audio);
      GST_OBJECT_UNLOCK (udpdemux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


/*
 * Free resources for the object.
 */
static void
gst_udpdemux_release (GstUdpDemux * udpdemux)
{
  GSList *tmp;
  GSList *parse;

  GST_OBJECT_LOCK (udpdemux);
  tmp = udpdemux->ls_srcpads;
  udpdemux->ls_srcpads = NULL;
  GST_OBJECT_UNLOCK (udpdemux);

  for (parse = tmp; parse; parse = g_slist_next (parse)) {
    GstUdpDemuxPad *pad = parse->data;

    gst_pad_set_active (pad->pad, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (udpdemux), pad->pad);
    g_slice_free (GstUdpDemuxPad, pad);
  }
  g_slist_free (tmp);
}

static GstStateChangeReturn
gst_udpdemux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstUdpDemux *udpdemux;

  udpdemux = GST_UDPDEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_udpdemux_release (udpdemux);
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_udpdemux_debug, "udpdemux", 0, "UDP demuxer");

  if (!gst_element_register (plugin, "udpdemux", GST_RANK_NONE, GST_TYPE_UDPDEMUX))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (
    VERSION_MAJOR,
    VERSION_MINOR,
    udpdemux,
    "UDP packet demux",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
