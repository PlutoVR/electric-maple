// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

#include <gst/gst.h>
#include <gio/gio.h>

#define GST_G_IO_MODULE_DECLARE(name) \
extern void G_PASTE(g_io_, G_PASTE(name, _load)) (gpointer data)

#define GST_G_IO_MODULE_LOAD(name) \
G_PASTE(g_io_, G_PASTE(name, _load)) (NULL)

/* Declaration of static plugins */

GST_PLUGIN_STATIC_DECLARE(app);        // Definitely needed
GST_PLUGIN_STATIC_DECLARE(autodetect); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(nice);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(sctp);
GST_PLUGIN_STATIC_DECLARE(srtp);
GST_PLUGIN_STATIC_DECLARE(dtls);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(webrtc);
GST_PLUGIN_STATIC_DECLARE(androidmedia);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(videotestsrc); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(overlaycomposition);
GST_PLUGIN_STATIC_DECLARE(playback); // "FFMPEG "
/* Declaration of static gio modules */

/* Call this function to load GIO modules */
static void
gst_android_load_gio_modules (void)
{
  GTlsBackend *backend;
  const gchar *ca_certs;



  ca_certs = g_getenv ("CA_CERTIFICATES");

  backend = g_tls_backend_get_default ();
  if (backend && ca_certs) {
    GTlsDatabase *db;
    GError *error = NULL;

    db = g_tls_file_database_new (ca_certs, &error);
    if (db) {
      g_tls_backend_set_default_database (backend, db);
      g_object_unref (db);
    } else {
      g_warning ("Failed to create a database from file: %s",
          error ? error->message : "Unknown");
    }
  }
}

/* This is called by gst_init() */
void
gst_init_static_plugins (void)
{
  GST_PLUGIN_STATIC_REGISTER(app);        // Definitely needed
  GST_PLUGIN_STATIC_REGISTER(autodetect); // Definitely needed
  GST_PLUGIN_STATIC_REGISTER(coreelements);
  GST_PLUGIN_STATIC_REGISTER(nice);
  GST_PLUGIN_STATIC_REGISTER(rtp);
  GST_PLUGIN_STATIC_REGISTER(rtpmanager);
  GST_PLUGIN_STATIC_REGISTER(sctp);
  GST_PLUGIN_STATIC_REGISTER(srtp);
  GST_PLUGIN_STATIC_REGISTER(dtls);
  GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
  GST_PLUGIN_STATIC_REGISTER(webrtc);
  GST_PLUGIN_STATIC_REGISTER(androidmedia);
  GST_PLUGIN_STATIC_REGISTER(opengl);
  GST_PLUGIN_STATIC_REGISTER(videotestsrc); // Definitely needed
  GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
  GST_PLUGIN_STATIC_REGISTER(overlaycomposition);
  GST_PLUGIN_STATIC_REGISTER(playback);
  gst_android_load_gio_modules ();
}
