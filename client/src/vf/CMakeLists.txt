add_library(pluto_vf STATIC vf_driver.c)
target_link_libraries(pluto_vf PRIVATE xrt-interfaces aux_os aux_util ${GST_LIBRARIES} 		

# ${GST_SDP_LIBRARIES}
# ${GST_WEBRTC_LIBRARIES}
# ${GLIB_LIBRARIES}
# ${LIBSOUP_LIBRARIES}
${JSONGLIB_LIBRARIES}
# ${GIO_LIBRARIES}
)
target_include_directories(pluto_vf PRIVATE 
${GST_INCLUDE_DIRS}
 ${GLIB_INCLUDE_DIRS}
  		${LIBSOUP_INCLUDE_DIRS}
${JSONGLIB_INCLUDE_DIRS}
${GIO_INCLUDE_DIRS}
)

message(${GST_LIBRARIES})
