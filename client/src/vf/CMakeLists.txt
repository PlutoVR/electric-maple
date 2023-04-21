add_library(pluto_vf STATIC vf_driver.c)
target_link_libraries(pluto_vf PRIVATE xrt-interfaces aux_os aux_util ${GST_LIBRARIES})
target_include_directories(pluto_vf PRIVATE ${GST_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

