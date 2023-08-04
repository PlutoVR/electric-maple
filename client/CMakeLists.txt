cmake_minimum_required(VERSION 3.10.2)

project(plutovr_webrtc_client VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(OpenXR REQUIRED)
find_package(EGL REQUIRED)
find_package(OpenGLES REQUIRED COMPONENTS V3)

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

# There is no clean way to disable the prober/builders (as far as I can see?)
# So we enable one driver so that the legacy builder does not die
setforce(XRT_BUILD_DRIVER_SIMULATED ON)

setforce(XRT_BUILD_SAMPLES OFF)

setforce(BUILD_TESTING OFF)
setforce(BUILD_DOC OFF)

setforce(XRT_MODULE_IPC OFF)
setforce(XRT_FEATURE_SERVICE OFF)

add_subdirectory(../monado ${CMAKE_CURRENT_BINARY_DIR}/monado)
add_subdirectory(../proto ${CMAKE_CURRENT_BINARY_DIR}/proto)

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

	include(libraries_static.cmake)
	set(GST_INCLUDE_DIRS ${GST_ARCH_DIR}/include/gstreamer-1.0/
			     ${GST_ARCH_DIR}/lib/gstreamer-1.0/include/
		)
	set(GLIB_INCLUDE_DIRS ${GST_ARCH_DIR}/include/glib-2.0
			      ${GST_ARCH_DIR}/lib/glib-2.0/include/
		)
	set(LIBSOUP_INCLUDE_DIRS ${GST_ARCH_DIR}/include/libsoup-2.4/)
	set(JSONGLIB_INCLUDE_DIRS ${GST_ARCH_DIR}/include/json-glib-1.0)
	set(GIO_INCLUDE_DIRS ${GST_ARCH_DIR}/include/glib-2.0/ ${GST_ARCH_DIR}/lib/gio-unix-2.0/)

else()
	find_package(PkgConfig)
	pkg_check_modules(PC_LIBSOUP3 IMPORTED_TARGET libsoup-3.0 REQUIRED)
	set(LIBSOUP_LIBRARIES PkgConfig::PC_LIBSOUP3)
	pkg_check_modules(PC_JSON_GLIB IMPORTED_TARGET json-glib-1.0 REQUIRED)
	set(JSONGLIB_LIBRARIES PkgConfig::PC_JSON_GLIB)
	pkg_check_modules(
		PC_GLIB_GIO
		IMPORTED_TARGET
		glib-2.0
		gio-2.0
		gio-unix-2.0
		gobject-2.0
		REQUIRED
		)
	set(GLIB_LIBRARIES PkgConfig::PC_GLIB_GIO)

	pkg_check_modules(
		PC_GST
		IMPORTED_TARGET
		gstreamer-webrtc-1.0
		gstreamer-gl-1.0
		gstreamer-gl-egl-1.0
		gstreamer-app-1.0
		REQUIRED
		)
	set(GST_LIBRARIES PkgConfig::PC_GST)
	set(LIBSOUP_INCLUDE_DIRS)
	set(JSONGLIB_INCLUDE_DIRS)
	set(GIO_INCLUDE_DIRS)
	set(GST_INCLUDE_DIRS ${PC_GST_INCLUDE_DIRS})
	set(GLIB_INCLUDE_DIRS)

endif()

if(ANDROID)
	add_subdirectory(gstreamer_android)
endif()
add_subdirectory(src)
