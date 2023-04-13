# subdir('resources')

# sources = [
#   'mss-http-server.c',
#   'main.c',
#   gresources
# ]

# executable('multistream-server', sources,
#   dependencies: [soup_dep, json_glib_dep, gst_dep, gst_sdp_dep, gstwebrtc_dep],
#   install: true,
#   c_args: '-DG_LOG_DOMAIN="ibcdemo"')

add_subdirectory(resources)


add_library(pl_gst STATIC gst_webrtc_main.c mss-http-server.c)


target_link_libraries(pl_gst PRIVATE aux_util aux_gstreamer ${GST_LIBRARIES} ${GST_SDP_LIBRARIES} ${GST_WEBRTC_LIBRARIES} ${GLIB_LIBRARIES} ${LIBSOUP_LIBRARIES} ${JSONGLIB_LIBRARIES} ${GIO_LIBRARIES} gresource-multistream-server)

# target_include_directories(pl_gst PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/)

target_include_directories(pl_gst PRIVATE ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${LIBSOUP_INCLUDE_DIRS} ${JSONGLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})