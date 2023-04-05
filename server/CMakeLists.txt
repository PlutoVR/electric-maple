# Copyright 2023, Pluto VR, Inc.
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

# Helper to obey CMP0077 and work around warnings.
function(setForce NAME VALUE)
	set(${NAME} ${VALUE} CACHE INTERNAL "" FORCE)
endfunction()

# Let's use this for now: Monado's main compositor is really good and is an
#easy way to see that things are working.
setForce(XRT_FEATURE_COMPOSITOR_MAIN ON)

# No reason to disable this
setForce(XRT_FEATURE_COMPOSITOR_NULL ON)

# It looks like we can use Monado's normal out-of-proc OpenXR instance as-is. We'll see.
setForce(XRT_FEATURE_OPENXR ON)

setForce(XRT_FEATURE_RENDERDOC OFF) # we are not using this
setForce(XRT_FEATURE_STEAMVR_PLUGIN OFF) # we are not using this
setForce(XRT_FEATURE_TRACING OFF) # we are not using this yet
setForce(XRT_FEATURE_SLAM OFF) # we are not using this

setForce(XRT_FEATURE_CLIENT_DEBUG_GUI OFF) # we are not using this

setForce(XRT_MODULE_MONADO_CLI OFF) # we are not using this
setForce(XRT_MODULE_MONADO_GUI OFF) # we are not using this
setForce(XRT_MODULE_MERCURY_HANDTRACKING OFF)

setForce(XRT_HAVE_D3D11 OFF)
setForce(XRT_HAVE_D3D12 OFF)
setForce(XRT_HAVE_OPENCV OFF)

# No drivers!
setForce(XRT_BUILD_DRIVER_ANDROID OFF)
setForce(XRT_BUILD_DRIVER_ARDUINO OFF)
setForce(XRT_BUILD_DRIVER_DAYDREAM OFF)
setForce(XRT_BUILD_DRIVER_DEPTHAI OFF)
setForce(XRT_BUILD_DRIVER_EUROC OFF)
setForce(XRT_BUILD_DRIVER_HANDTRACKING OFF)
setForce(XRT_BUILD_DRIVER_TWRAP OFF)
setForce(XRT_BUILD_DRIVER_HDK OFF)
setForce(XRT_BUILD_DRIVER_HYDRA OFF)
setForce(XRT_BUILD_DRIVER_ILLIXR OFF)
setForce(XRT_BUILD_DRIVER_NS OFF)
setForce(XRT_BUILD_DRIVER_OHMD OFF)
setForce(XRT_BUILD_DRIVER_OPENGLOVES OFF)
setForce(XRT_BUILD_DRIVER_PSMV OFF)
setForce(XRT_BUILD_DRIVER_PSVR OFF)
setForce(XRT_BUILD_DRIVER_QWERTY OFF)
setForce(XRT_BUILD_DRIVER_REALSENSE OFF)
setForce(XRT_BUILD_DRIVER_REMOTE OFF)
setForce(XRT_BUILD_DRIVER_RIFT_S OFF)
setForce(XRT_BUILD_DRIVER_SURVIVE OFF)
setForce(XRT_BUILD_DRIVER_ULV2 OFF)
setForce(XRT_BUILD_DRIVER_VF OFF)
setForce(XRT_BUILD_DRIVER_VIVE OFF)
setForce(XRT_BUILD_DRIVER_WMR OFF)
setForce(XRT_BUILD_DRIVER_SIMULAVR OFF)

# There is no clean way to disable the prober/builders (as far as I can see?)
# So we enable one driver so that the legacy builder does not die
setForce(XRT_BUILD_DRIVER_SIMULATED ON)

setForce(XRT_BUILD_SAMPLES OFF)

setForce(BUILD_TESTING OFF)
setForce(BUILD_DOC OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(XRT_OXR_RUNTIME_SUFFIX pluto_server)

add_subdirectory(../monado ${CMAKE_CURRENT_BINARY_DIR}/monado)

add_subdirectory(../proto ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_subdirectory(src)
