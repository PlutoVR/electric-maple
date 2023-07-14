// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */

#include "gst/app_log.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstdlib>
#include "render.hpp"
#include "util/u_logging.h"
// #include "common.hpp"

bool
checkGLError(const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		ALOGE("RYLIE OpenGL error: %d, %s:%d", err, func, line);
		return false;
	}
	return true;
}

#define CHECK_GL_ERROR() checkGLError(__FUNCTION__, __LINE__)

void
checkGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		ALOGE("RYLIE OpenGL error: %s %s: %d, %s:%d", when, expr, err, func, line);
	}
}

#define GL(EXPR)                                                                                                       \
	do {                                                                                                           \
		checkGLErrorWrap("before", #EXPR, __FUNCTION__, __LINE__);                                             \
		EXPR;                                                                                                  \
		checkGLErrorWrap("after", #EXPR, __FUNCTION__, __LINE__);                                              \
	} while (0)


bool
checkEGLError(const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		ALOGE("RYLIE EGL error: %d, %s:%d", err, func, line);
		return false;
	}
	return true;
}

#define CHECK_EGL_ERROR() checkEGLError(__FUNCTION__, __LINE__)

void
checkEGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		ALOGE("RYLIE EGL error: %s %s: %d, %s:%d", when, expr, err, func, line);
	}
}

#define EGL(EXPR)                                                                                                      \
	do {                                                                                                           \
		checkEGLErrorWrap("before", #EXPR, __FUNCTION__, __LINE__);                                            \
		EXPR;                                                                                                  \
		checkEGLErrorWrap("after", #EXPR, __FUNCTION__, __LINE__);                                             \
	} while (0)

// Initialize EGL context. We'll need this going forward.
void
initializeEGL(struct state_t &state)
{
	EGL(state.display = eglGetDisplay(EGL_DEFAULT_DISPLAY));
	if (state.display == EGL_NO_DISPLAY) {
		ALOGE("Failed to get EGL display");
		return;
	}

	bool success = eglInitialize(state.display, NULL, NULL);

	CHECK_EGL_ERROR();
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
	EGL(eglChooseConfig(state.display, attributes, configs, MAX_CONFIGS, &num_configs));

	if (num_configs == 0) {
		ALOGE("Failed to find suitable EGL config");
		abort();
	}
	ALOGI("Got %d egl configs, just taking the first one.", num_configs);

	state.config = configs[0];

	EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	EGL(state.context = eglCreateContext(state.display, state.config, EGL_NO_CONTEXT, contextAttributes));

	if (state.context == EGL_NO_CONTEXT) {
		ALOGE("Failed to create EGL context");
		abort();
	}
	CHECK_EGL_ERROR();

	EGLint surfaceAttributes[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
	EGL(state.surface = eglCreatePbufferSurface(state.display, state.config, surfaceAttributes));
	if (state.surface == EGL_NO_SURFACE) {
		ALOGE("Failed to create EGL surface");
		eglDestroyContext(state.display, state.context);
		return;
	}
	CHECK_EGL_ERROR();

	if (eglMakeCurrent(state.display, state.surface, state.surface, state.context) == EGL_FALSE) {
		ALOGE("Failed to make EGL context current");
		CHECK_EGL_ERROR();
		eglDestroySurface(state.display, state.surface);
		eglDestroyContext(state.display, state.context);
	}
}

// Vertex shader source code
const GLchar *vertexShaderSource = R"(
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
const GLchar *fragmentShaderSource = R"(
    #version 300 es
    precision lowp float;

    in vec2 frag_uv;
    out vec4 frag_color;
    uniform sampler2D textureSampler;

    void main() {
        frag_color = texture(textureSampler, frag_uv);
    }
)";

GLuint shaderProgram;
GLuint quadVAO, quadVBO;

// Function to check shader compilation errors
void
checkShaderCompilation(GLuint shader)
{
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
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
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		ALOGE("Shader program linking failed: %s\n", infoLog);
	}
}

void
setupShaders()
{
	// Compile the vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	CHECK_GL_ERROR();
	GL(glShaderSource(vertexShader, 1, &vertexShaderSource, NULL));
	GL(glCompileShader(vertexShader));
	checkShaderCompilation(vertexShader);

	// Compile the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	CHECK_GL_ERROR();
	GL(glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL));
	GL(glCompileShader(fragmentShader));
	checkShaderCompilation(fragmentShader);

	// Create and link the shader program
	shaderProgram = glCreateProgram();
	CHECK_GL_ERROR();
	GL(glAttachShader(shaderProgram, vertexShader));
	GL(glAttachShader(shaderProgram, fragmentShader));
	GL(glLinkProgram(shaderProgram));
	checkProgramLinking(shaderProgram);

	// Clean up the shaders as they're no longer needed
	GL(glDeleteShader(vertexShader));
	GL(glDeleteShader(fragmentShader));
}

void
setupQuadVertexData()
{
	// Set up the quad vertex data
#if 1
	// old, chatgpt
	GLfloat quadVertices[] = {// Positions    // UVs
	                          -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	                          1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 1.0f};
#else
	// also chatgpt
	GLfloat quadVertices[] = {// Positions    // Texture Coords
	                          -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
	                          1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};
#endif

	GL(glGenVertexArrays(1, &quadVAO));
	GL(glGenBuffers(1, &quadVBO));

	GL(glBindVertexArray(quadVAO));
	GL(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
	GL(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

	GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0));
	GL(glEnableVertexAttribArray(0));

	GL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat))));
	GL(glEnableVertexAttribArray(1));

	GL(glBindVertexArray(0));
}

void
setupRender()
{
	setupShaders();
	setupQuadVertexData();
}

void
draw(GLuint framebuffer, GLuint texture)
{
	//    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// Clear the framebuffer

	EGLContext eglc = eglGetCurrentContext();
	if (eglc == EGL_NO_CONTEXT) {
		ALOGE("No EGL context");
		abort();
	}
	// Use the shader program
	GL(glUseProgram(shaderProgram));

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

	// Draw the quad
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

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

	// Unbind the framebuffer
	//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
