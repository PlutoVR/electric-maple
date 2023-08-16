// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 * @author Rylie Pavlik <rpavlik@collabora.com>
 */

#include "render.hpp"

#include "GLDebug.h"
#include "GLError.h"
#include "../em_app_log.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <openxr/openxr.h>
#include <stdexcept>

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
Renderer::setupShaders()
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
Renderer::setupQuadVertexData()
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

Renderer::~Renderer()
{
	reset();
}

void
Renderer::setupRender()
{


	registerGlDebugCallback();
	setupShaders();
	setupQuadVertexData();
}

void
Renderer::reset()
{
	if (program != 0) {
		glDeleteProgram(program);
		program = 0;
	}
	if (quadVAO != 0) {
		glDeleteVertexArrays(1, &quadVAO);
		quadVAO = 0;
	}
	if (quadVBO != 0) {
		glDeleteBuffers(1, &quadVBO);
		quadVBO = 0;
	}
}

void
Renderer::draw(GLuint texture, GLenum texture_target) const
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

	CHECK_GL_ERROR();
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
}
