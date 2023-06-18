add_library(pluto_vf STATIC gst_driver.c)
target_link_libraries(pluto_vf PRIVATE xrt-interfaces aux_os aux_util
        ${GST_LIBRARIES}
        ${JSONGLIB_LIBRARIES}
        OpenXR::openxr_loader
        openxr_loader
        pluto_proto
)

target_include_directories(pluto_vf PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
        ${CMAKE_CURRENT_SOURCE_DIR}/../monado/src/xrt/auxiliary/gstreamer
        ${GST_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${LIBSOUP_INCLUDE_DIRS}
        ${JSONGLIB_INCLUDE_DIRS}
        ${GIO_INCLUDE_DIRS}
)

message(${GST_LIBRARIES})
