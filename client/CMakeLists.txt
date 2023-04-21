cmake_minimum_required(VERSION 3.20.0)

project("plutovr_webrtc_client")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(OpenXR REQUIRED)

# Default to PIC code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

add_library(openxr_loader SHARED IMPORTED)
set_property(
	TARGET openxr_loader
	PROPERTY
		IMPORTED_LOCATION
		${CMAKE_CURRENT_SOURCE_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
	)

include(FetchContent)

FetchContent_Declare(
	gstreamer_android
	URL https://gstreamer.freedesktop.org/data/pkg/android/1.22.2/gstreamer-1.0-android-universal-1.22.2.tar.xz
	    DOWNLOAD_EXTRACT_TIMESTAMP
	    TRUE
	    PREFIX
	    ${CMAKE_CURRENT_BINARY_DIR}/external
	)


# if(NOT gstreamer_android_POPULATED)
# 	# Fetch the content using previously declared details
# 	FetchContent_Populate(gstreamer_android)

# 	# Set custom variables, policies, etc.
# 	# ...

# 	# Bring the populated content into the build
# 	# add_subdirectory(${gstreamer_android_SOURCE_DIR} ${gstreamer_android_BINARY_DIR})


# endif()

FetchContent_MakeAvailable(gstreamer_android)

set(GST_LIBRARIES
# Order matters for gobject, gmodule, glib.
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgobject-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgmodule-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libglib-2.0.a
# Doesn't seem like order matters for the stuff after this.
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstbase-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstapp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstvideo-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libiconv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libintl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libffi.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpcre2-8.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/liborc-0.4.a
)
set(GST_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/gstreamer-1.0/ )
set(GLIB_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/glib-2.0 ${gstreamer_android_SOURCE_DIR}/arm64/lib/glib-2.0/include/)

add_subdirectory(src)
