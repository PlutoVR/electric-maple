// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup drv_vf
 */

#pragma once

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include "xrt/xrt_frame.h"
#include "util/u_logging.h"
#include "util/u_time.h"
#include "os/os_time.h"

#include <android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>

// FIXME: THE BELOW ARE UGLY !?
#include "../../../../../../../.gradle/caches/transforms-3/0ea571ec8b0b3b9231cb793af03e2479/transformed/jetified-openxr_loader_for_android-1.0.20/prefab/modules/openxr_loader/include/openxr/openxr.h"
#include "../../../../../../../.gradle/caches/transforms-3/0ea571ec8b0b3b9231cb793af03e2479/transformed/jetified-openxr_loader_for_android-1.0.20/prefab/modules/openxr_loader/include/openxr/openxr_platform.h"

#define XR_LOAD(fn) xrGetInstanceProcAddr(state.instance, #fn, (PFN_xrVoidFunction *)&fn);

#include "../../../../proto/generated/pluto.pb.h"
#include "../../../../monado/src/external/nanopb/pb_encode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "xrt/xrt_frameserver.h"
// FIXME: set relative path through include-dir
#include "/home/fredinfinite23/code/PlutoVR/linux-streaming-CLIENT2/monado/src/xrt/auxiliary/gstreamer/gstjniutils.h"

struct state_t
{
    struct android_app *app;
    JNIEnv *jni;
    JavaVM *java_vm;
    bool hasPermissions;
    // ANativeWindow *window; // Are we going to need this ?
    EGLDisplay display;
    EGLContext context;
    EGLConfig config;
    EGLSurface surface;
    XrInstance instance;
    XrSystemId system;
    XrSession session;
    XrSessionState sessionState;
    XrSpace worldSpace;
    XrSpace viewSpace;
    XrSwapchain swapchain;
    XrSwapchainImageOpenGLESKHR images[4];
    GLuint framebuffers[4];
    GLuint shader_program;
    uint32_t imageCount;
    uint32_t width;
    uint32_t height;

    int socket_fd;
    struct sockaddr_in socket_addr;

    // this is bad, we want an xrt_frame_node etc.

    int way;

    struct xrt_frame_sink frame_sink;
    struct xrt_frame *xf;
    // this is the GL texture id used by the main renderer. This is what gst/ code should also be using.
    GLuint frame_texture_id;
    GLenum frame_texture_target;
    GLboolean frame_available;
};

#ifdef __cplusplus
extern "C" {
#endif

static const char *dotfilepath = "mrow";

/*!
 * @defgroup drv_vf Video Fileframeserver driver
 * @ingroup drv
 *
 * @brief Frameserver using a video file.
 */

/*!
 * Create a vf frameserver by opening a video file.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_open_file(struct xrt_frame_context *xfctx, const char *path);

/*!
 * Create a vf frameserver that uses the videotestsource.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_videotestsource(struct xrt_frame_context *xfctx, struct state_t *state);


#ifdef __cplusplus
}
#endif
