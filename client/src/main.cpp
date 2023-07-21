// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Main file for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <array>
#include <errno.h>

#include <assert.h>
#include "gst/gst_common.h"
#include "render.hpp"

#include <unistd.h>

#include <android/native_activity.h>
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <GLES2/gl2ext.h>

// FOR RYLIE: The below is one of those deps to monado that
// still exist in current code. Eventually, would be nice
// to remove the monado dep completely on client side.
#include "os/os_threading.h"

static int pfd[2];
static pthread_t thr;
static const char *tag = "myapp";

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

// FOR RYLIE: For seeing our U_LOG_E appear in LOGCAT :)
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

// FOR RYLIE: This is a general state var shared accross both c++ and C sides
// Take a look at gst_common.h...There's also 'vid' that's shared. eventually
// merge state and vid into one single struct.
static state_t state = {};

// FOR RYLIE: THE APP_CMD_ START/RESUME will not fire if quest2 is NOT worn
// unless the proper dev params on the headset have been set (haven't done that).
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

// FOR RYLIE: The below is a remnant of xrt_fs usage (monado-provide "frame-server)
//           in time, completely get rid of monado since the below's not used.
//           what is now pushing the frame is gstsl in the glsinkbin gstreamer
//           element, and it's making the frames available as gl textures directly.
//           No need to deal with GstBuffer as it's done in the monado frameserver part.
void
sink_push_frame(struct xrt_frame_sink *xfs, struct xrt_frame *xf)
{
	U_LOG_E("FRED: sink_push_frame called!");
	if (!xf) {
		U_LOG_E("what??");
		return;
	}
	struct state_t *st = container_of(xfs, struct state_t, frame_sink);

	// This can cause a segfault if we hold onto one frame for too long so OH.
	if (!st->xf) {
		//		xrt_frame_reference(&xf, st->xf);
		xrt_frame_reference(&st->xf, xf);
	}
	U_LOG_E("Called! %d %p %u %u %zu", st->frame_texture_id, xf->data, xf->width, xf->height, xf->stride);
}

// FOR RYLIE : The function was use by Moshi to render "something" on app start.
//            mainly for making sure the renderer (implemented in Render.cpp) was
//            working. In your case, when integrating to PlutosphereOXR, you're
//            probably going to have to rework the whole renderer part. You'll still
//            get a gl texture id when calling gst_app_sink_try_pull_sample(...) below,
//            but I will let you do the binding on that potentiual quad layer as you
//            see fit. Just don't forget to check if hw decoder gave you a TEXTURE_EXTERNAL
//            (and probably not a TEXTURE_2D), which then would require the appropriate
//            shader-related parameters to sample texture-external :
//
//            #extension GL_OES_EGL_image_external : require
//            precision mediump float;
//            uniform samplerExternalOES sTextureName;
//
//
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

// FOR RYLIE: You see that Moshi's initial renderer was assuming Texture 2D with RGBA color format
//           because that's what monado's "Frameserver" was giving us (extra slow however, with
//           copies...). In your case when integrating into PlutosphereOXR, you'll be using gstgl's
//           ability to render decoded frames (YUV) directly onto underlying android::Surface (nested
//           inside whenever the glsinkbin element's being given the memory:GLMemory CAPS), which will
//           come out to you when pulling the sample as gl texture IDs that you'll be able to use as
//           samplers into your glsl shaders for rendering onto a quad (Or ?? pass directly to OpenXR
//           on a quad layer ?)... and so most of the rendering bits in here, you'll have to leave out
//           when integrating to Pluto's client app.
//
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
die_errno()
{
	U_LOG_E("Something happened! %s", strerror(errno));
}

// FOR RYLIE: you'll see, the connexion scheme is a bit complex, in 2 phases since we're
// dealing with webrtc. There's a "signalling" server, exposed by the server par on
// 127.0.0.1:8080 to do the "ICE transaction", which basically send an "offer" of possible
// network interfaces for server and client and finds a matching pair for the webrtc
// connection/stream between server and client. (This has all been tested working on the
// quest2 with the webrtc pipeline and ICE callback (mostly implemented in gst_driver.c)

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
	return;
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

// FOR RYLIE: Our main loop on the main app thread. Very important stuff. Also note the
// very important call to gst_app_sink_try_pull_sample(...) that retrieves the newest
// sample from the appsink (connected to our glsinkbin end in gst_driver.c). Those are
// then interpreted as gl texture ids to be bound to egl context.
//
// Important note about EGL contexts here. Note that gstgl (glsinkbin) HAS to get created
// with RENDERING EGL CONTEXT current (that's happening in gst_driver.c when calling
// gst_gl_context_get_current_gl_context(...). Reason's simple... when gstgl initializes
// it'll create a SHARED egl context with whatever's current and so pay carefull attention
// to the eglMakeCurrent here and there.
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
			case XR_SESSION_STATE_IDLE: U_LOG_I("OpenXR session is now IDLE"); break;
			case XR_SESSION_STATE_READY: {
				U_LOG_I("OpenXR session is now READY, beginning session");
				XrSessionBeginInfo beginInfo = {};
				beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
				beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

				result = xrBeginSession(state.session, &beginInfo);

				if (XR_FAILED(result)) {
					U_LOG_I("Failed to begin OpenXR session (%d)", result);
				}
			} break;
			case XR_SESSION_STATE_SYNCHRONIZED: U_LOG_I("OpenXR session is now SYNCHRONIZED"); break;
			case XR_SESSION_STATE_VISIBLE: U_LOG_I("OpenXR session is now VISIBLE"); break;
			case XR_SESSION_STATE_FOCUSED: U_LOG_I("OpenXR session is now FOCUSED"); break;
			case XR_SESSION_STATE_STOPPING:
				U_LOG_I("OpenXR session is now STOPPING");
				xrEndSession(state.session);
				break;
			case XR_SESSION_STATE_LOSS_PENDING: U_LOG_I("OpenXR session is now LOSS_PENDING"); break;
			case XR_SESSION_STATE_EXITING: U_LOG_I("OpenXR session is now EXITING"); break;
			default: break;
			}

			state.sessionState = event->state;
		}

		buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
	}

	// If session isn't ready, return. We'll be called again and will poll events again.
	if (state.sessionState < XR_SESSION_STATE_READY) {
		U_LOG_I("Waiting for session ready state!");
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
		U_LOG_E("Failed to locate views");
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
		U_LOG_E("Failed to acquire swapchain image (%d)", result);
	}

	XrSwapchainImageWaitInfo waitInfo = {.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
	                                     .timeout = XR_INFINITE_DURATION};

	result = xrWaitSwapchainImage(state.swapchain, &waitInfo);

	if (XR_FAILED(result)) {
		U_LOG_E("Failed to wait for swapchain image (%d)", result);
	}

	// FOR RYLIE: we're not asking xrfs for a frame, we're checking ourselves what appsink
	// may have for us. That's why the state.xf logic's been disabled (not needed anymore)
	// if (state.xf) {
	U_LOG_E("FRED: mainloop_one: Trying to get the EGL lock");
	os_mutex_lock(&state.egl_lock);
	if (eglMakeCurrent(state.display, state.surface, state.surface, state.context) == EGL_FALSE) {
		U_LOG_E("FRED: mainloop_one: Failed make egl context current");
	}

	// FOR RYLIE : As mentioned, the glsinkbin -> appsink part of the gstreamer pipeline in gst_driver.c
	// will output "samples" that might be signalled for using "new_sample_cb" in gst_driver (useful for
	// testing if the gstreamer sink elements are giving us "anything"), but also on which we can "poll"
	// to see and that's what we do here. Note that the non-try version of the call "gst_app_sink_pull_sample"
	// WILL BLOCK until there's a sample.

	// Get Newest sample from GST appsink. Waiting 1ms here before giving up (might want to adjust that time)
	U_LOG_E("DEBUG: Trying to get new gstgl sample, waiting max 1ms\n");
	g_autoptr(GstSample) sample =
	    gst_app_sink_try_pull_sample(GST_APP_SINK(state.vid->appsink), (GstClockTime)(1000 * GST_USECOND));
	if (sample != NULL) {

		U_LOG_E("FRED: GOT A SAMPLE !!!");
		GstBuffer *buffer = gst_sample_get_buffer(sample);
		GstCaps *caps = gst_sample_get_caps(sample);

		GstVideoInfo info;
		gst_video_info_from_caps(&info, caps);
		/*gint width = GST_VIDEO_INFO_WIDTH (&info);
		gint height = GST_VIDEO_INFO_HEIGHT (&info);*/

		// FOR RYLIE: Handle resize according to how it's done in PlutosphereOXR
		/*if (width != state.vid->width || height != state.vid->height) {
		    vid->width = width;
		    vid->height = height;
		}*/

		GstVideoFrame frame;
		GstMapFlags flags = static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL);
		gst_video_frame_map(&frame, &info, buffer, flags);
		state.frame_texture_id = *(GLuint *)frame.data[0];

		if (state.vid->context == NULL) {
			/* Get GStreamer's gl context. */
			gst_gl_query_local_gl_context(state.vid->appsink, GST_PAD_SINK, &state.vid->context);

			/* Check if we have 2D or OES textures */
			GstStructure *s = gst_caps_get_structure(caps, 0);
			const gchar *texture_target_str = gst_structure_get_string(s, "texture-target");
			if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_EXTERNAL_OES_STR)) {
				state.frame_texture_target = GL_TEXTURE_EXTERNAL_OES;
			} else if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_2D_STR)) {
				state.frame_texture_target = GL_TEXTURE_2D;
			} else {
				g_assert_not_reached();
			}
		}

		GstGLSyncMeta *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
		if (sync_meta) {
			/* MOSHI: the set_sync() seems to be needed for resizing */
			gst_gl_sync_meta_set_sync_point(sync_meta, state.vid->context);
			gst_gl_sync_meta_wait(sync_meta, state.vid->context);
		}

		// FIXME: Might not be necessary, since we're polling.
		// This will make our main renderer pick up and render the gl texture.
		state.vid->state->frame_available = true;

		gst_video_frame_unmap(&frame);

		// FIXME: This will go away now that we have a GL_TEXTURE_EXTERNAL texture
		U_LOG_E("DEBUG: Binding textures!");
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


		// can be factored into the above, it's just useful to be able to disable seperately
		/*if (state.xf) {
		    xrt_frame_reference(&state.xf, NULL);
		}*/

		U_LOG_E("DEBUG: Binding framebuffer\n");
		glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[imageIndex]);

		glViewport(0, 0, state.width * 2, state.height);


		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Just display purple nothingness
		U_LOG_E("DEBUG: DRAWING!\n");
		for (uint32_t eye = 0; eye < 2; eye++) {
			glViewport(eye * state.width, 0, state.width, state.height);
			draw(state.framebuffers[imageIndex], state.frame_texture_id);
		}

		// Release

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	} else {
		U_LOG_E("FRED: NO gst appsink sample...");
	}

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

	eglMakeCurrent(state.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	U_LOG_E("FRED: mainloop_one: releasing the EGL lock");
	os_mutex_unlock(&state.egl_lock);
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

// FOR RYLIE : This is our main android app entry point. please note that there is
// a PROFOUND difference between our webrtc/gstreamer-pipeline test application here
// and PlutosphereOXR on which you'll have to integrate... The current test app
// is a NO-JAVA one (AndroidManifest's hasCode=false), whereas PlutosphereOXR
// is based on a java entrypoint (class MainActivity : NativeActivity() with the
// onCreate(...) method). Since their app's also making use of native C/C++ code
// and cmake, you'll be able to integrate most of what you find in here easily
// but the crux of the integration will be about gstreamer-1.0 and the plugins
// (especially androidmedia, which makes use of JNI).
//
// Read on, you'll see, most of the explanations around integrating libgstreamer-1.0
// and the plugins is going to be in gst_driver.c.
void
android_main(struct android_app *app)
{
	start_logger("meow meow");

	// FOR RYLIE : VERY VERY useful for debugging gstreamer.
	// GST_DEBUG = *:3 will give you ONLY ERROR-level messages.
	// GST_DEBUG = *:6 will give you ALL messages (make sure you BOOST your android-studio's
	// Logcat buffer to be able to capture everything gstreamer'S going to spit at you !
	// in Tools -> logcat -> Cycle Buffer Size (I set it to 102400 KB).

	// setenv("GST_DEBUG", "*:3", 1);
	// setenv("GST_DEBUG", "*ssl*:9,*tls*:9,*webrtc*:9", 1);
	// setenv("GST_DEBUG", "GST_CAPS:5", 1);
	setenv("GST_DEBUG", "*:2,*CAPS*:6,amcvideodec:7", 1);

	// do not do ansi color codes
	setenv("GST_DEBUG_NO_COLOR", "1", 1);

	state.app = app;
	state.java_vm = app->activity->vm;

	(*app->activity->vm).AttachCurrentThread(&state.jni, NULL);
	app->onAppCmd = onAppCmd;

	os_mutex_init(&state.egl_lock);
	U_LOG_E("FRED: main: Trying to get the EGL lock");
	os_mutex_lock(&state.egl_lock);
	initializeEGL(state);

	// FOR RYLIE : This is a monado's frameserver remnant.
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
		U_LOG_E("Failed to initialize OpenXR loader\n");
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
		U_LOG_E("Failed to initialize OpenXR instance\n");
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
		U_LOG_E("Failed to enumerate view configurations\n");
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
		U_LOG_E("Failed to enumerate view configuration views\n");
		return;
	}

	state.width = viewInfo[0].recommendedImageRectWidth;
	state.height = viewInfo[0].recommendedImageRectHeight;

	state.frame_texture_id = generateRandomTexture(state.width, state.height, 2);

	// FOR RYLIE: This call below is a remnant of xrt_fs usage. That CB is not even called.
	//        in time, completely get rid of monado for that.
	state.frame_sink.push_frame = sink_push_frame;
	struct xrt_frame_context xfctx = {};

	// FOR RYLIE: This is where everything gstreamer starts.
	U_LOG_E("FRED: Creating gst pipeline");
	struct xrt_fs *blah = vf_fs_gst_pipeline(&xfctx, &state);
	U_LOG_E("FRED: Done Creating gst pipeline");

	U_LOG_E("FRED: Starting xrt_fs source. Not used, please remove eventually.\n");
	// FIXME: Eventually completely remove XRT_FS dependency here for driving the gst pipeline look
	xrt_fs_stream_start(blah, &state.frame_sink, XRT_FS_CAPTURE_TYPE_TRACKING, 0);

	// OpenXR session
	U_LOG_E("FRED: Creating OpenXR session...");
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
		U_LOG_E("ERROR: Failed to create OpenXR session (%d)\n", result);
	}

	U_LOG_E("FRED: Creating OpenXR Swapchain...");
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
		U_LOG_E("Failed to create OpenXR swapchain (%d)\n", result);
	}

	for (uint32_t i = 0; i < 4; i++) {
		state.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
	}

	xrEnumerateSwapchainImages(state.swapchain, 4, &state.imageCount, (XrSwapchainImageBaseHeader *)state.images);

	if (XR_FAILED(result)) {
		U_LOG_E("ERROR: Failed to get swapchain images (%d)\n", result);
	}

	// FOR RYLIE: The below framebuffer creation and redering-related calls will have to get
	// adapted to PlutosphereOXR's reality.
	U_LOG_E("FRED: Gen and bind gl texture and framebuffers.");
	glGenFramebuffers(state.imageCount, state.framebuffers);

	// FIXME: This if below is suspicious.. we're not using xf anymore....
	if (state.xf) {
		glBindTexture(GL_TEXTURE_2D, state.frame_texture_id);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, state.xf->stride / 4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state.xf->width, state.xf->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		             state.xf->data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// This is important here. Make sure we're properly creating framebuffer.
	for (uint32_t i = 0; i < state.imageCount; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.images[i].image, 0);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			U_LOG_E("Failed to create framebuffer (%d)\n", status);
		}
	}

	U_LOG_E("FRED: Create spaces\n");
	create_spaces(state);

	U_LOG_E("FRED: Setup Render\n");
	setupRender();

	eglMakeCurrent(state.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	U_LOG_E("FRED: main: releasing the EGL lock");
	os_mutex_unlock(&state.egl_lock);

	U_LOG_E("FRED: Really make socket\n");
	really_make_socket(state);


	// Main rendering loop.
	U_LOG_E("DEBUG: Starting main loop.\n");
	while (!app->destroyRequested) {
		mainloop_one(state);
	}

	os_mutex_destroy(&state.egl_lock);
	(*app->activity->vm).DetachCurrentThread();
}
