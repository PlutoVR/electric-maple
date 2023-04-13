#include "mss-http-server.h"

#include <glib-unix.h>
#include <gst/gst.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/rtcsessiondescription.h>

#define DEFAULT_SRT_URI "srt://:7001"
#define DEFAULT_RIST_ADDRESSES "224.0.0.1:5004"

#ifdef __aarch64__
#define DEFAULT_VIDEOSINK " queue max-size-bytes=0 ! kmssink bus-id=a0070000.v_mix"
#else
#define DEFAULT_VIDEOSINK " videoconvert ! autovideosink "
#endif

static gchar *srt_uri = NULL;
static gchar *rist_addresses = NULL;

static GOptionEntry options[] = {
    { "srt-uri", 'u', 0, G_OPTION_ARG_STRING, &srt_uri,
      "SRT stream URI. Default: " DEFAULT_SRT_URI, "srt://address:port" },
    { "rist-addresses", 'r', 0, G_OPTION_ARG_STRING, &rist_addresses,
      "Comma-separated list of addresses to send RIST packets to. Default: " DEFAULT_RIST_ADDRESSES, "address:port,address:port" },
    { NULL }
};

MssHttpServer *http_server;

static gboolean
sigint_handler (gpointer user_data)
{
  g_main_loop_quit (user_data);
  return G_SOURCE_REMOVE;
}

static gboolean
gst_bus_cb (GstBus * bus, GstMessage * message, gpointer data)
{
  GstBin *pipeline = GST_BIN (data);

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
    {
      GError *gerr;
      gchar *debug_msg;
      gst_message_parse_error (message, &gerr, &debug_msg);
      GST_DEBUG_BIN_TO_DOT_FILE (pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "mss-pipeline-ERROR");
      g_error ("Error: %s (%s)", gerr->message, debug_msg);
      g_error_free (gerr);
      g_free (debug_msg);
    }
    break;
  case GST_MESSAGE_WARNING:
    {
      GError *gerr;
      gchar *debug_msg;
      gst_message_parse_warning (message, &gerr, &debug_msg);
      GST_DEBUG_BIN_TO_DOT_FILE (pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "mss-pipeline-WARNING");
      g_warning ("Warning: %s (%s)", gerr->message, debug_msg);
      g_error_free (gerr);
      g_free (debug_msg);
    }
    break;
  case GST_MESSAGE_EOS:
    {
      g_error ("Got EOS!!");
    }
    break;
  default:
    break;
  }
  return TRUE;
}

static GstElement *
get_webrtcbin_for_client (GstBin *pipeline, MssClientId client_id)
{
  gchar *name;
  GstElement *webrtcbin;

  name = g_strdup_printf ("webrtcbin_%p", client_id);
  webrtcbin = gst_bin_get_by_name (pipeline, name);
  g_free (name);

  return webrtcbin;
}

static void
connect_webrtc_to_tee (GstElement *webrtcbin)
{
  GstElement *pipeline;
  GstElement *tee;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstPadLinkReturn ret;

  pipeline = GST_ELEMENT (gst_element_get_parent (webrtcbin));
  if (pipeline == NULL)
    return;
  tee = gst_bin_get_by_name (GST_BIN (pipeline), "webrtctee");
  srcpad = gst_element_request_pad_simple (tee, "src_%u");
  sinkpad = gst_element_request_pad_simple (webrtcbin, "sink_0");
  ret = gst_pad_link (srcpad, sinkpad);
  g_assert (ret == GST_PAD_LINK_OK);
  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);
  gst_object_unref (tee);
  gst_object_unref (pipeline);
}

static void
on_offer_created (GstPromise *promise, GstElement *webrtcbin)
{
  GstWebRTCSessionDescription *offer = NULL;
  gchar *sdp;

  gst_structure_get (gst_promise_get_reply (promise),
      "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
  gst_promise_unref (promise);

  g_signal_emit_by_name (webrtcbin, "set-local-description", offer, NULL);

  sdp = gst_sdp_message_as_text (offer->sdp);
  mss_http_server_send_sdp_offer (http_server,
      g_object_get_data (G_OBJECT (webrtcbin), "client_id"),
      sdp);
  g_free (sdp);

  gst_webrtc_session_description_free (offer);

  connect_webrtc_to_tee (webrtcbin);
}

static void
webrtc_on_ice_candidate_cb (GstElement *webrtcbin, guint mlineindex,
  gchar *candidate)
{
  mss_http_server_send_candidate (http_server,
      g_object_get_data (G_OBJECT (webrtcbin), "client_id"),
      mlineindex, candidate);
}

static void
webrtc_client_connected_cb (MssHttpServer *server, MssClientId client_id,
    GstBin *pipeline)
{
  gchar *name;
  GstElement *webrtcbin;
  GstCaps *caps;
  GstStateChangeReturn ret;
  GstWebRTCRTPTransceiver *transceiver;

  name = g_strdup_printf ("webrtcbin_%p", client_id);

  webrtcbin = gst_element_factory_make ("webrtcbin", name);
  g_object_set_data (G_OBJECT (webrtcbin), "client_id", client_id);
  gst_bin_add (pipeline, webrtcbin);
  ret = gst_element_set_state (webrtcbin, GST_STATE_PLAYING);
  g_assert (ret != GST_STATE_CHANGE_FAILURE);

  g_signal_connect (webrtcbin, "on-ice-candidate",
      G_CALLBACK (webrtc_on_ice_candidate_cb), NULL);

  caps = gst_caps_from_string ("application/x-rtp, payload=96,encoding-name=H264,clock-rate=90000,media=video,packetization-mode=(string)1,profile-level-id=(string)42e01f");
  g_signal_emit_by_name (webrtcbin, "add-transceiver",
      GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY,
      caps, &transceiver);

  gst_caps_unref (caps);
  gst_clear_object (&transceiver);

  g_signal_emit_by_name (webrtcbin, "create-offer", NULL,
      gst_promise_new_with_change_func (
          (GstPromiseChangeFunc) on_offer_created, webrtcbin,NULL));

  GST_DEBUG_BIN_TO_DOT_FILE (pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "rtcbin");

  g_free (name);
}

static void
webrtc_sdp_answer_cb (MssHttpServer *server, MssClientId client_id,
    const gchar *sdp, GstBin *pipeline)
{
  GstSDPMessage *sdp_msg = NULL;
  GstWebRTCSessionDescription *desc = NULL;

  if (gst_sdp_message_new_from_text (sdp, &sdp_msg) != GST_SDP_OK) {
    g_debug ("Error parsing SDP description");
    goto out;
  }

  desc = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER,
      sdp_msg);
  if (desc) {
    GstElement *webrtcbin;
    GstPromise *promise;

    webrtcbin = get_webrtcbin_for_client (pipeline, client_id);
    if (!webrtcbin) {
      goto out;
    }
    promise = gst_promise_new();

    g_signal_emit_by_name (webrtcbin, "set-remote-description", desc, promise);

    gst_promise_wait (promise);
    gst_promise_unref (promise);

    gst_object_unref (webrtcbin);
  } else {
    gst_sdp_message_free (sdp_msg);
  }

out:
  g_clear_pointer (&desc, gst_webrtc_session_description_free);
}

static void
webrtc_candidate_cb (MssHttpServer *server, MssClientId client_id,
    guint mlineindex, const gchar *candidate, GstBin *pipeline)
{
  if (strlen (candidate)) {
    GstElement *webrtcbin;

    webrtcbin = get_webrtcbin_for_client (pipeline, client_id);
    if (webrtcbin) {
      g_signal_emit_by_name (webrtcbin, "add-ice-candidate", mlineindex,
          candidate);
      gst_object_unref (webrtcbin);
    }
  }

  g_debug ("Remote candidate: %s", candidate);
}

static GstPadProbeReturn
remove_webrtcbin_probe_cb (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstElement *webrtcbin = GST_ELEMENT (user_data);

  gst_bin_remove (GST_BIN (GST_ELEMENT_PARENT (webrtcbin)), webrtcbin);
  gst_element_set_state (webrtcbin, GST_STATE_NULL);

  return GST_PAD_PROBE_REMOVE;
}

static void
webrtc_client_disconnected_cb (MssHttpServer *server, MssClientId client_id,
    GstBin *pipeline)
{
  GstElement *webrtcbin;

  webrtcbin = get_webrtcbin_for_client (pipeline, client_id);
  if (webrtcbin) {
    GstPad *sinkpad;

    if (sinkpad) {
      sinkpad = gst_element_get_static_pad (webrtcbin, "sink_0");

      gst_pad_add_probe (GST_PAD_PEER (sinkpad),
          GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
          remove_webrtcbin_probe_cb, webrtcbin, gst_object_unref);

      gst_clear_object (&sinkpad);
    }
  }
}

struct RestartData {
  GstElement *src;
  GstElement *pipeline;
};

static void
free_restart_data (gpointer user_data)
{
  struct RestartData *rd = user_data;

  gst_object_unref (rd->src);
  g_free (rd);
}

static gboolean
restart_source (gpointer user_data)
{
  struct RestartData *rd = user_data;
  GstElement *e;
  GstStateChangeReturn ret;

  gst_element_set_state (rd->src, GST_STATE_NULL);
  gst_element_set_locked_state (rd->src, TRUE);
  e = gst_bin_get_by_name (GST_BIN (rd->pipeline), "srtqueue");
  gst_bin_add (GST_BIN (rd->pipeline), rd->src);
  if (!gst_element_link (rd->src, e))
    g_assert_not_reached ();
  gst_element_set_locked_state (rd->src, FALSE);
  ret = gst_element_set_state (rd->src, GST_STATE_PLAYING);
  g_assert (ret != GST_STATE_CHANGE_FAILURE);
  gst_object_unref (e);

  g_debug ("Restarted source after EOS");

  return G_SOURCE_REMOVE;
}

static GstPadProbeReturn
src_event_cb (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
  GstElement *pipeline = user_data;
  GstElement *src;
  struct RestartData *rd;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_EVENT (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  src = gst_pad_get_parent_element (pad);

  gst_bin_remove (GST_BIN (pipeline), src);

  rd = g_new (struct RestartData, 1);
  rd->src = src;
  rd->pipeline = pipeline;
  g_idle_add_full (G_PRIORITY_HIGH_IDLE, restart_source, rd, free_restart_data);

  return GST_PAD_PROBE_DROP;
}

static gboolean
print_stats (gpointer user_data)
{
  GstElement *src = user_data;
  GstStructure *s;
  char *str;

  g_object_get (src, "stats", &s, NULL);
  str = gst_structure_to_string (s);
  //g_debug ("%s", str);
  g_free (str);
  gst_structure_free (s);

  return G_SOURCE_CONTINUE;
}

int main (int argc, char *argv[])
{
  GOptionContext *option_context;
  GMainLoop *loop;
  gchar *pipeline_str;
  GstElement *pipeline;
  GError *error = NULL;
  GstBus *bus;
  GstElement *src;
  GstPad *srcpad;
  GstStateChangeReturn ret;

  option_context = g_option_context_new (NULL);
  g_option_context_add_main_entries (option_context, options, NULL);

  if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    exit (1);
  }

  if (!srt_uri) {
    srt_uri = g_strdup(DEFAULT_SRT_URI);
  }
  if (!rist_addresses) {
    rist_addresses = g_strdup (DEFAULT_RIST_ADDRESSES);
  }

  http_server = mss_http_server_new ();

  pipeline_str = g_strdup_printf (
      "videotestsrc ! tee name=t "
      "t. ! queue ! rtpmp2tpay ! ristsink bonding-addresses=%s "
      "t. ! queue leaky=downstream max-size-buffers=400 ! srtsink uri=srt://:7002?mode=listener async=0 "
      "t. ! queue ! tsdemux latency=50 ! tee name=h264_t "
        "h264_t. ! queue ! decodebin ! videoconvert ! " DEFAULT_VIDEOSINK " "
        "h264_t. ! queue ! h264parse ! video/x-h264, alignment=au ! "
          "hlssink2 location=%s/segment%%05d.ts playlist-location=%s/playlist.m3u8 send-keyframe-requests=0 target-duration=1 playlist-length=5 "
        "h264_t. ! queue ! h264parse ! rtph264pay config-interval=1 ! application/x-rtp,payload=96 ! tee name=webrtctee allow-not-linked=true ",
      srt_uri,
      rist_addresses,
      mss_http_server_get_hls_dir (http_server),
      mss_http_server_get_hls_dir (http_server));

  gst_init (&argc, &argv);
  pipeline = gst_parse_launch (pipeline_str, &error);
  g_assert_no_error (error);
  g_free (pipeline_str);

  bus = gst_element_get_bus (pipeline);
  gst_bus_add_watch (bus, gst_bus_cb, pipeline);
  gst_object_unref (bus);

  g_signal_connect (http_server, "ws-client-disconnected",
      G_CALLBACK (webrtc_client_disconnected_cb), pipeline);
  g_signal_connect (http_server, "sdp-answer",
      G_CALLBACK (webrtc_sdp_answer_cb), pipeline);
  g_signal_connect (http_server, "candidate",
      G_CALLBACK (webrtc_candidate_cb), pipeline);

  loop = g_main_loop_new (NULL, FALSE);
  g_unix_signal_add (SIGINT, sigint_handler, loop);

  g_print ("Input SRT URI is %s\n"
      "\nOutput streams:\n"
      "\tHLS & WebRTC web player: http://localhost:8080\n"
      "\tRIST: %s\n"
      "\tSRT: srt://127.0.0.1:7002\n",
      srt_uri,
      rist_addresses);

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  srcpad = gst_element_get_static_pad (src, "src");
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, src_event_cb,
      pipeline, NULL);
  g_timeout_add (1000, print_stats, src);
  gst_object_unref (srcpad);
  gst_object_unref (src);

  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_assert (ret != GST_STATE_CHANGE_FAILURE);

  g_signal_connect (http_server, "ws-client-connected",
      G_CALLBACK (webrtc_client_connected_cb), pipeline);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_clear_object (&pipeline);
  g_clear_object (&http_server);
  g_clear_pointer (&srt_uri, g_free);
  g_clear_pointer (&rist_addresses, g_free);
}
