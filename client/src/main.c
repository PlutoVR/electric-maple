#define IP "10.0.1.2"

#include <android_native_app_glue.c>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>
#include <stdbool.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>


#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "app", __VA_ARGS__)
#define XR_LOAD(fn) xrGetInstanceProcAddr(state.instance, #fn, (PFN_xrVoidFunction*) &fn);

static struct {
  struct android_app* app;
  JNIEnv* jni;
  bool hasPermissions;
  ANativeWindow* window;
  EGLDisplay display;
  EGLContext context;
  EGLConfig config;
  EGLSurface surface;
  XrInstance instance;
  XrSystemId system;
  XrSession session;
  XrSessionState sessionState;
  XrSpace worldSpace;
  XrSwapchain swapchain;
  XrSwapchainImageOpenGLESKHR images[4];
  GLuint framebuffers[4];
  uint32_t imageCount;
  uint32_t width;
  uint32_t height;
} state;

static void onAppCmd(struct android_app* app, int32_t cmd);
static void initializeEGL(void);

void android_main(struct android_app* app) {
  state.app = app;
  (*app->activity->vm)->AttachCurrentThread(app->activity->vm, &state.jni, NULL);
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

  XrResult result = xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*) &loaderInfo);

  if (XR_FAILED(result)) {
    LOG("Failed to initialize OpenXR loader");
  }

  // Create OpenXR instance

  const char* extensions[] = {
    "XR_KHR_opengl_es_enable"
  };

  XrInstanceCreateInfo instanceInfo = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .applicationInfo.engineName = "N/A",
    .applicationInfo.applicationName = "N/A",
    .applicationInfo.apiVersion = XR_CURRENT_API_VERSION,
    .enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
    .enabledExtensionNames = extensions
  };

  result = xrCreateInstance(&instanceInfo, &state.instance);

  if (XR_FAILED(result)) {
    LOG("Failed to initialize OpenXR instance");
  }

  // OpenXR system

  XrSystemGetInfo systemInfo = {
    .type = XR_TYPE_SYSTEM_GET_INFO,
    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
  };

  result = xrGetSystem(state.instance, &systemInfo, &state.system);

  uint32_t viewConfigurationCount;
  XrViewConfigurationType viewConfigurations[2];
  result = xrEnumerateViewConfigurations(state.instance, state.system, 2, &viewConfigurationCount, viewConfigurations);

  if (XR_FAILED(result)) {
    LOG("Failed to enumerate view configurations");
  }

  uint32_t viewCount;
  XrViewConfigurationView viewInfo[2] = {
    [0].type = XR_TYPE_VIEW_CONFIGURATION_VIEW,
    [1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW
  };

  xrEnumerateViewConfigurationViews(state.instance, state.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, NULL);
  result = xrEnumerateViewConfigurationViews(state.instance, state.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, viewInfo);

  if (XR_FAILED(result) || viewCount != 2) {
    LOG("Failed to enumerate view configuration views");
  }

  state.width = viewInfo[0].recommendedImageRectWidth;
  state.height = viewInfo[0].recommendedImageRectHeight;

  // OpenXR session

  PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR = NULL;
  XR_LOAD(xrGetOpenGLESGraphicsRequirementsKHR);
  XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR };
  xrGetOpenGLESGraphicsRequirementsKHR(state.instance, state.system, &graphicsRequirements);

  XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {
    .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
    .display = state.display,
    .config = state.config,
    .context = state.context
  };

  XrSessionCreateInfo sessionInfo = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = &graphicsBinding,
    .systemId = state.system
  };

  result = xrCreateSession(state.instance, &sessionInfo, &state.session);

  if (XR_FAILED(result)) {
    LOG("Failed to create OpenXR session (%d)", result);
  }

  // OpenXR swapchain

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

  result = xrCreateSwapchain(state.session, &swapchainInfo, &state.swapchain);

  if (XR_FAILED(result)) {
    LOG("Failed to create OpenXR swapchain (%d)", result);
  }

  for (uint32_t i = 0; i < 4; i++) {
    state.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
  }

  xrEnumerateSwapchainImages(state.swapchain, 4, &state.imageCount, (XrSwapchainImageBaseHeader*) state.images);

  if (XR_FAILED(result)) {
    LOG("Failed to get swapchain images (%d)", result);
  }

  glGenFramebuffers(state.imageCount, state.framebuffers);

  for (uint32_t i = 0; i < state.imageCount; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.images[i].image, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      LOG("Failed to create framebuffer (%d)", status);
    }
  }

  // Space

  XrReferenceSpaceCreateInfo spaceInfo = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
    .poseInReferenceSpace = { { 0.f, 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f } }
  };

  result = xrCreateReferenceSpace(state.session, &spaceInfo, &state.worldSpace);

  if (XR_FAILED(result)) {
    LOG("Failed to create reference space (%d)", result);
  }

  // Loop

  while (!app->destroyRequested) {

    // Poll Android events

    for (;;) {
      int events;
      struct android_poll_source* source;
      bool wait = !app->window || app->activityState != APP_CMD_RESUME;
      int timeout = wait ? -1 : 0;
      if (ALooper_pollAll(timeout, NULL, &events, (void**) &source) >= 0) {
        if (source) {
          source->process(app, source);
        }

        if (timeout == 0 && (!app->window || app->activityState != APP_CMD_RESUME)) {
          break;
        }
      } else {
        break;
      }
    }

    // Poll OpenXR events

    XrEventDataBuffer buffer;
    buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
    buffer.next = NULL;

    while (xrPollEvent(state.instance, &buffer) == XR_SUCCESS) {
      if (buffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
        XrEventDataSessionStateChanged* event = (XrEventDataSessionStateChanged*) &buffer;

        switch (event->state) {
          case XR_SESSION_STATE_IDLE:
            LOG("OpenXR session is now IDLE");
            break;
          case XR_SESSION_STATE_READY:
            LOG("OpenXR session is now READY, beginning session");

            result = xrBeginSession(state.session, &(XrSessionBeginInfo) {
              .type = XR_TYPE_SESSION_BEGIN_INFO,
              .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
            });

            if (XR_FAILED(result)) {
              LOG("Failed to begin OpenXR session (%d)", result);
            }

            break;
          case XR_SESSION_STATE_SYNCHRONIZED: LOG("OpenXR session is now SYNCHRONIZED"); break;
          case XR_SESSION_STATE_VISIBLE: LOG("OpenXR session is now VISIBLE"); break;
          case XR_SESSION_STATE_FOCUSED: LOG("OpenXR session is now FOCUSED"); break;
          case XR_SESSION_STATE_STOPPING:
            LOG("OpenXR session is now STOPPING");
            xrEndSession(state.session);
            break;
          case XR_SESSION_STATE_LOSS_PENDING: LOG("OpenXR session is now LOSS_PENDING"); break;
          case XR_SESSION_STATE_EXITING: LOG("OpenXR session is now EXITING"); break;
          default: break;
        }

        state.sessionState = event->state;
      }

      buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    // Spin until session is ready and permissions are granted

    if (state.sessionState < XR_SESSION_STATE_READY) { // || !state.hasPermissions) {
      LOG("Waiting!");
      continue;
    }

    // Begin frame

    XrFrameState frameState = {
      .type = XR_TYPE_FRAME_STATE
    };

    result = xrWaitFrame(state.session, NULL, &frameState);

    if (XR_FAILED(result)) {
      LOG("xrWaitFrame failed");
    }

    XrFrameBeginInfo beginfo = {
      .type = XR_TYPE_FRAME_BEGIN_INFO
    };

    result = xrBeginFrame(state.session, &beginfo);

    if (XR_FAILED(result)) {
      LOG("xrBeginFrame failed");
    }

    // Locate views, set up layers

    XrView views[2] = {
      [0].type = XR_TYPE_VIEW,
      [1].type = XR_TYPE_VIEW
    };

    XrViewLocateInfo locateInfo = {
      .type = XR_TYPE_VIEW_LOCATE_INFO,
      .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      .displayTime = frameState.predictedDisplayTime,
      .space = state.worldSpace
    };

    XrViewState viewState = {
      .type = XR_TYPE_VIEW_STATE
    };

    uint32_t viewCount;
    result = xrLocateViews(state.session, &locateInfo, &viewState, 2, &viewCount, views);

    if (XR_FAILED(result)) {
      LOG("Failed to locate views");
    }

    XrCompositionLayerProjection layer = {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
      .space = state.worldSpace,
      .viewCount = 2,
      .views = (XrCompositionLayerProjectionView[2]) {
        {
          .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
          .subImage = { state.swapchain, { { 0, 0 }, { state.width, state.height } } },
          .pose = views[0].pose,
          .fov = views[0].fov
        },
        {
          .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
          .subImage = { state.swapchain, { { state.width, 0 }, { state.width, state.height } } },
          .pose = views[1].pose,
          .fov = views[1].fov
        }
      }
    };

    // Render

    // ??
    frameState.shouldRender = true;

    if (frameState.shouldRender) {
      uint32_t imageIndex;
      result = xrAcquireSwapchainImage(state.swapchain, NULL, &imageIndex);

      if (XR_FAILED(result)) {
        LOG("Failed to acquire swapchain image (%d)", result);
      }

      XrSwapchainImageWaitInfo waitInfo = {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
        .timeout = XR_INFINITE_DURATION
      };

      result = xrWaitSwapchainImage(state.swapchain, &waitInfo);

      if (XR_FAILED(result)) {
        LOG("Failed to wait for swapchain image (%d)", result);
      }


      glBindFramebuffer(GL_FRAMEBUFFER, state.framebuffers[imageIndex]);

      // Just display purple nothingness
      for (uint32_t eye = 0; eye < 2; eye++) {
        glViewport(state.width * eye, 0, state.width, state.height);
          glClearColor(1.f, 0.f, 1.f, 1.f);
          glClear(GL_COLOR_BUFFER_BIT);
      }

      // Release

      glBindFramebuffer(GL_FRAMEBUFFER, 0);


      xrReleaseSwapchainImage(state.swapchain, NULL);
    }

    // Submit frame

    XrFrameEndInfo endInfo = {
      .type = XR_TYPE_FRAME_END_INFO,
      .displayTime = frameState.predictedDisplayTime,
      .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
      .layerCount = frameState.shouldRender,
      .layers = (const XrCompositionLayerBaseHeader*[1]) {
        (XrCompositionLayerBaseHeader*) &layer
      }
    };

    xrEndFrame(state.session, &endInfo);
  }

  (*app->activity->vm)->DetachCurrentThread(app->activity->vm);
}

JNIEXPORT void JNICALL Java_com_example_test_MainActivity_onPermissionsGranted(JNIEnv* jni, jobject activity) {
  state.hasPermissions = true;
}

static void onAppCmd(struct android_app* app, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_START: LOG("APP_CMD_START"); break;
    case APP_CMD_RESUME: LOG("APP_CMD_RESUME"); break;
    case APP_CMD_PAUSE: LOG("APP_CMD_PAUSE"); break;
    case APP_CMD_STOP: LOG("APP_CMD_STOP"); break;
    case APP_CMD_DESTROY: LOG("APP_CMD_DESTROY"); break;
    case APP_CMD_INIT_WINDOW: LOG("APP_CMD_INIT_WINDOW"); break;
    case APP_CMD_TERM_WINDOW: LOG("APP_CMD_TERM_WINDOW"); break;
  }
}

static void initializeEGL(void) {
  state.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  if (state.display == EGL_NO_DISPLAY) {
    LOG("Failed to get EGL display");
    return;
  }

  bool success = eglInitialize(state.display, NULL, NULL);

  if (!success) {
    LOG("Failed to initialize EGL");
    return;
  }

  EGLint configCount;
  EGLConfig configs[1024];
  success = eglGetConfigs(state.display, configs, 1024, &configCount);

  if (!success) {
    LOG("Failed to get EGL configs");
    return;
  }

  const EGLint attributes[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 0,
    EGL_STENCIL_SIZE, 0,
    EGL_SAMPLES, 0,
    EGL_NONE
  };

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
    LOG("Failed to find suitable EGL config");
  }

  EGLint contextAttributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE
  };

  if ((state.context = eglCreateContext(state.display, state.config, EGL_NO_CONTEXT, contextAttributes)) == EGL_NO_CONTEXT) {
    LOG("Failed to create EGL context");
  }

  EGLint surfaceAttributes[] = {
    EGL_WIDTH, 16,
    EGL_HEIGHT, 16,
    EGL_NONE
  };

  if ((state.surface = eglCreatePbufferSurface(state.display, state.config, surfaceAttributes)) == EGL_NO_SURFACE) {
    LOG("Failed to create EGL surface");
    eglDestroyContext(state.display, state.context);
    return;
  }

  if (eglMakeCurrent(state.display, state.surface, state.surface, state.context) == EGL_FALSE) {
    LOG("Failed to make EGL context current");
    eglDestroySurface(state.display, state.surface);
    eglDestroyContext(state.display, state.context);
  }
}

