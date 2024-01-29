// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */

#include "common.hpp"
#include <iostream>

// Initialize EGL context. We'll need this going forward.
void
initializeEGL(struct state_t &state)
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


GLuint quadVAO, quadVBO;
GLuint shaderProgram;

const char *vertexShaderSource = R"glsl(
    #version 300 es
    precision highp float;

    layout (location = 0) in vec2 position;
    layout (location = 1) in vec2 texCoord;

    out vec2 TexCoord;

    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        TexCoord = texCoord;
    }
)glsl";

const char *fragmentShaderSource = R"glsl(
    #version 300 es
    precision highp float;

    in vec2 TexCoord;
    uniform sampler2D tex;

    out vec4 FragColor;

    void main() {
        FragColor = texture(tex, TexCoord);
    }
)glsl";

void
drawTriangle(GLuint texture)
{
	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

GLuint
makeShaderProgram()
{
	// Create quad VAO and VBO
	GLfloat quadVertices[] = {
	    // Positions    // TexCoords
	    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

	    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
	};
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
	glBindVertexArray(0);

	// Compile and link shader program
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cout << "Error: Vertex shader compilation failed\n" << infoLog << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cout << "Error: Fragment shader compilation failed\n" << infoLog << std::endl;
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cout << "Error: Shader program linking failed\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return 0;
}