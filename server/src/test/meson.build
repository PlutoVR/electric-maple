subdir('resources')

sources = [
  'mss-http-server.c',
  'main.c',
  gresources
]

executable('multistream-server', sources,
  dependencies: [soup_dep, json_glib_dep, gst_dep, gst_sdp_dep, gstwebrtc_dep],
  install: true,
  c_args: '-DG_LOG_DOMAIN="ibcdemo"')

executable('webrtc-client', ['webrtc-client.c'],
  dependencies: [soup_dep, json_glib_dep, gst_dep, gst_sdp_dep, gstwebrtc_dep],
  install: true,
  c_args: '-DG_LOG_DOMAIN="ibcdemo"')
