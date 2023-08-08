// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Very simple GLES3 renderer for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 * @author Ryan Pavlik <rpavlik@collabora.com>
 */

#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

struct em_state;
struct EglData
{
	/// Creates an R8G8B8A8 ES3 context and pbuffer surface (for some reason)
	EglData();

	/// Calls reset
	~EglData();

	// do not move
	EglData(const EglData &) = delete;
	// do not move
	EglData(EglData &&) = delete;

	// do not copy
	EglData &
	operator=(const EglData &) = delete;
	// do not copy
	EglData &
	operator=(EglData &&) = delete;


	bool
	isReady() const noexcept
	{
		return display != EGL_NO_DISPLAY && context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE;
	}

	void
	makeCurrent() const;

	void
	makeNotCurrent() const;

	EGLDisplay display = EGL_NO_DISPLAY;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLConfig config = nullptr;
};

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
