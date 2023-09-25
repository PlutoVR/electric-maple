# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

add_executable(test_data_accumulator test_data_accumulator.cpp)
target_include_directories(test_data_accumulator PRIVATE ../src)
target_link_libraries(test_data_accumulator PRIVATE Catch2::Catch2WithMain)
add_test(data_accumulator COMMAND test_data_accumulator)
