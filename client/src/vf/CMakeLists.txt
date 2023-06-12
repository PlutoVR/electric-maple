add_library(pluto_vf STATIC vf_driver.c)
target_link_libraries(pluto_vf PRIVATE xrt-interfaces aux_os aux_util
        ${GST_LIBRARIES}
        #-Wl,--whole-archive ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a -Wl,--no-whole-archive
        #${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.so
#${ART_LIBRARIES}
#${ADBD_LIBRARIES}
#${ANDROID_RUNTIME_LIBRARIES}
#${OS_STATSD_LIBRARIES}
#${ANDROID_SYSTEM_LIB64_LIBRARIES}
# ${GST_SDP_LIBRARIES}
# ${GST_WEBRTC_LIBRARIES}
# ${GLIB_LIBRARIES}
# ${LIBSOUP_LIBRARIES}
${JSONGLIB_LIBRARIES}
# ${GIO_LIBRARIES}
)
target_include_directories(pluto_vf PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/../monado/src/xrt/auxiliary/gstreamer
${GST_INCLUDE_DIRS}
${GLIB_INCLUDE_DIRS}
${LIBSOUP_INCLUDE_DIRS}
${JSONGLIB_INCLUDE_DIRS}
${GIO_INCLUDE_DIRS}
)
message(${GST_LIBRARIES})
message(${ART_LIBRARIES})
