# Copyright 2023, Pluto VR, Inc.
# SPDX-License-Identifier: BSL-1.0

add_library(em_proto STATIC generated/electricmaple.pb.h generated/electricmaple.pb.c)

target_link_libraries(em_proto xrt-external-nanopb)


target_include_directories(em_proto INTERFACE generated)
