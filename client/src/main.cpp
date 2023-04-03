// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Main file for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */


#include <errno.h>

#include <assert.h>

#include "common.hpp"

#include "unimportant_triangle.hpp"

static state_t state = {};

static void
onAppCmd(struct android_app *app, int32_t cmd);

static void
initializeEGL(void);


void
die_errno()
{
	U_LOG_E("Something happened! %s", strerror(errno));
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
	U_LOG_E("Socket fd is %d, errno is %s", st.socket_fd, strerror(errno));

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

	U_LOG_E("really_make_socket: Result is %d", iResult);
}


static void
hmd_pose(struct state_t &st)
{

	XrResult result = XR_SUCCESS;


	XrSpaceLocation hmdLocalLocation = {};
	hmdLocalLocation.type = XR_TYPE_SPACE_LOCATION;
	hmdLocalLocation.next = NULL;
	result = xrLocateSpace(st.viewSpace, st.worldSpace, os_monotonic_get_ns(), &hmdLocalLocation);
	if (result != XR_SUCCESS) {
		U_LOG_E("Bad!");
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
		U_LOG_E("BAD! %d %s", iResult, strerror(errno));
		die_errno();
	}
}

void
create_spaces(struct state_t &st)
{

	XrResult result = XR_SUCCESS;

	XrReferenceSpaceCreateInfo spaceInfo = {.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
	                                        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
	                                        .poseInReferenceSpace = {{0.f, 0.f, 0.f, 1.f}, {0.f, 0.f, 0.f}}};


	result = xrCreateReferenceSpace(state.session, &spaceInfo, &state.worldSpace);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to create reference space (%d)", result);
	}
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;

	result = xrCreateReferenceSpace(state.session, &spaceInfo, &state.viewSpace);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to create reference space (%d)", result);
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
			case XR_SESSION_STATE_IDLE: U_LOG_E("OpenXR session is now IDLE"); break;
			case XR_SESSION_STATE_READY: U_LOG_E("OpenXR session is now READY, beginning session");

#if 0
                        result = xrBeginSession(state.session, &(XrSessionBeginInfo) {
                                .type = XR_TYPE_SESSION_BEGIN_INFO,
                                .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
                        });
#else
				{
					XrSessionBeginInfo beginInfo = {};
					beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
					beginInfo.primaryViewConfigurationType =
					    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

					result = xrBeginSession(state.session, &beginInfo);
				}
#endif

				if (XR_FAILED(result)) {
					U_LOG_E("Failed to begin OpenXR session (%d)", result);
				}

				break;
			case XR_SESSION_STATE_SYNCHRONIZED: U_LOG_E("OpenXR session is now SYNCHRONIZED"); break;
			case XR_SESSION_STATE_VISIBLE: U_LOG_E("OpenXR session is now VISIBLE"); break;
			case XR_SESSION_STATE_FOCUSED: U_LOG_E("OpenXR session is now FOCUSED"); break;
			case XR_SESSION_STATE_STOPPING:
				U_LOG_E("OpenXR session is now STOPPING");
				xrEndSession(state.session);
				break;
			case XR_SESSION_STATE_LOSS_PENDING: U_LOG_E("OpenXR session is now LOSS_PENDING"); break;
			case XR_SESSION_STATE_EXITING: U_LOG_E("OpenXR session is now EXITING"); break;
			default: break;
			}

			state.sessionState = event->state;
		}

		buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
	}

	// Spin until session is ready
	if (state.sessionState < XR_SESSION_STATE_READY) {
		U_LOG_E("Waiting!");
		os_nanosleep(U_TIME_1MS_IN_NS * 100);
		return;
	}

	// Begin frame

	XrFrameState frameState = {.type = XR_TYPE_FRAME_STATE};


	result = xrWaitFrame(state.session, NULL, &frameState);

	if (XR_FAILED(result)) {
		U_LOG_E("xrWaitFrame failed");
	}

	XrFrameBeginInfo beginfo = {.type = XR_TYPE_FRAME_BEGIN_INFO};

	result = xrBeginFrame(state.session, &beginfo);

	if (XR_FAILED(result)) {
		U_LOG_E("xrBeginFrame failed");
	}

	// Locate views, set up layers

#if 0
        XrView views[2] = {
                [0].type = XR_TYPE_VIEW,
                [1].type = XR_TYPE_VIEW
        };
#else
	XrView views[2] = {};
	views[0].type = XR_TYPE_VIEW;
	views[1].type = XR_TYPE_VIEW;
#endif


	XrViewLocateInfo locateInfo = {.type = XR_TYPE_VIEW_LOCATE_INFO,
	                               .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
	                               .displayTime = frameState.predictedDisplayTime,
	                               .space = state.worldSpace};

	XrViewState viewState = {.type = XR_TYPE_VIEW_STATE};

	uint32_t viewCount = 2;
	result = xrLocateViews(state.session, &locateInfo, &viewState, 2, &viewCount, views);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to locate views");
	}

#if 0
        XrCompositionLayerProjection layer = {
                .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
                .space = state.worldSpace,
                .viewCount = 2,
                .views = (XrCompositionLayerProjectionView[2]) {
                        {
                                .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                                .subImage = {state.swapchain,
                                             {{0, 0}, {state.width, state.height}}},
                                .pose = views[0].pose,
                                .fov = views[0].fov
                        },
                        {
                                .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                                .subImage = {state.swapchain,
                                             {{state.width, 0}, {state.width, state.height}}},
                                .pose = views[1].pose,
                                .fov = views[1].fov
                        }
                }
        };
#else
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
#endif


	// Render

	// ??
	frameState.shouldRender = true;

	if (frameState.shouldRender) {
		uint32_t imageIndex;
		result = xrAcquireSwapchainImage(state.swapchain, NULL, &imageIndex);

		if (XR_FAILED(result)) {
			U_LOG_E("Failed to acquire swapchain image (%d)", result);
		}

		XrSwapchainImageWaitInfo waitInfo = {.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
		                                     .timeout = XR_INFINITE_DURATION};

		result = xrWaitSwapchainImage(state.swapchain, &waitInfo);

		if (XR_FAILED(result)) {
			U_LOG_E("Failed to wait for swapchain image (%d)", result);
		}


		glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[imageIndex]);

		// Just display purple nothingness
		for (uint32_t eye = 0; eye < 2; eye++) {
			glViewport(eye * state.width, 0, state.width, state.height);
			drawTriangle(state.shader_program);
		}

		// Release

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		xrReleaseSwapchainImage(state.swapchain, NULL);
	}

	// Submit frame

#if 0
        XrFrameEndInfo endInfo = {
                .type = XR_TYPE_FRAME_END_INFO,
                .displayTime = frameState.predictedDisplayTime,
                .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                .layerCount = frameState.shouldRender,
                .layers = (const XrCompositionLayerBaseHeader *[1]) {
                        (XrCompositionLayerBaseHeader *) &layer
                }
        };
#else
	XrFrameEndInfo endInfo = {};
	endInfo.type = XR_TYPE_FRAME_END_INFO;
	endInfo.displayTime = frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	endInfo.layerCount = frameState.shouldRender ? 1 : 0;
	endInfo.layers = (const XrCompositionLayerBaseHeader *[1]){(XrCompositionLayerBaseHeader *)&layer};
#endif

	hmd_pose(state);

	xrEndFrame(state.session, &endInfo);
}

void
android_main(struct android_app *app)
{

	state.app = app;
	(*app->activity->vm).AttachCurrentThread(&state.jni, NULL);
	app->onAppCmd = onAppCmd;

	initializeEGL();



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
		U_LOG_E("Failed to initialize OpenXR loader");
	}

	// Create OpenXR instance

	const char *extensions[] = {"XR_KHR_opengl_es_enable"};

#if 0
    XrInstanceCreateInfo instanceInfo = {
            .type = XR_TYPE_INSTANCE_CREATE_INFO,
            .applicationInfo.engineName = "N/A",
            .applicationInfo.applicationName = "N/A",
            .applicationInfo.apiVersion = XR_CURRENT_API_VERSION,
            .enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
            .enabledExtensionNames = extensions
    };
#else
	XrInstanceCreateInfo instanceInfo = {};
	instanceInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.next = nullptr;

	strncpy(instanceInfo.applicationInfo.engineName, "N/A", XR_MAX_APPLICATION_NAME_SIZE - 1);
	instanceInfo.applicationInfo.engineName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';

	strncpy(instanceInfo.applicationInfo.applicationName, "N/A", XR_MAX_APPLICATION_NAME_SIZE - 1);
	instanceInfo.applicationInfo.applicationName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';

	instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	instanceInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
	instanceInfo.enabledExtensionNames = extensions;
#endif


	result = xrCreateInstance(&instanceInfo, &state.instance);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to initialize OpenXR instance");
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
		U_LOG_E("Failed to enumerate view configurations");
	}

	uint32_t viewCount = 0;
#if 0
    XrViewConfigurationView viewInfo[2] = {
            [0].type = XR_TYPE_VIEW_CONFIGURATION_VIEW,
            [1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW
    };
#else
	XrViewConfigurationView viewInfo[2] = {};
	viewInfo[0].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
	viewInfo[1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
#endif


	xrEnumerateViewConfigurationViews(state.instance, state.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0,
	                                  &viewCount, NULL);
	result = xrEnumerateViewConfigurationViews(state.instance, state.system,
	                                           XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, viewInfo);

	if (XR_FAILED(result) || viewCount != 2) {
		U_LOG_E("Failed to enumerate view configuration views");
		return;
	}

	state.width = viewInfo[0].recommendedImageRectWidth;
	state.height = viewInfo[0].recommendedImageRectHeight;

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
		U_LOG_E("Failed to create OpenXR session (%d)", result);
	}

	// OpenXR swapchain

#if 0
    XrSwapchainCreateInfo swapchainInfo = {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
            .format = GL_SRGB8_ALPHA8,
            .width = state.width * 2,
            .height = state.height,
            .sampleCount = 1,
            .faceCount = 1,
            .arraySize = 1,
            .mipCount = 1
    };
#else
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
#endif

	result = xrCreateSwapchain(state.session, &swapchainInfo, &state.swapchain);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to create OpenXR swapchain (%d)", result);
	}

	for (uint32_t i = 0; i < 4; i++) {
		state.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
	}

	xrEnumerateSwapchainImages(state.swapchain, 4, &state.imageCount, (XrSwapchainImageBaseHeader *)state.images);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to get swapchain images (%d)", result);
	}

	glGenFramebuffers(state.imageCount, state.framebuffers);

	for (uint32_t i = 0; i < state.imageCount; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.images[i].image, 0);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			U_LOG_E("Failed to create framebuffer (%d)", status);
		}
	}


	create_spaces(state);

	state.shader_program = make_program();

	really_make_socket(state);


	// Mainloop
	while (!app->destroyRequested) {
		mainloop_one(state);
	}

	(*app->activity->vm).DetachCurrentThread();
}

JNIEXPORT void JNICALL
Java_com_example_test_MainActivity_onPermissionsGranted(JNIEnv *jni, jobject activity)
{
	state.hasPermissions = true;
}

static void
onAppCmd(struct android_app *app, int32_t cmd)
{
	switch (cmd) {
	case APP_CMD_START: U_LOG_E("APP_CMD_START"); break;
	case APP_CMD_RESUME: U_LOG_E("APP_CMD_RESUME"); break;
	case APP_CMD_PAUSE: U_LOG_E("APP_CMD_PAUSE"); break;
	case APP_CMD_STOP: U_LOG_E("APP_CMD_STOP"); break;
	case APP_CMD_DESTROY: U_LOG_E("APP_CMD_DESTROY"); break;
	case APP_CMD_INIT_WINDOW: U_LOG_E("APP_CMD_INIT_WINDOW"); break;
	case APP_CMD_TERM_WINDOW: U_LOG_E("APP_CMD_TERM_WINDOW"); break;
	}
}

static void
initializeEGL(void)
{
	state.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (state.display == EGL_NO_DISPLAY) {
		U_LOG_E("Failed to get EGL display");
		return;
	}

	bool success = eglInitialize(state.display, NULL, NULL);

	if (!success) {
		U_LOG_E("Failed to initialize EGL");
		return;
	}

	EGLint configCount;
	EGLConfig configs[1024];
	success = eglGetConfigs(state.display, configs, 1024, &configCount);

	if (!success) {
		U_LOG_E("Failed to get EGL configs");
		return;
	}

	const EGLint attributes[] = {EGL_RED_SIZE,   8, EGL_GREEN_SIZE,   8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
	                             EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_SAMPLES,   0, EGL_NONE};

	for (EGLint i = 0; i < configCount && !state.config; i++) {
		EGLint renderableType;
		EGLint surfaceType;

		eglGetConfigAttrib(state.display, configs[i], EGL_RENDERABLE_TYPE, &renderableType);
		eglGetConfigAttrib(state.display, configs[i], EGL_SURFACE_TYPE, &surfaceType);

		if ((renderableType & EGL_OPENGL_ES3_BIT) == 0) {
			continue;
		}

		if ((surfaceType & (EGL_PBUFFER_BIT | EGL_WINDOW_BIT)) != (EGL_PBUFFER_BIT | EGL_WINDOW_BIT)) {
			continue;
		}

		for (size_t a = 0; a < sizeof(attributes) / sizeof(attributes[0]); a += 2) {
			if (attributes[a] == EGL_NONE) {
				state.config = configs[i];
				break;
			}

			EGLint value;
			eglGetConfigAttrib(state.display, configs[i], attributes[a], &value);
			if (value != attributes[a + 1]) {
				break;
			}
		}
	}

	if (!state.config) {
		U_LOG_E("Failed to find suitable EGL config");
	}

	EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

	if ((state.context = eglCreateContext(state.display, state.config, EGL_NO_CONTEXT, contextAttributes)) ==
	    EGL_NO_CONTEXT) {
		U_LOG_E("Failed to create EGL context");
	}

	EGLint surfaceAttributes[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};

	if ((state.surface = eglCreatePbufferSurface(state.display, state.config, surfaceAttributes)) ==
	    EGL_NO_SURFACE) {
		U_LOG_E("Failed to create EGL surface");
		eglDestroyContext(state.display, state.context);
		return;
	}

	if (eglMakeCurrent(state.display, state.surface, state.surface, state.context) == EGL_FALSE) {
		U_LOG_E("Failed to make EGL context current");
		eglDestroySurface(state.display, state.surface);
		eglDestroyContext(state.display, state.context);
	}
}
