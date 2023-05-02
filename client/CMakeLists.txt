cmake_minimum_required(VERSION 3.10.2)

project("plutovr_webrtc_client" VERSION 0.0.1)

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
		${OCULUS_OPENXR_MOBILE_SDK}/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
	)

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

set(gstreamer_android_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/gstreamer_android")

# set(GST_LIBRARIES
# # Order matters for gobject, gmodule, glib.
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgobject-2.0.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgmodule-2.0.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libglib-2.0.a
# # Doesn't seem like order matters for the stuff after this.

# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstbase-1.0.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstapp-1.0.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstvideo-1.0.a

# ${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideotestsrc.a

# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libiconv.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libintl.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libffi.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/libpcre2-8.a
# ${gstreamer_android_SOURCE_DIR}/arm64/lib/liborc-0.4.a
# )


set(GST_LIBRARIES
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstrtsp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstphotography-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/liboggkate.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libx264.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libspeex.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libtheoradec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstaudio-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstfft-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpcre2-8.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libtheoraenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgsttranscoder-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libiconv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libintl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstbadaudio-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libssl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstwebrtcnice-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libopenjp2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libfreetype.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstcodecparsers-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libzbar.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/liborc-test-0.4.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpangocairo-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstvalidate-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libffi.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstvideo-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libavformat.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstrtp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgsturidownloader-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libwavpack.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvorbis.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstapp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libopus.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libglib-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstinsertbin-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcairo.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstisoff-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvorbisidec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstsctp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libkate.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libdca.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libopenh264.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/librtmp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpng16.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libfontconfig.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstvulkan-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvorbisfile.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstmpegts-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libtheora.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libdav1d.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpangoft2-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstsdp-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstnet-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libopencore-amrwb.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libharfbuzz.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcrypto.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libmp3lame.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libspandsp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgmodule-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libbz2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcroco-0.6.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libjson-glib-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstriff-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstplayer-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstcodecs-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libges-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libwebrtc_audio_processing.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libavcodec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libjpeg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libxml2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpango-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrsvg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstudp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstproxy.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsdpelem.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstid3demux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstclosedcaption.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdvdsub.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdvbsuboverlay.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopusparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiomixmatrix.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstflv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstadpcmenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideofiltersbad.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtpmanager.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgoom.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudioparsers.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideorate.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideofilter.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstwavpack.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvulkan.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsty4mdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsrtp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaiff.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsubenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstapetag.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaes.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideoframe_audiolevel.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudioconvert.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstbayer.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsticydemux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstfrei0r.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstjpegformat.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiobuffersplit.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtpmanagerbad.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstadpcmdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmpegpsdemux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstnle.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstbz2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiofx.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiorate.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideotestsrc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstogg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstfieldanalysis.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideobox.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstwavenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstamrnb.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstadaptivedemux2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrealmedia.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopenh264.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcoreelements.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsthls.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstwavparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrist.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstxingmux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdtsdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsoup.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstauparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvolume.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsttheora.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstkate.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtsp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstencoding.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsmoothstreaming.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstpango.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstid3tag.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiolatency.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsegmentclip.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcoloreffects.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstspeed.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmpg123.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtspclientsink.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsoundtouch.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstadder.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsttypefindfunctions.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstautoconvert.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstplayback.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudioresample.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstasfmux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiofxbad.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopus.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsttaglib.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstamrwbdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmpegtsmux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsta52dec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstwebrtc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmultipart.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdeinterlace.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsmpte.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsty4menc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstnetsim.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgeometrictransform.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtmp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopenjpeg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcodectimestamper.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideocrop.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdebugutilsbad.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcoretracers.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstassrender.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstzbar.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstladspa.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiovisualizers.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstx264.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstalphacolor.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstlibav.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstremovesilence.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstges.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgio.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstlevel.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgdp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstalpha.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstautodetect.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstinterleave.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvorbis.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstimagefreeze.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmxf.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideoconvertscale.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstspeex.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopengl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstivorbisdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstjpeg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstshapewipe.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstalaw.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstspandsp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdvdlpcmdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmultifile.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstreplaygain.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcamerabin.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstipcpipeline.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstinterlace.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrfbsrc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcutter.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsrt.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtmp2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmidi.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmulaw.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdebug.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstandroidmedia.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstlame.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmatroska.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcairo.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgaudieffects.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstpnm.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmpegtsdemux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsctp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstoverlaycomposition.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstflac.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdtls.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstwebrtcdsp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstpcapparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsttimecode.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsteffectv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstnice.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsiren.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstmpegpsmux.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiomixer.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdvdspu.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstspectrum.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrawparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideoparsersbad.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstcompositor.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstisomp4.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstinter.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgdkpixbuf.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstequalizer.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstlegacyrawparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdtmf.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstpng.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsmooth.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvpx.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstivtc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstasf.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsbc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstdash.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvideomixer.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgsttcp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstapp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaccurip.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstopensles.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstgoom2k1.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstfreeverb.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstrtp.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstsubparse.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstaudiotestsrc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstflxdec.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstvoaacenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gstreamer-1.0/libgstavi.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libfribidi.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgraphene-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstwebrtc-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcairo-gobject.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/liborc-0.4.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/librsvg-2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libSoundTouch.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libsqlite3.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libsrtp2.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstgl-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/liba52.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvorbisenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstpbutils-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcairo-script-interpreter.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgobject-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpixman-1.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libdv.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libogg.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libtiff.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libtag.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libsrt.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libnice.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstplay-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstallocators-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/gio/modules/libgioopenssl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgdk_pixbuf-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstbasecamerabinsrc-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libmpg123.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libswresample.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libz.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libsoup-2.4.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libFLAC.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgio-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libexpat.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvpx.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libavutil.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstadaptivedemux-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libvo-aacenc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libsbc.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libopencore-amrnb.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstrtspserver-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgsttag-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libpsl.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libass.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgthread-2.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libcharset.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstbase-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstcheck-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libavfilter.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstcontroller-1.0.a
${gstreamer_android_SOURCE_DIR}/arm64/lib/libturbojpeg.a


)

# ${GLIB_LIBRARIES}
# ${LIBSOUP_LIBRARIES}
# ${JSONGLIB_LIBRARIES}
# ${GIO_LIBRARIES}


# set(GLIB_LIBRARY_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/lib)
# set(LIBSOUP_LIBRARY_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/lib)
set(JSONGLIB_LIBRARIES ${gstreamer_android_SOURCE_DIR}/arm64/lib/libjson-glib-1.0.a)
# set(GIO_LIBRARY_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/lib)

# find_library(GLIB_LIBRARIES glib-2.0 HINTS ${GLIB_LIBRARY_DIRS})
# find_library(LIBSOUP_LIBRARIES soup-2.4 HINTS ${LIBSOUP_LIBRARY_DIRS})
# find_library(JSONGLIB_LIBRARIES json-glib-1.0 HINTS ${JSONGLIB_LIBRARY_DIRS})
# find_library(GIO_LIBRARIES gio-2.0 HINTS ${GIO_LIBRARY_DIRS})

set(GST_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/gstreamer-1.0/ )
set(GLIB_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/glib-2.0 ${gstreamer_android_SOURCE_DIR}/arm64/lib/glib-2.0/include/)
set(LIBSOUP_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/libsoup-2.4/)
set(JSONGLIB_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/json-glib-1.0)
set(GIO_INCLUDE_DIRS ${gstreamer_android_SOURCE_DIR}/arm64/include/glib-2.0/ ${gstreamer_android_SOURCE_DIR}/arm64/lib/gio-unix-2.0/)


# These two don't seem to help
include_directories(
		${GSTREAMER_ROOT}/include/gstreamer-1.0
		${GSTREAMER_ROOT}/include/glib-2.0
		${GSTREAMER_ROOT}/lib/glib-2.0/include)

link_directories(
		${GSTREAMER_ROOT}/lib
		${GSTREAMER_ROOT}/lib/gstreamer-1.0)

message(gstreamer_android_SOURCE_DIR)
message(GST_LIBRARIES)

add_subdirectory(src)
