# Copyright 2018-2022, Collabora, Ltd.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.2)
project(linux-streaming VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Make sure we have pretty colours
option(DISABLE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)

if(NOT DISABLE_COLORED_OUTPUT)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		add_compile_options(-fcolor-diagnostics)
	endif()
endif()

# Get Monado's CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/monado/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/monado/cmake/sanitizers")

if(NOT MSVC)
	set(CMAKE_C_FLAGS
	    "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wno-unused-parameter -Werror=incompatible-pointer-types"
		)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
endif()

# Default to PIC code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MONADO_SRC_XRT ${CMAKE_CURRENT_SOURCE_DIR}/monado/src/xrt)

### Set configuration options for Monado

# Let's use this for now: Monado's main compositor is really good and is an
#easy way to see that things are working.
set(XRT_FEATURE_COMPOSITOR_MAIN ON)

# No reason to disable this
set(XRT_FEATURE_COMPOSITOR_NULL ON)

# It looks like we can use Monado's normal out-of-proc OpenXR instance as-is. We'll see.
set(XRT_FEATURE_OPENXR ON)

set(XRT_FEATURE_RENDERDOC OFF) # we are not using this
set(XRT_FEATURE_STEAMVR_PLUGIN OFF) # we are not using this
set(XRT_FEATURE_TRACING OFF) # we are not using this yet
set(XRT_FEATURE_SLAM OFF) # we are not using this

set(XRT_FEATURE_CLIENT_DEBUG_GUI OFF) # we are not using this

set(XRT_MODULE_MONADO_CLI OFF) # we are not using this
set(XRT_MODULE_MONADO_GUI OFF) # we are not using this
set(XRT_MODULE_MERCURY_HANDTRACKING OFF)

set(XRT_HAVE_D3D11 OFF)
set(XRT_HAVE_D3D12 OFF)
set(XRT_HAVE_OPENCV OFF)

# No drivers!
set(XRT_BUILD_DRIVER_ANDROID OFF)
set(XRT_BUILD_DRIVER_ARDUINO OFF)
set(XRT_BUILD_DRIVER_DAYDREAM OFF)
set(XRT_BUILD_DRIVER_DEPTHAI OFF)
set(XRT_BUILD_DRIVER_EUROC OFF)
set(XRT_BUILD_DRIVER_HANDTRACKING OFF)
set(XRT_BUILD_DRIVER_TWRAP OFF)
set(XRT_BUILD_DRIVER_HDK OFF)
set(XRT_BUILD_DRIVER_HYDRA OFF)
set(XRT_BUILD_DRIVER_ILLIXR OFF)
set(XRT_BUILD_DRIVER_NS OFF)
set(XRT_BUILD_DRIVER_OHMD OFF)
set(XRT_BUILD_DRIVER_OPENGLOVES OFF)
set(XRT_BUILD_DRIVER_PSMV OFF)
set(XRT_BUILD_DRIVER_PSVR OFF)
set(XRT_BUILD_DRIVER_QWERTY OFF)
set(XRT_BUILD_DRIVER_REALSENSE OFF)
set(XRT_BUILD_DRIVER_REMOTE OFF)
set(XRT_BUILD_DRIVER_RIFT_S OFF)
set(XRT_BUILD_DRIVER_SURVIVE OFF)
set(XRT_BUILD_DRIVER_ULV2 OFF)
set(XRT_BUILD_DRIVER_VF OFF)
set(XRT_BUILD_DRIVER_VIVE OFF)
set(XRT_BUILD_DRIVER_WMR OFF)
set(XRT_BUILD_DRIVER_SIMULAVR OFF)

# There is no clean way to disable the prober/builders (as far as I can see?)
# So we enable one driver so that the legacy builder does not die
set(XRT_BUILD_DRIVER_SIMULATED ON)

set(XRT_BUILD_SAMPLES OFF)

set(BUILD_TESTING OFF)
set(BUILD_DOC OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(XRT_OXR_RUNTIME_SUFFIX pluto_server)

add_subdirectory(../monado ${CMAKE_CURRENT_BINARY_DIR}/monado)

add_subdirectory(../proto ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_subdirectory(src)
