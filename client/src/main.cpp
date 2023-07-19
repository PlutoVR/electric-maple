// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Main file for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */


#include "gst/app_log.h"
#include "gst/gst_common.h"
#include "render.hpp"
#include "pluto.pb.h"
#include "pb_encode.h"

#include "os/os_time.h"
#include "util/u_time.h"

#include <EGL/egl.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_activity.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <array>
#include <assert.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <jni.h>
#include <unistd.h>

// static GLuint global_data[1440*1584*4];

static int pfd[2];
static pthread_t thr;
static const char *tag = "ElectricMaple";

static void *
thread_func(void *)
{
	ssize_t rdsz;
	char buf[2048];
	while ((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
		if (buf[rdsz - 1] == '\n')
			--rdsz;
		buf[rdsz] = 0; /* add null-terminator */
		__android_log_write(ANDROID_LOG_DEBUG, tag, buf);
	}
	return 0;
}



int
start_logger(const char *app_name)
{
	tag = app_name;

	/* make stdout line-buffered and stderr unbuffered */
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IOLBF, 0);

	/* create the pipe and redirect stdout and stderr */
	pipe(pfd);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	/* spawn the logging thread */
	if (pthread_create(&thr, 0, thread_func, 0) == -1)
		return -1;
	pthread_detach(thr);
	return 0;
}



static state_t state = {};

static void
onAppCmd(struct android_app *app, int32_t cmd)
{
	switch (cmd) {
	case APP_CMD_START: ALOGE("APP_CMD_START"); break;
	case APP_CMD_RESUME: ALOGE("APP_CMD_RESUME"); break;
	case APP_CMD_PAUSE: ALOGE("APP_CMD_PAUSE"); break;
	case APP_CMD_STOP: ALOGE("APP_CMD_STOP"); break;
	case APP_CMD_DESTROY: ALOGE("APP_CMD_DESTROY"); break;
	case APP_CMD_INIT_WINDOW: ALOGE("APP_CMD_INIT_WINDOW"); break;
	case APP_CMD_TERM_WINDOW: ALOGE("APP_CMD_TERM_WINDOW"); break;
	}
}


void
sink_push_frame(struct xrt_frame_sink *xfs, struct xrt_frame *xf)
{
	ALOGE("sink_push_frame called!");
	if (!xf) {
		ALOGE("what??");
		return;
	}
	struct state_t *st = container_of(xfs, struct state_t, frame_sink);

	// This can cause a segfault if we hold onto one frame for too long so OH.
	if (!st->xf) {
		//		xrt_frame_reference(&xf, st->xf);
		xrt_frame_reference(&st->xf, xf);
	}
	ALOGE("Called! %d %p %u %u %zu", st->frame_texture_id, xf->data, xf->width, xf->height, xf->stride);
}

void
writeRandomTexture(GLsizei width, GLsizei height, int way, GLubyte *data)
{

	// Seed the random number generator
	//    std::srand(std::time(0));
	if (way == 0) {

		// Fill the texture data with random values
		for (GLsizei i = 0; i < width * height * 4; i++) {
			data[i] = std::rand() % 256;
		}
	} else if (way == 1) {

		int tileSize = 100;
		for (GLsizei y = 0; y < height; y++) {
			for (GLsizei x = 0; x < width; x++) {
				GLsizei tileX = x / tileSize;
				GLsizei tileY = y / tileSize;

				GLubyte color = ((tileX + tileY) % 2 == 0) ? 255 : 0;
				GLsizei index = (y * width + x) * 4;

				data[index] = color;
				data[index + 1] = color;
				data[index + 2] = color;
				data[index + 3] = 255;
			}
		}
	} else {
		// Define the colors for the rainbow checkerboard
		std::array<GLubyte[4], 7> colors = {{
		    {255, 0, 0, 255},   // Red
		    {255, 127, 0, 255}, // Orange
		    {255, 255, 0, 255}, // Yellow
		    {0, 255, 0, 255},   // Green
		    {0, 0, 255, 255},   // Blue
		    {75, 0, 130, 255},  // Indigo
		    {148, 0, 211, 255}  // Violet
		}};
		int tileSize = 100;
		for (GLsizei y = 0; y < height; y++) {
			for (GLsizei x = 0; x < width; x++) {
				GLsizei tileX = x / tileSize;
				GLsizei tileY = y / tileSize;

				GLsizei tileIndex = (tileX + tileY) % colors.size();
				GLsizei index = (y * width + x) * 4;

				data[index] = colors[tileIndex][0];
				data[index + 1] = colors[tileIndex][1];
				data[index + 2] = colors[tileIndex][2];
				data[index + 3] = colors[tileIndex][3];
			}
		}
	}
}

GLuint
generateRandomTexture(GLsizei width, GLsizei height, int way)
{
	// Allocate memory for the texture data
	GLubyte *data = new GLubyte[width * height * 4];

	writeRandomTexture(width, height, way, data);
	// Create the texture
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);

	// Free the texture data
	delete[] data;

	return texture;
}


void
generateRandomTextureOld(GLsizei width, GLsizei height, int way, GLuint *handle)
{


	// Allocate memory for the texture data
	GLubyte *data = new GLubyte[width * height * 4];

	writeRandomTexture(width, height, way, data);
	glBindTexture(GL_TEXTURE_2D, *handle);
	// This call is definitely what's crashing, which is real confusing.
	ALOGE("%d %d", width, height);
#if 0
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Set to 1 for tightly packed data

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

#endif
	glBindTexture(GL_TEXTURE_2D, 0);

	// Free the texture data
	delete[] data;
}

void
die_errno()
{
	ALOGE("Something happened! %s", strerror(errno));
}


void
really_make_socket(struct state_t &st)
{
#if 1
	// We shouldn't be using SOCK_STREAM! We're relying on blind luck to get synced packets

	st.socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
	// Doesn't work :(
	st.socket_fd = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
#endif
	ALOGE("Socket fd is %d, errno is %s", st.socket_fd, strerror(errno));

	// Connect to the parent process
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;

	// todo: use inet_pton
	serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serverAddr.sin_port = htons(61943);

	int iResult = connect(st.socket_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

	if (iResult < 0) {
		die_errno();
	}

	ALOGE("really_make_socket: Result is %d", iResult);
}


static void
hmd_pose(struct state_t &st)
{
	return;
	XrResult result = XR_SUCCESS;


	XrSpaceLocation hmdLocalLocation = {};
	hmdLocalLocation.type = XR_TYPE_SPACE_LOCATION;
	hmdLocalLocation.next = NULL;
	result = xrLocateSpace(st.viewSpace, st.worldSpace, os_monotonic_get_ns(), &hmdLocalLocation);
	if (result != XR_SUCCESS) {
		ALOGE("Bad!");
	}

	XrPosef hmdLocalPose = hmdLocalLocation.pose;

	pluto_TrackingMessage message = pluto_TrackingMessage_init_default;

	message.has_P_localSpace_viewSpace = true;
	message.P_localSpace_viewSpace.has_position = true;
	message.P_localSpace_viewSpace.has_orientation = true;
	message.P_localSpace_viewSpace.position.x = hmdLocalPose.position.x;
	message.P_localSpace_viewSpace.position.y = hmdLocalPose.position.y;
	message.P_localSpace_viewSpace.position.z = hmdLocalPose.position.z;

	message.P_localSpace_viewSpace.orientation.w = hmdLocalPose.orientation.w;
	message.P_localSpace_viewSpace.orientation.x = hmdLocalPose.orientation.x;
	message.P_localSpace_viewSpace.orientation.y = hmdLocalPose.orientation.y;
	message.P_localSpace_viewSpace.orientation.z = hmdLocalPose.orientation.z;

	uint8_t buffer[8192];



	pb_ostream_t os = pb_ostream_from_buffer(buffer, sizeof(buffer));

	pb_encode(&os, pluto_TrackingMessage_fields, &message);

	int iResult = send(st.socket_fd, buffer, pluto_TrackingMessage_size, 0);

	if (iResult <= 0) {
		ALOGE("BAD! %d %s", iResult, strerror(errno));
		die_errno();
	}
}

void
create_spaces(struct state_t &st)
{

	XrResult result = XR_SUCCESS;

	XrReferenceSpaceCreateInfo spaceInfo = {.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
	                                        .referenceSpaceType =
	                                            XR_REFERENCE_SPACE_TYPE_LOCAL, // XR_REFERENCE_SPACE_TYPE_STAGE,
	                                        .poseInReferenceSpace = {{0.f, 0.f, 0.f, 1.f}, {0.f, 0.f, 0.f}}};


	result = xrCreateReferenceSpace(state.session, &spaceInfo, &state.worldSpace);

	if (XR_FAILED(result)) {
		ALOGE("Failed to create reference space (%d)", result);
	}
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;

	result = xrCreateReferenceSpace(state.session, &spaceInfo, &state.viewSpace);

	if (XR_FAILED(result)) {
		ALOGE("Failed to create reference space (%d)", result);
	}
}


void
mainloop_one(struct state_t &state)
{


	// Poll Android events

	for (;;) {
		int events;
		struct android_poll_source *source;
		bool wait = !state.app->window || state.app->activityState != APP_CMD_RESUME;
		int timeout = wait ? -1 : 0;
		if (ALooper_pollAll(timeout, NULL, &events, (void **)&source) >= 0) {
			if (source) {
				source->process(state.app, source);
			}

			if (timeout == 0 && (!state.app->window || state.app->activityState != APP_CMD_RESUME)) {
				break;
			}
		} else {
			break;
		}
	}

	// Poll OpenXR events
	XrResult result = XR_SUCCESS;
	XrEventDataBuffer buffer;
	buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
	buffer.next = NULL;

	while (xrPollEvent(state.instance, &buffer) == XR_SUCCESS) {
		if (buffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
			XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&buffer;

			switch (event->state) {
			case XR_SESSION_STATE_IDLE: ALOGI("OpenXR session is now IDLE"); break;
			case XR_SESSION_STATE_READY: {
				ALOGI("OpenXR session is now READY, beginning session");
				XrSessionBeginInfo beginInfo = {};
				beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
				beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

				result = xrBeginSession(state.session, &beginInfo);

				if (XR_FAILED(result)) {
					ALOGI("Failed to begin OpenXR session (%d)", result);
				}
			} break;
			case XR_SESSION_STATE_SYNCHRONIZED: ALOGI("OpenXR session is now SYNCHRONIZED"); break;
			case XR_SESSION_STATE_VISIBLE: ALOGI("OpenXR session is now VISIBLE"); break;
			case XR_SESSION_STATE_FOCUSED: ALOGI("OpenXR session is now FOCUSED"); break;
			case XR_SESSION_STATE_STOPPING:
				ALOGI("OpenXR session is now STOPPING");
				xrEndSession(state.session);
				break;
			case XR_SESSION_STATE_LOSS_PENDING: ALOGI("OpenXR session is now LOSS_PENDING"); break;
			case XR_SESSION_STATE_EXITING: ALOGI("OpenXR session is now EXITING"); break;
			default: break;
			}

			state.sessionState = event->state;
		}

		buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
	}

	// If session isn't ready, return. We'll be called again and will poll events again.
	if (state.sessionState < XR_SESSION_STATE_READY) {
		ALOGI("Waiting for session ready state!");
		os_nanosleep(U_TIME_1MS_IN_NS * 100);
		return;
	}

	// Begin frame

	XrFrameState frameState = {.type = XR_TYPE_FRAME_STATE};

	result = xrWaitFrame(state.session, NULL, &frameState);

	if (XR_FAILED(result)) {
		ALOGE("xrWaitFrame failed");
	}

	XrFrameBeginInfo beginfo = {.type = XR_TYPE_FRAME_BEGIN_INFO};

	result = xrBeginFrame(state.session, &beginfo);

	if (XR_FAILED(result)) {
		ALOGE("xrBeginFrame failed");
	}

	// Locate views, set up layers
	XrView views[2] = {};
	views[0].type = XR_TYPE_VIEW;
	views[1].type = XR_TYPE_VIEW;


	XrViewLocateInfo locateInfo = {.type = XR_TYPE_VIEW_LOCATE_INFO,
	                               .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
	                               .displayTime = frameState.predictedDisplayTime,
	                               .space = state.worldSpace};

	XrViewState viewState = {.type = XR_TYPE_VIEW_STATE};

	uint32_t viewCount = 2;
	result = xrLocateViews(state.session, &locateInfo, &viewState, 2, &viewCount, views);

	if (XR_FAILED(result)) {
		ALOGE("Failed to locate views");
	}

	int width = state.width;
	int height = state.height;
	XrCompositionLayerProjection layer = {};
	layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	layer.space = state.worldSpace;
	layer.viewCount = 2;

	XrCompositionLayerProjectionView projectionViews[2] = {};
	projectionViews[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
	projectionViews[0].subImage.swapchain = state.swapchain;
	projectionViews[0].subImage.imageRect.offset = {0, 0};
	projectionViews[0].subImage.imageRect.extent = {width, height};
	projectionViews[0].pose = views[0].pose;
	projectionViews[0].fov = views[0].fov;

	projectionViews[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
	projectionViews[1].subImage.swapchain = state.swapchain;
	projectionViews[1].subImage.imageRect.offset = {width, 0};
	projectionViews[1].subImage.imageRect.extent = {width, height};
	projectionViews[1].pose = views[1].pose;
	projectionViews[1].fov = views[1].fov;

	layer.views = projectionViews;

	// Render

	uint32_t imageIndex;
	result = xrAcquireSwapchainImage(state.swapchain, NULL, &imageIndex);

	if (XR_FAILED(result)) {
		ALOGE("Failed to acquire swapchain image (%d)", result);
	}

	XrSwapchainImageWaitInfo waitInfo = {.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
	                                     .timeout = XR_INFINITE_DURATION};

	result = xrWaitSwapchainImage(state.swapchain, &waitInfo);

	if (XR_FAILED(result)) {
		ALOGE("Failed to wait for swapchain image (%d)", result);
	}

	// ALOGI("mainloop_one: Making EGL context current");
	EGLBoolean bret = eglMakeCurrent( //
	    state.display,                //
	    EGL_NO_SURFACE,               //
	    EGL_NO_SURFACE,               //
	    state.context);               //
	if (!bret) {
		ALOGE("eglMakeCurrent(Make): false, %u", eglGetError());
	}

	if (state.xf) {
		//	if (false) {

		//            for (int y = 0; y < state.xf->height; y++) {
		//                for (int x = 0; x < state.xf->width; x++) {
		//                    const uint8_t *src = state.xf->data;
		//                    uint8_t *dst = (uint8_t*)global_data;
		//
		//                    src = src + (y * state.xf->stride ) + (x*4);
		//                    dst = dst + (y * width*4) + (x * 4);
		//                    dst[0] = src[0];
		//                    dst[1] = src[1];
		//                    dst[2] = src[2];
		//                    dst[3] = src[3];
		//
		//                }
		//
		//            }


		// FIXME: This will go away now that we have a GL_TEXTURE_EXTERNAL texture
		ALOGE("DEBUG: Binding textures!");
		glBindTexture(GL_TEXTURE_2D, state.frame_texture_id);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 1440);
		//            glPixelStorei(GL_UNPACK_ALIGNMENT, 2); // Set to 1 for tightly packed data
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1440, 1584, GL_RGBA, GL_UNSIGNED_BYTE, state.xf->data);

		//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, state.xf->width, state.xf->height, 0, GL_RGBA,
		// GL_UNSIGNED_BYTE, 		             state.xf->data);

		//        glPixelStorei(GL_UNPACK_ROW_LENGTH, state.xf->stride / 4);
		//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state.xf->width, state.xf->height, 0, GL_RGBA,
		//    GL_UNSIGNED_BYTE, state.xf->data);
		glBindTexture(GL_TEXTURE_2D, 0);

		//            delete[] data;
	}

	// can be factored into the above, it's just useful to be able to disable seperately
	if (state.xf) {
		xrt_frame_reference(&state.xf, NULL);
	}

	// ALOGE("DEBUG: Binding framebuffer\n");
	glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[imageIndex]);

	glViewport(0, 0, state.width * 2, state.height);


	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Just display purple nothingness
	// ALOGE("DEBUG: DRAWING!\n");
	for (uint32_t eye = 0; eye < 2; eye++) {
		glViewport(eye * state.width, 0, state.width, state.height);
		draw(state.framebuffers[imageIndex], state.frame_texture_id);
	}

	// Release

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	xrReleaseSwapchainImage(state.swapchain, NULL);

	// Submit frame


	XrFrameEndInfo endInfo = {};
	endInfo.type = XR_TYPE_FRAME_END_INFO;
	endInfo.displayTime = frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	endInfo.layerCount = frameState.shouldRender ? 1 : 0;
	endInfo.layers = (const XrCompositionLayerBaseHeader *[1]){(XrCompositionLayerBaseHeader *)&layer};

	hmd_pose(state);

	xrEndFrame(state.session, &endInfo);

	bret = eglMakeCurrent( //
	    state.display,     //
	    EGL_NO_SURFACE,    //
	    EGL_NO_SURFACE,    //
	    EGL_NO_CONTEXT);   //
	if (bret != EGL_TRUE) {
		ALOGE("eglMakeCurrent(Un-make): false, %u", eglGetError());
	}
}

#if 0
const char *
getFilePath(JNIEnv *env, jobject activity)
{
	jclass contextClass = env->GetObjectClass(activity);
	jmethodID getExternalFilesDirMethod =
	    env->GetMethodID(contextClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
	jstring directoryName = env->NewStringUTF("debug");
	jobject directory = env->CallObjectMethod(activity, getExternalFilesDirMethod, directoryName);
	env->DeleteLocalRef(directoryName);
	env->DeleteLocalRef(contextClass);
	if (directory == nullptr) {
		return env->NewStringUTF("Failed to obtain external files directory");
	}
	jmethodID getPathMethod = env->GetMethodID(env->FindClass("java/io/File"), "getPath", "()Ljava/lang/String;");
	jstring path = static_cast<jstring>(env->CallObjectMethod(directory, getPathMethod));
	env->DeleteLocalRef(directory);
	if (path == nullptr) {
		return env->NewStringUTF("Failed to obtain path to external files directory");
	}
	std::string filePath = env->GetStringUTFChars(path, nullptr);
	filePath += "/meow.dot";
	jstring result = env->NewStringUTF(filePath.c_str());
	env->ReleaseStringUTFChars(path, filePath.c_str());
	return result;
}
#else
extern "C" const char *
getFilePath(JNIEnv *env, jobject activity)
{
	jclass contextClass = env->GetObjectClass(activity);
	jmethodID getExternalFilesDirMethod =
	    env->GetMethodID(contextClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
	jstring directoryName = env->NewStringUTF("debug");
	jobject directory = env->CallObjectMethod(activity, getExternalFilesDirMethod, directoryName);
	env->DeleteLocalRef(directoryName);
	env->DeleteLocalRef(contextClass);
	if (directory == nullptr) {
		return strdup("Failed to obtain external files directory");
	}
	jmethodID getPathMethod = env->GetMethodID(env->FindClass("java/io/File"), "getPath", "()Ljava/lang/String;");
	jstring path = static_cast<jstring>(env->CallObjectMethod(directory, getPathMethod));
	env->DeleteLocalRef(directory);
	if (path == nullptr) {
		return strdup("Failed to obtain path to external files directory");
	}
	const char *filePath = env->GetStringUTFChars(path, nullptr);
	env->DeleteLocalRef(path);
	return filePath;
}
#endif

void
android_main(struct android_app *app)
{
	start_logger("ElectricMaple");
	// setenv("GST_DEBUG", "*:3", 1);
	// setenv("GST_DEBUG", "*ssl*:9,*tls*:9,*webrtc*:9", 1);
	setenv("GST_DEBUG", "*:2,*CAPS*:6,amcvideodec:7", 1);

	state.app = app;
	state.java_vm = app->activity->vm;

	(*app->activity->vm).AttachCurrentThread(&state.jni, NULL);
	app->onAppCmd = onAppCmd;

	initializeEGL(state);

	/*  ANativeActivity *activity = app->activity;
	  JNIEnv* env = state.jni;
	  jclass clazz = env->FindClass("android/Manifest$permission");
	  jfieldID field = env->GetStaticFieldID(clazz, "WRITE_EXTERNAL_STORAGE", "Ljava/lang/String;");
	  jstring permissionString = (jstring) env->GetStaticObjectField(clazz, field);
	  jint permission = env->CallIntMethod(activity->clazz, env->GetMethodID(clazz, "checkSelfPermission",
	  "(Ljava/lang/String;)I"), permissionString);

	  if (permission != PackageManager.PERMISSION_GRANTED) {
	      env->CallVoidMethod(activity->clazz, env->GetMethodID(clazz, "requestPermissions",
	  "([Ljava/lang/String;I)V"), env->NewObjectArray(1, env->FindClass("java/lang/String"), permissionString), 1);
	  }*/

	state.xf = nullptr;

	// Initialize OpenXR loader

	PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = NULL;
	XR_LOAD(xrInitializeLoaderKHR);
	XrLoaderInitInfoAndroidKHR loaderInfo = {
	    .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
	    .applicationVM = app->activity->vm,
	    .applicationContext = app->activity->clazz,
	};

	XrResult result = xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&loaderInfo);

	if (XR_FAILED(result)) {
		ALOGE("Failed to initialize OpenXR loader\n");
	}

	// Create OpenXR instance

	const char *extensions[] = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
	                            XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};

	XrInstanceCreateInfoAndroidKHR androidInfo = {};
	androidInfo.type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR;
	androidInfo.applicationActivity = app->activity->clazz;
	androidInfo.applicationVM = app->activity->vm;

	XrInstanceCreateInfo instanceInfo = {};
	instanceInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.next = &androidInfo;

	strncpy(instanceInfo.applicationInfo.engineName, "N/A", XR_MAX_APPLICATION_NAME_SIZE - 1);
	instanceInfo.applicationInfo.engineName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';

	strncpy(instanceInfo.applicationInfo.applicationName, "N/A", XR_MAX_APPLICATION_NAME_SIZE - 1);
	instanceInfo.applicationInfo.applicationName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';

	instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	instanceInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
	instanceInfo.enabledExtensionNames = extensions;


	result = xrCreateInstance(&instanceInfo, &state.instance);

	if (XR_FAILED(result)) {
		ALOGE("Failed to initialize OpenXR instance\n");
	}

	// OpenXR system

	XrSystemGetInfo systemInfo = {.type = XR_TYPE_SYSTEM_GET_INFO,
	                              .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};

	result = xrGetSystem(state.instance, &systemInfo, &state.system);

	uint32_t viewConfigurationCount;
	XrViewConfigurationType viewConfigurations[2];
	result =
	    xrEnumerateViewConfigurations(state.instance, state.system, 2, &viewConfigurationCount, viewConfigurations);

	if (XR_FAILED(result)) {
		ALOGE("Failed to enumerate view configurations\n");
	}


	XrViewConfigurationView viewInfo[2] = {};
	viewInfo[0].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
	viewInfo[1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

	uint32_t viewCount = 0;
	xrEnumerateViewConfigurationViews(state.instance, state.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0,
	                                  &viewCount, NULL);
	result = xrEnumerateViewConfigurationViews(state.instance, state.system,
	                                           XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, viewInfo);

	if (XR_FAILED(result) || viewCount != 2) {
		ALOGE("Failed to enumerate view configuration views\n");
		return;
	}

	state.width = viewInfo[0].recommendedImageRectWidth;
	state.height = viewInfo[0].recommendedImageRectHeight;

	state.frame_texture_id = generateRandomTexture(state.width, state.height, 2);
	//    state.frame_tex = generateRandomTexture(320, 240);

	state.frame_sink.push_frame = sink_push_frame;

	struct xrt_frame_context xfctx = {};
	struct xrt_fs *client = em_fs_create_streaming_client(&xfctx, &state, state.display, state.context);
	ALOGE("Done creating videotestsrc\n");

	// OpenXR session

	PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR = NULL;
	XR_LOAD(xrGetOpenGLESGraphicsRequirementsKHR);
	XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
	xrGetOpenGLESGraphicsRequirementsKHR(state.instance, state.system, &graphicsRequirements);

	XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
	                                                       .display = state.display,
	                                                       .config = state.config,
	                                                       .context = state.context};

	XrSessionCreateInfo sessionInfo = {
	    .type = XR_TYPE_SESSION_CREATE_INFO, .next = &graphicsBinding, .systemId = state.system};

	result = xrCreateSession(state.instance, &sessionInfo, &state.session);

	if (XR_FAILED(result)) {
		ALOGE("Failed to create OpenXR session (%d)\n", result);
	}

	// OpenXR swapchain
	XrSwapchainCreateInfo swapchainInfo = {};
	swapchainInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
	swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.format = GL_SRGB8_ALPHA8;
	swapchainInfo.width = state.width * 2;
	swapchainInfo.height = state.height;
	swapchainInfo.sampleCount = 1;
	swapchainInfo.faceCount = 1;
	swapchainInfo.arraySize = 1;
	swapchainInfo.mipCount = 1;

	result = xrCreateSwapchain(state.session, &swapchainInfo, &state.swapchain);

	if (XR_FAILED(result)) {
		ALOGE("Failed to create OpenXR swapchain (%d)\n", result);
	}

	for (uint32_t i = 0; i < 4; i++) {
		state.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
	}

	// TODO if the swapchain has more than 4 images this will error and we aren't checking it
	xrEnumerateSwapchainImages(state.swapchain, 4, &state.imageCount, (XrSwapchainImageBaseHeader *)state.images);

	if (XR_FAILED(result)) {
		ALOGE("Failed to get swapchain images (%d)\n", result);
	}

	glGenFramebuffers(state.imageCount, state.framebuffers);


	// if there is a frame: we probably just made one above
	if (state.xf) {
		ALOGI("state.xf is non-null");
		glBindTexture(GL_TEXTURE_2D, state.frame_texture_id);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, state.xf->stride / 4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state.xf->width, state.xf->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		             state.xf->data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	for (uint32_t i = 0; i < state.imageCount; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.images[i].image, 0);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			ALOGE("Failed to create framebuffer (%d)\n", status);
		}
	}


	ALOGE("DEBUG: Create spaces\n");
	create_spaces(state);

	ALOGE("DEBUG: Setup Render\n");
	setupRender();

	eglMakeCurrent(state.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	ALOGE("DEBUG: Un-make EGL context current\n");


	ALOGE("Starting source\n");
	// FIXME: Eventually completely remove XRT_FS dependency here for driving the gst pipeline look
	xrt_fs_stream_start(client, &state.frame_sink, XRT_FS_CAPTURE_TYPE_TRACKING, 0);
	ALOGE("Done starting source\n");

	ALOGE("DEBUG: Really make socket\n");
	really_make_socket(state);


	// Mainloop
	ALOGE("DEBUG: Starting main loop.\n");
	while (!app->destroyRequested) {
		mainloop_one(state);
	}

	(*app->activity->vm).DetachCurrentThread();
}
