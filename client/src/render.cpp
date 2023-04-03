// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer
 * render.
 * @author Moshi Turner <moses@collabora.com>
 */

#include "common.hpp"

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

// These two functions aren't important, just draw a triangle. We will remove them later

GLuint
makeShaderProgram()
{
	// Vertex shader program
	const char *vertex_shader_source =
	    "#version 300 es\n"
	    "in vec3 position;\n"
	    "in vec4 color;\n"
	    "out vec4 vertex_color;\n"
	    "void main()\n"
	    "{\n"
	    "    gl_Position = vec4(position, 1.0);\n"
	    "    vertex_color = color;\n"
	    "}\n";

	// Fragment shader program
	const char *fragment_shader_source =
	    "#version 300 es\n"
	    "precision mediump float;\n"
	    "in vec4 vertex_color;\n"
	    "out vec4 frag_color;\n"
	    "void main()\n"
	    "{\n"
	    "    frag_color = vertex_color;\n"
	    "}\n";


	// Create the vertex shader object
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	// Attach the vertex shader source code and compile it
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	// Check for any compile errors
	GLint success;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		U_LOG_E("Vertex shader compilation failed: %s\n", info_log);
	}

	// Create the fragment shader object
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach the fragment shader source code and compile it
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	// Check for any compile errors
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		U_LOG_E("Fragment shader compilation failed: %s\n", info_log);
	}

	// Create the shader program object
	GLuint shader_program = glCreateProgram();

	// Attach the vertex and fragment shaders to the program and link it
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	// Check for any link errors
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(shader_program, 512, NULL, info_log);
		U_LOG_E("Shader program linking failed: %s\n", info_log);
	}

	// Clean up the shader objects
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}


void
drawTriangle(GLuint shader_program)
{
	// Define the vertices of the triangle
	GLfloat vertices[] = {
	    0.0f,  0.5f,  0.0f, // top
	    -0.5f, -0.5f, 0.0f, // bottom left
	    0.5f,  -0.5f, 0.0f  // bottom right
	};

	// Define the colors of the vertices
	GLfloat colors[] = {
	    1.0f, 0.0f, 0.0f, 1.0f, // top vertex is red
	    0.0f, 1.0f, 0.0f, 1.0f, // bottom left vertex is green
	    0.0f, 0.0f, 1.0f, 1.0f  // bottom right vertex is blue
	};

	// Use the shader program
	glUseProgram(shader_program);

	// Create vertex buffer objects for the vertices and colors
	GLuint vbo[2];
	glGenBuffers(2, vbo);

	// Bind the vertex buffer for the vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind the vertex buffer for the colors
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

	// Enable the vertex attribute for the vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Enable the vertex attribute for the colors
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Draw the triangle
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Check for errors
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		U_LOG_E("OpenGL error: %d\n", error);
	}
}
