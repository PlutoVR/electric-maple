// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 * @author Ryan Pavlik <rpavlik@collabora.com>
 */

#include "render.hpp"

#include "GLDebug.h"
#include "GLError.h"
#include "gst/app_log.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <openxr/openxr.h>
#include <stdexcept>

RendererData::RendererData(std::unique_ptr<EglData> &&egl) : egl_(std::move(egl))
{
	egl_->makeCurrent();
	registerGlDebugCallback();
	setupRender();
	egl_->makeNotCurrent();
}


// Vertex shader source code
static constexpr const GLchar *vertexShaderSource = R"(
    #version 300 es
    in vec3 position;
    in vec2 uv;
    out vec2 frag_uv;

    void main() {
        gl_Position = vec4(position, 1.0);
        frag_uv = uv;
    }
)";

// Fragment shader source code
static constexpr const GLchar *fragmentShaderSource = R"(
    #version 300 es
    #extension GL_OES_EGL_image_external : require
    #extension GL_OES_EGL_image_external_essl3 : require
    precision mediump float;

    in vec2 frag_uv;
    out vec4 frag_color;
    uniform samplerExternalOES textureSampler;

    void main() {
        frag_color = texture(textureSampler, frag_uv);
    }
)";

// Function to check shader compilation errors
void
checkShaderCompilation(GLuint shader)
{
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
		ALOGE("Shader compilation failed: %s\n", infoLog);
	}
}

// Function to check shader program linking errors
void
checkProgramLinking(GLuint program)
{
	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
		ALOGE("Shader program linking failed: %s\n", infoLog);
	}
}

void
RendererData::setupShaders()
{
	// Compile the vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	checkShaderCompilation(vertexShader);

	// Compile the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	checkShaderCompilation(fragmentShader);

	// Create and link the shader program
	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	checkProgramLinking(program);

	// Clean up the shaders as they're no longer needed
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	textureSamplerLocation_ = glGetUniformLocation(program, "textureSampler");
}

struct TextureCoord
{
	float u;
	float v;
};
struct Vertex
{
	XrVector3f pos;
	TextureCoord texcoord;
};
static constexpr size_t kVertexBufferStride = sizeof(Vertex);

static_assert(kVertexBufferStride == 5 * sizeof(GLfloat), "3 position coordinates and u,v");

void
RendererData::setupQuadVertexData()
{
	// Set up the quad vertex data
	static constexpr Vertex quadVertices[] = {
	    {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
	    {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
	    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	static constexpr size_t stride = sizeof(Vertex) / sizeof(GLfloat);
	static_assert(stride == 5, "3 position coordinates and u,v");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kVertexBufferStride, (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kVertexBufferStride, (GLvoid *)offsetof(Vertex, texcoord));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void
RendererData::setupRender()
{
	setupShaders();
	setupQuadVertexData();
}

void
RendererData::draw(GLuint framebuffer, GLuint texture, GLenum texture_target) const
{
	//    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// Use the shader program
	glUseProgram(program);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, texture);
	glBindTexture(texture_target, texture);
	glUniform1i(textureSamplerLocation_, 0);

	// Draw the quad
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

#if 0
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		const char *errorStr;
		switch (err) {
		case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
		default: errorStr = "Unknown error"; break;
		}
		ALOGE("error! %s", errorStr);
	}
#endif

	// Unbind the framebuffer
	//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

EglData::EglData()
{

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (display == EGL_NO_DISPLAY) {
		ALOGE("Failed to get EGL display");
		return;
	}

	bool success = eglInitialize(display, NULL, NULL);

	if (!success) {
		ALOGE("Failed to initialize EGL");
		return;
	}

#define MAX_CONFIGS 1024
	EGLConfig configs[MAX_CONFIGS];

	// RGBA8, multisample not required, ES3, pbuffer and window
	const EGLint attributes[] = {
	    EGL_RED_SIZE,
	    8, //

	    EGL_GREEN_SIZE,
	    8, //

	    EGL_BLUE_SIZE,
	    8, //

	    EGL_ALPHA_SIZE,
	    8, //

	    EGL_SAMPLES,
	    1, //

	    EGL_RENDERABLE_TYPE,
	    EGL_OPENGL_ES3_BIT,

	    EGL_SURFACE_TYPE,
	    (EGL_PBUFFER_BIT | EGL_WINDOW_BIT),

	    EGL_NONE,
	};

	EGLint num_configs = 0;
	CHK_EGL(eglChooseConfig(display, attributes, configs, MAX_CONFIGS, &num_configs));

	if (num_configs == 0) {
		ALOGE("Failed to find suitable EGL config");
		throw std::runtime_error("Failed to find suitable EGL config");
	}
	ALOGI("Got %d egl configs, just taking the first one.", num_configs);

	config = configs[0];

	EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	CHK_EGL(context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes));

	if (context == EGL_NO_CONTEXT) {
		ALOGE("Failed to create EGL context");
		throw std::runtime_error("Failed to create EGL context");
	}
	CHECK_EGL_ERROR();
	ALOGI("EGL: Created context");

	// TODO why are we making a 16x16 pbuffer surface? Do we even need it?
	EGLint surfaceAttributes[] = {
	    EGL_WIDTH,
	    16, //
	    EGL_HEIGHT,
	    16, //
	    EGL_NONE,
	};
	CHK_EGL(surface = eglCreatePbufferSurface(display, config, surfaceAttributes));
	if (surface == EGL_NO_SURFACE) {
		ALOGE("Failed to create EGL surface");
		eglDestroyContext(display, context);
		throw std::runtime_error("Failed to create EGL surface");
	}
	CHECK_EGL_ERROR();
	ALOGI("EGL: Created surface");
}

EglData::~EglData()
{
	EGLDisplay d = display;
	if (d == EGL_NO_DISPLAY) {
		d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}
	if (surface != EGL_NO_SURFACE) {
		eglDestroySurface(d, surface);
		surface = EGL_NO_SURFACE;
	}

	if (context != EGL_NO_CONTEXT) {
		eglDestroyContext(d, context);
		context = EGL_NO_CONTEXT;
	}
}

void
EglData::init()
{}

void
EglData::reset()
{}

void
EglData::makeCurrent()
{
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		ALOGE("Failed to make EGL context current");
		CHECK_EGL_ERROR();
		throw std::runtime_error("Could not make EGL context current");
	}
	ALOGI("EGL: Made context current");
}
