# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.10.2)

project(electricmaple_standalone_client VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(OpenXR REQUIRED)
find_package(EGL REQUIRED)
find_package(OpenGLES REQUIRED COMPONENTS V3)

include(CTest)

# Default to PIC code
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

### Set configuration options for Monado

# Helper to obey CMP0077 and work around warnings.
function(setForce NAME VALUE)
	set(${NAME}
	    ${VALUE}
	    CACHE INTERNAL "" FORCE
		)
endfunction()

setforce(XRT_FEATURE_COLOR_LOG OFF)
setforce(XRT_FEATURE_COMPOSITOR_MAIN OFF)
setforce(XRT_FEATURE_COMPOSITOR_NULL OFF)
setforce(XRT_FEATURE_OPENXR OFF)
setforce(XRT_FEATURE_RENDERDOC OFF)
setforce(XRT_FEATURE_STEAMVR_PLUGIN OFF)
setforce(XRT_FEATURE_TRACING OFF)
setforce(XRT_FEATURE_SLAM OFF)

setforce(XRT_FEATURE_CLIENT_DEBUG_GUI OFF)

setforce(XRT_MODULE_MONADO_CLI OFF)
setforce(XRT_MODULE_MONADO_GUI OFF)
setforce(XRT_MODULE_MERCURY_HANDTRACKING OFF)

setforce(XRT_HAVE_D3D11 OFF)
setforce(XRT_HAVE_D3D12 OFF)
setforce(XRT_HAVE_OPENCV OFF)

# No drivers!
setforce(XRT_BUILD_DRIVER_ANDROID OFF)
setforce(XRT_BUILD_DRIVER_ARDUINO OFF)
setforce(XRT_BUILD_DRIVER_DAYDREAM OFF)
setforce(XRT_BUILD_DRIVER_DEPTHAI OFF)
setforce(XRT_BUILD_DRIVER_EUROC OFF)
setforce(XRT_BUILD_DRIVER_HANDTRACKING OFF)
setforce(XRT_BUILD_DRIVER_TWRAP OFF)
setforce(XRT_BUILD_DRIVER_HDK OFF)
setforce(XRT_BUILD_DRIVER_HYDRA OFF)
setforce(XRT_BUILD_DRIVER_ILLIXR OFF)
setforce(XRT_BUILD_DRIVER_NS OFF)
setforce(XRT_BUILD_DRIVER_OHMD OFF)
setforce(XRT_BUILD_DRIVER_OPENGLOVES OFF)
setforce(XRT_BUILD_DRIVER_PSMV OFF)
setforce(XRT_BUILD_DRIVER_PSVR OFF)
setforce(XRT_BUILD_DRIVER_QWERTY OFF)
setforce(XRT_BUILD_DRIVER_REALSENSE OFF)
setforce(XRT_BUILD_DRIVER_REMOTE OFF)
setforce(XRT_BUILD_DRIVER_RIFT_S OFF)
setforce(XRT_BUILD_DRIVER_SURVIVE OFF)
setforce(XRT_BUILD_DRIVER_ULV2 OFF)
setforce(XRT_BUILD_DRIVER_VF OFF) # Hi!
setforce(XRT_BUILD_DRIVER_VIVE OFF)
setforce(XRT_BUILD_DRIVER_WMR OFF)
setforce(XRT_BUILD_DRIVER_SIMULAVR OFF)
setforce(XRT_BUILD_DRIVER_STEAMVR_LIGHTHOUSE OFF)

# There is no clean way to disable the prober/builders (as far as I can see?)
# So we enable one driver so that the legacy builder does not die
setforce(XRT_BUILD_DRIVER_SIMULATED ON)

setforce(XRT_BUILD_SAMPLES OFF)

setforce(BUILD_TESTING OFF)
setforce(BUILD_DOC OFF)

setforce(XRT_MODULE_IPC OFF)
setforce(XRT_FEATURE_SERVICE OFF)

# Add select libraries from Monado

set(NANOPB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../monado/src/external/nanopb")
add_library(
	xrt-external-nanopb STATIC "${NANOPB_ROOT}/pb_common.c" "${NANOPB_ROOT}/pb_decode.c"
				   "${NANOPB_ROOT}/pb_encode.c"
	)
target_include_directories(xrt-external-nanopb PUBLIC "${NANOPB_ROOT}")
add_library(xrt-interfaces INTERFACE)
target_include_directories(xrt-interfaces INTERFACE ../monado/src/xrt/include)

add_library(aux-includes INTERFACE)
target_include_directories(aux-includes INTERFACE ../monado/src/xrt/auxiliary)
target_link_libraries(aux-includes INTERFACE xrt-interfaces)

add_library(
	aux_util STATIC ../monado/src/xrt/auxiliary/util/u_misc.c
			../monado/src/xrt/auxiliary/util/u_time.cpp
	)
target_link_libraries(aux_util PUBLIC aux-includes)

if(ANDROID)
	find_library(ANDROID_LOG_LIBRARY log)
	find_library(ANDROID_LIBRARY android)
endif()

add_subdirectory(../proto ${CMAKE_CURRENT_BINARY_DIR}/proto)
add_subdirectory(../external/Catch2 catch2)

if(ANDROID)
	if(ANDROID_ABI STREQUAL "arm64-v8a")
		set(GST_ARCH arm64)
	elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
		set(GST_ARCH armv7)
	elseif(ANDROID_ABI STREQUAL "x86")
		set(GST_ARCH x86)
	elseif(ANDROID_ABI STREQUAL "x86_64")
		set(GST_ARCH x86_64)
	else()
		message(FATAL_ERROR "What is this ABI? ${ANDROID_ABI}")
	endif()

	if(GST_ARCH_DIR)
		message(STATUS "Using gstreamer from ${GST_ARCH_DIR}")
	else()
		if(GST_BASE_DIR)
			message(STATUS "GST_ARCH_DIR not set, computing from arch and GST_BASE_DIR")
		else()
			message(
				STATUS
					"GST_ARCH_DIR and GST_BASE_DIR not set, assuming default location"
				)
			set(GST_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/gstreamer_android")
		endif()
		set(GST_ARCH_DIR ${GST_BASE_DIR}/${GST_ARCH})
	endif()

	set(USE_GST_SHARED OFF)
	if(USE_GST_SHARED)
		include(libraries_shared.cmake)
	else()
		include(libraries_static.cmake)
	endif()

	set(GST_INCLUDE_DIRS ${GST_ARCH_DIR}/include/gstreamer-1.0/
			     ${GST_ARCH_DIR}/lib/gstreamer-1.0/include/
		)
	set(GLIB_INCLUDE_DIRS ${GST_ARCH_DIR}/include/glib-2.0
			      ${GST_ARCH_DIR}/lib/glib-2.0/include/
		)
	set(LIBSOUP_INCLUDE_DIRS ${GST_ARCH_DIR}/include/libsoup-2.4/)
	set(JSONGLIB_INCLUDE_DIRS ${GST_ARCH_DIR}/include/json-glib-1.0)
	set(GIO_INCLUDE_DIRS ${GST_ARCH_DIR}/include/glib-2.0/ ${GST_ARCH_DIR}/lib/gio-unix-2.0/)

	add_subdirectory(gstreamer_android)
	add_subdirectory(egl)
	add_subdirectory(src)
endif()

add_subdirectory(tests)
