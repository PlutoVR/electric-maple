cmake_minimum_required(VERSION 3.10.2)

project(plutovr_webrtc_client VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

if(ANDROID_ABI STREQUAL "arm64-v8a")
	set(GST_ARCH_DIR arm64)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
	set(GST_ARCH_DIR armv7)
elseif(ANDROID_ABI STREQUAL "x86")
	set(GST_ARCH_DIR x86)
elseif(ANDROID_ABI STREQUAL "x86_64")
	set(GST_ARCH_DIR x86_64)
else()
	message(FATAL_ERROR "What is this ABI? ${ANDROID_ABI}")
endif()

set(gstreamer_android_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/gstreamer_android")

set(GST_ROOT ${gstreamer_android_SOURCE_DIR}/${GST_ARCH_DIR})

# add_library(openxr_loader SHARED IMPORTED)
# set_property(
# 	TARGET openxr_loader
# 	PROPERTY
# 		IMPORTED_LOCATION
# 		${OCULUS_OPENXR_MOBILE_SDK}/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
# 	)

# include(FetchContent)

# FetchContent_Declare(
# 	gstreamer_android
# 	URL https://gstreamer.freedesktop.org/data/pkg/android/1.22.2/gstreamer-1.0-android-universal-1.22.2.tar.xz
# 	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
# 	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
# 	)

# if(NOT gstreamer_android_POPULATED)
# 	# Fetch the content using previously declared details
# 	FetchContent_Populate(gstreamer_android)

# 	# Set custom variables, policies, etc.
# 	# ...

# 	# Bring the populated content into the build
# 	# add_subdirectory(${gstreamer_android_SOURCE_DIR} ${gstreamer_android_BINARY_DIR})

# endif()

# FetchContent_MakeAvailable(gstreamer_android)

# set(android_runtime_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/android_runtime")

# set(ART_LIBRARIES
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libadbconnection.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libandroidio.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libart-compiler.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libart-dexlayout.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libart-disassembler.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libart.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libartbase.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libartpalette.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libbacktrace.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libbase.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libc++.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libcrypto.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libdexfile.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libdt_fd_forward.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libdt_socket.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libexpat.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libjavacore.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libjdwp.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/liblz4.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/liblzma.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libnativebridge.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libnativehelper.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libnativeloader.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libnpt.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libopenjdk.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libopenjdkjvm.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libopenjdkjvmti.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libperfetto_hprof.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libprofile.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libsigchain.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libunwindstack.so
#     ${android_runtime_SOURCE_DIR}/com.android.art/lib64/libziparchive.so)

# set(ADBD_LIBRARIES
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libadb_pairing_auth.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libadb_pairing_connection.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libadb_pairing_server.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libadb_protos.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libadbconnection_client.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libbase.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libc++.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libcrypto.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libcrypto_utils.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libcutils.so
#     ${android_runtime_SOURCE_DIR}/com.android.adbd/lib64/libprotobuf-cpp-lite.so)

# set(ANDROID_RUNTIME_LIBRARIES
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/ld-android.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libbase.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libc++.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libc_malloc_debug.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libc_malloc_hooks.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/liblzma.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libprocinfo.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/libunwindstack.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/bionic/libc.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/bionic/libdl.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/bionic/libdl_android.so
#     ${android_runtime_SOURCE_DIR}/com.android.runtime/lib64/bionic/libm.so)
# set(OS_STATSD_LIBRARIES
#     ${android_runtime_SOURCE_DIR}/com.android.os.statsd/lib64/libstats_jni.so
#     ${android_runtime_SOURCE_DIR}/com.android.os.statsd/lib64/libstatspull.so
#     ${android_runtime_SOURCE_DIR}/com.android.os.statsd/lib64/libstatssocket.so)

# set(ANDROID_SYSTEM_LIB64_LIBRARIES
#     ${android_runtime_SOURCE_DIR}/system-lib64/heapprofd_client_api.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libartpalette-system.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libprocessgroup.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libtombstoned_client.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libselinux.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libcgrouprc.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libpcre2.so
#     ${android_runtime_SOURCE_DIR}/system-lib64/libpackagelistparser.so)

# set(GST_LIBRARIES
# # Order matters for gobject, gmodule, glib.
# ${GST_ROOT}/lib/libgobject-2.0.a
# ${GST_ROOT}/lib/libgmodule-2.0.a
# ${GST_ROOT}/lib/libglib-2.0.a
# # Doesn't seem like order matters for the stuff after this.

# ${GST_ROOT}/lib/libgstreamer-1.0.a
# ${GST_ROOT}/lib/libgstbase-1.0.a
# ${GST_ROOT}/lib/libgstapp-1.0.a
# ${GST_ROOT}/lib/libgstvideo-1.0.a

# ${GST_ROOT}/lib/gstreamer-1.0/libgstvideotestsrc.a

# ${GST_ROOT}/lib/libiconv.a
# ${GST_ROOT}/lib/libintl.a
# ${GST_ROOT}/lib/libffi.a
# ${GST_ROOT}/lib/libpcre2-8.a
# ${GST_ROOT}/lib/liborc-0.4.a
# )

set(GST_LIBRARIES
    #${GST_ROOT}/lib/libgstreamer-1.0.a
    ${GST_ROOT}/lib/libgstrtsp-1.0.a
    ${GST_ROOT}/lib/libgstphotography-1.0.a
    ${GST_ROOT}/lib/liboggkate.a
    ${GST_ROOT}/lib/libx264.a
    ${GST_ROOT}/lib/libspeex.a
    ${GST_ROOT}/lib/libtheoradec.a
    ${GST_ROOT}/lib/libgstaudio-1.0.a
    ${GST_ROOT}/lib/libgstfft-1.0.a
    ${GST_ROOT}/lib/libpcre2-8.a
    ${GST_ROOT}/lib/libtheoraenc.a
    ${GST_ROOT}/lib/libgsttranscoder-1.0.a
    ${GST_ROOT}/lib/libiconv.a
    ${GST_ROOT}/lib/libintl.a
    ${GST_ROOT}/lib/libgstbadaudio-1.0.a
    ${GST_ROOT}/lib/libssl.a
    ${GST_ROOT}/lib/libgstwebrtcnice-1.0.a
    ${GST_ROOT}/lib/libopenjp2.a
    ${GST_ROOT}/lib/libfreetype.a
    ${GST_ROOT}/lib/libgstcodecparsers-1.0.a
    ${GST_ROOT}/lib/libzbar.a
    ${GST_ROOT}/lib/liborc-test-0.4.a
    ${GST_ROOT}/lib/libpangocairo-1.0.a
    ${GST_ROOT}/lib/libgstvalidate-1.0.a
    ${GST_ROOT}/lib/libffi.a
    ${GST_ROOT}/lib/libgstvideo-1.0.a
    ${GST_ROOT}/lib/libavformat.a
    ${GST_ROOT}/lib/libgstrtp-1.0.a
    ${GST_ROOT}/lib/libgsturidownloader-1.0.a
    ${GST_ROOT}/lib/libwavpack.a
    ${GST_ROOT}/lib/libvorbis.a
    ${GST_ROOT}/lib/libgstapp-1.0.a
    ${GST_ROOT}/lib/libopus.a
    ${GST_ROOT}/lib/libglib-2.0.a
    ${GST_ROOT}/lib/libgstinsertbin-1.0.a
    ${GST_ROOT}/lib/libcairo.a
    ${GST_ROOT}/lib/libgstisoff-1.0.a
    # ${GST_ROOT}/lib/libvorbisidec.a
    ${GST_ROOT}/lib/libgstsctp-1.0.a
    ${GST_ROOT}/lib/libkate.a
    ${GST_ROOT}/lib/libdca.a
    ${GST_ROOT}/lib/libopenh264.a
    ${GST_ROOT}/lib/librtmp.a
    ${GST_ROOT}/lib/libpng16.a
    ${GST_ROOT}/lib/libfontconfig.a
    ${GST_ROOT}/lib/libgstvulkan-1.0.a
    ${GST_ROOT}/lib/libvorbisfile.a
    ${GST_ROOT}/lib/libgstmpegts-1.0.a
    ${GST_ROOT}/lib/libtheora.a
    ${GST_ROOT}/lib/libdav1d.a
    ${GST_ROOT}/lib/libpangoft2-1.0.a
    ${GST_ROOT}/lib/libgstsdp-1.0.a
    ${GST_ROOT}/lib/libgstnet-1.0.a
    ${GST_ROOT}/lib/libopencore-amrwb.a
    ${GST_ROOT}/lib/libharfbuzz.a
    ${GST_ROOT}/lib/libcrypto.a
    ${GST_ROOT}/lib/libmp3lame.a
    ${GST_ROOT}/lib/libspandsp.a
    ${GST_ROOT}/lib/libgmodule-2.0.a
    ${GST_ROOT}/lib/libbz2.a
    ${GST_ROOT}/lib/libcroco-0.6.a
    ${GST_ROOT}/lib/libjson-glib-1.0.a
    ${GST_ROOT}/lib/libgstriff-1.0.a
    ${GST_ROOT}/lib/libgstplayer-1.0.a
    ${GST_ROOT}/lib/libgstcodecs-1.0.a
    ${GST_ROOT}/lib/libges-1.0.a
    ${GST_ROOT}/lib/libwebrtc_audio_processing.a
    ${GST_ROOT}/lib/libavcodec.a
    ${GST_ROOT}/lib/libjpeg.a
    ${GST_ROOT}/lib/libxml2.a
    ${GST_ROOT}/lib/libpango-1.0.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrsvg.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstudp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstproxy.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsdpelem.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstid3demux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstclosedcaption.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdvdsub.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdvbsuboverlay.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopusparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiomixmatrix.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstflv.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstadpcmenc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideofiltersbad.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtpmanager.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgoom.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudioparsers.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideorate.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideofilter.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstwavpack.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvulkan.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsty4mdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsrtp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaiff.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsubenc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstapetag.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaes.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideoframe_audiolevel.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudioconvert.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstbayer.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsticydemux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstfrei0r.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstjpegformat.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiobuffersplit.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtpmanagerbad.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstadpcmdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmpegpsdemux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstnle.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstbz2.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiofx.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiorate.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideotestsrc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstogg.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstfieldanalysis.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideobox.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstwavenc.a
    # ${GST_ROOT}/lib/gstreamer-1.0/libgstamrnb.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstadaptivedemux2.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrealmedia.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopenh264.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcoreelements.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsthls.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstwavparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrist.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstxingmux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdtsdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsoup.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstauparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvolume.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsttheora.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstkate.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtsp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstencoding.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsmoothstreaming.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstpango.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstid3tag.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiolatency.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsegmentclip.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcoloreffects.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstspeed.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmpg123.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtspclientsink.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsoundtouch.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstadder.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsttypefindfunctions.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstautoconvert.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstplayback.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudioresample.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstasfmux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiofxbad.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopus.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsttaglib.a
    # ${GST_ROOT}/lib/gstreamer-1.0/libgstamrwbdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmpegtsmux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsta52dec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstwebrtc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmultipart.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdeinterlace.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsmpte.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsty4menc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstnetsim.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgeometrictransform.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtmp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopenjpeg.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcodectimestamper.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideocrop.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdebugutilsbad.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcoretracers.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstassrender.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstzbar.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstladspa.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiovisualizers.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstx264.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstalphacolor.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstlibav.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstremovesilence.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstges.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgio.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstlevel.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgdp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstalpha.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstautodetect.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstinterleave.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvorbis.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstimagefreeze.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmxf.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideoconvertscale.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstspeex.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopengl.a
    # ${GST_ROOT}/lib/gstreamer-1.0/libgstivorbisdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstjpeg.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstshapewipe.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstalaw.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstspandsp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdvdlpcmdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmultifile.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdv.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstreplaygain.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcamerabin.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstipcpipeline.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstinterlace.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrfbsrc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcutter.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsrt.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtmp2.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmidi.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmulaw.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdebug.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstandroidmedia.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstlame.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmatroska.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcairo.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgaudieffects.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstpnm.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmpegtsdemux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsctp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstoverlaycomposition.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstflac.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdtls.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstwebrtcdsp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstpcapparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsttimecode.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsteffectv.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstnice.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsiren.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstmpegpsmux.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiomixer.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdvdspu.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstspectrum.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrawparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideoparsersbad.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstcompositor.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstisomp4.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstinter.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgdkpixbuf.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstequalizer.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstlegacyrawparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdtmf.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstpng.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsmooth.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvpx.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstivtc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstasf.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsbc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstdash.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvideomixer.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgsttcp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstapp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaccurip.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstopensles.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstgoom2k1.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstfreeverb.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstrtp.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstsubparse.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstaudiotestsrc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstflxdec.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstvoaacenc.a
    ${GST_ROOT}/lib/gstreamer-1.0/libgstavi.a
    ${GST_ROOT}/lib/libfribidi.a
    ${GST_ROOT}/lib/libgraphene-1.0.a
    ${GST_ROOT}/lib/libgstwebrtc-1.0.a
    ${GST_ROOT}/lib/libcairo-gobject.a
    ${GST_ROOT}/lib/liborc-0.4.a
    ${GST_ROOT}/lib/librsvg-2.a
    ${GST_ROOT}/lib/libSoundTouch.a
    ${GST_ROOT}/lib/libsqlite3.a
    ${GST_ROOT}/lib/libsrtp2.a
    ${GST_ROOT}/lib/libgstgl-1.0.a
    ${GST_ROOT}/lib/liba52.a
    ${GST_ROOT}/lib/libvorbisenc.a
    ${GST_ROOT}/lib/libgstpbutils-1.0.a
    ${GST_ROOT}/lib/libcairo-script-interpreter.a
    ${GST_ROOT}/lib/libgobject-2.0.a
    ${GST_ROOT}/lib/libpixman-1.a
    ${GST_ROOT}/lib/libdv.a
    ${GST_ROOT}/lib/libogg.a
    ${GST_ROOT}/lib/libtiff.a
    ${GST_ROOT}/lib/libtag.a
    ${GST_ROOT}/lib/libsrt.a
    ${GST_ROOT}/lib/libnice.a
    ${GST_ROOT}/lib/libgstplay-1.0.a
    ${GST_ROOT}/lib/libgstallocators-1.0.a
    ${GST_ROOT}/lib/gio/modules/libgioopenssl.a
    ${GST_ROOT}/lib/libgdk_pixbuf-2.0.a
    ${GST_ROOT}/lib/libgstbasecamerabinsrc-1.0.a
    ${GST_ROOT}/lib/libmpg123.a
    ${GST_ROOT}/lib/libswresample.a
    ${GST_ROOT}/lib/libz.a
    ${GST_ROOT}/lib/libsoup-2.4.a
    ${GST_ROOT}/lib/libFLAC.a
    ${GST_ROOT}/lib/libgio-2.0.a
    ${GST_ROOT}/lib/libexpat.a
    ${GST_ROOT}/lib/libvpx.a
    ${GST_ROOT}/lib/libavutil.a
    ${GST_ROOT}/lib/libgstadaptivedemux-1.0.a
    ${GST_ROOT}/lib/libvo-aacenc.a
    ${GST_ROOT}/lib/libsbc.a
    # ${GST_ROOT}/lib/libopencore-amrnb.a
    ${GST_ROOT}/lib/libgstrtspserver-1.0.a
    ${GST_ROOT}/lib/libgsttag-1.0.a
    ${GST_ROOT}/lib/libpsl.a
    ${GST_ROOT}/lib/libass.a
    ${GST_ROOT}/lib/libgthread-2.0.a
    ${GST_ROOT}/lib/libcharset.a
    ${GST_ROOT}/lib/libgstbase-1.0.a
    ${GST_ROOT}/lib/libgstcheck-1.0.a
    ${GST_ROOT}/lib/libavfilter.a
    ${GST_ROOT}/lib/libgstcontroller-1.0.a
    ${GST_ROOT}/lib/libturbojpeg.a
	)

# ${GLIB_LIBRARIES}
# ${LIBSOUP_LIBRARIES}
# ${JSONGLIB_LIBRARIES}
# ${GIO_LIBRARIES}

# set(GLIB_LIBRARY_DIRS ${GST_ROOT}/lib)
# set(LIBSOUP_LIBRARY_DIRS ${GST_ROOT}/lib)
set(JSONGLIB_LIBRARIES ${GST_ROOT}/lib/libjson-glib-1.0.a)
# set(GIO_LIBRARY_DIRS ${GST_ROOT}/lib)

# find_library(GLIB_LIBRARIES glib-2.0 HINTS ${GLIB_LIBRARY_DIRS})
# find_library(LIBSOUP_LIBRARIES soup-2.4 HINTS ${LIBSOUP_LIBRARY_DIRS})
# find_library(JSONGLIB_LIBRARIES json-glib-1.0 HINTS ${JSONGLIB_LIBRARY_DIRS})
# find_library(GIO_LIBRARIES gio-2.0 HINTS ${GIO_LIBRARY_DIRS})

set(GST_INCLUDE_DIRS ${GST_ROOT}/include/gstreamer-1.0/ ${GST_ROOT}/lib/gstreamer-1.0/include/)
set(GLIB_INCLUDE_DIRS ${GST_ROOT}/include/glib-2.0 ${GST_ROOT}/lib/glib-2.0/include/)
set(LIBSOUP_INCLUDE_DIRS ${GST_ROOT}/include/libsoup-2.4/)
set(JSONGLIB_INCLUDE_DIRS ${GST_ROOT}/include/json-glib-1.0)
set(GIO_INCLUDE_DIRS ${GST_ROOT}/include/glib-2.0/ ${GST_ROOT}/lib/gio-unix-2.0/)

# These two don't seem to help
# include_directories(
# 	${GSTREAMER_ROOT}/include/gstreamer-1.0 ${GSTREAMER_ROOT}/include/glib-2.0
# 	${GSTREAMER_ROOT}/lib/glib-2.0/include
# 	)

# link_directories(${GSTREAMER_ROOT}/lib ${GSTREAMER_ROOT}/lib/gstreamer-1.0)

# message(gstreamer_android_SOURCE_DIR)
# message(GST_LIBRARIES)

# message(ART_LIBRARIES)

add_subdirectory(src)
