add_library(pluto_proto STATIC generated/pluto.pb.h generated/pluto.pb.c)

target_link_libraries(pluto_proto xrt-external-nanopb)


target_include_directories(pluto_proto INTERFACE generated)