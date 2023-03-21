add_library(
	drv_pluto STATIC pluto_streaming_driver/driver_pluto_streaming.c
			 pluto_streaming_driver/driver_pluto_streaming.h
	)

target_link_libraries(drv_pluto PRIVATE xrt-interfaces aux_util)
