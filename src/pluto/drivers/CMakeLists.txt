add_library(drv_pluto STATIC pl_driver.c pl_driver.h)

target_link_libraries(drv_pluto PRIVATE xrt-interfaces aux_util)
