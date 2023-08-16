// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 * @author Rylie Pavlik <rpavlik@collabora.com>
 */

#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	Renderer(const Renderer &) = delete;
	Renderer(Renderer &&) = delete;

	Renderer &
	operator=(const Renderer &) = delete;
	Renderer &
	operator=(Renderer &&) = delete;

	/// Create resources. Must call with EGL Context current
	void
	setupRender();

	/// Destroy resources. Must call with EGL context current.
	void
	reset();

	/// Draw texture to framebuffer. Must call with EGL Context current.
	void
	draw(GLuint texture, GLenum texture_target) const;


private:
	void
	setupShaders();
	void
	setupQuadVertexData();

	GLuint program = 0;
	GLuint quadVAO = 0;
	GLuint quadVBO = 0;

	GLint textureSamplerLocation_ = 0;
};
