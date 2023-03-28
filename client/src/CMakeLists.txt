cmake_minimum_required(VERSION 3.10.2)

project("xrgears")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_definitions(-DXR_OS_ANDROID)
add_definitions(-DXR_USE_PLATFORM_ANDROID)
# add_definitions(-DXR_USE_GRAPHICS_API_VULKAN)

find_package(Vulkan REQUIRED)
find_package(OpenXR REQUIRED)
# find_package(EGL REQUIRED)

add_library(openxr_loader SHARED IMPORTED)
set_property(
        TARGET
        openxr_loader
        PROPERTY
        IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
)


# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(xrgears SHARED
    main.c
)

target_include_directories(xrgears PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/include
)

# add lib dependencies
target_link_libraries(xrgears
    android
    OpenXR::openxr_loader
    Vulkan::Vulkan
    # OpenXR::headers
    # OpenXR::openxr_headers
    native_app_glue
    EGL
    GLESv3
    android
    log # This is an android thing
    openxr_loader
    
)

