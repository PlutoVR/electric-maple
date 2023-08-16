// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Wrapper around OpenGL (ES) swapchain
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_utils
 */

#include "GLSwapchain.h"
#include "../em_app_log.h"
#include <GLES3/gl3.h>
#include <openxr/openxr.h>

#include <cassert>


GLSwapchain::~GLSwapchain()
{
	reset();
}

bool
GLSwapchain::enumerateAndGenerateFramebuffers(XrSwapchain swapchain)
{
	assert(swapchainImages_.empty());
	assert(framebuffers_.empty());
	uint32_t countOutput = 0;
	if (!XR_UNQUALIFIED_SUCCESS(xrEnumerateSwapchainImages(swapchain, 0, &countOutput, nullptr))) {
		ALOGE("%s: Failed initial call to xrEnumerateSwapchainImages", __FUNCTION__);
		return false;
	}
	swapchainImages_.resize(countOutput, {kSwapchainImageType});
	XrResult result = xrEnumerateSwapchainImages(swapchain, countOutput, &countOutput,
	                                             (XrSwapchainImageBaseHeader *)swapchainImages_.data());
	if (!XR_UNQUALIFIED_SUCCESS(result)) {
		ALOGE("%s: Failed second call to xrEnumerateSwapchainImages", __FUNCTION__);
		swapchainImages_.clear();
		return false;
	}
	// defensive coding: the array size should not have changed between the two calls but safest to truncate here
	// just in case.
	swapchainImages_.resize(countOutput);

	const GLsizei n = static_cast<GLsizei>(countOutput);

	ALOGI("%s: Generating framebuffers", __FUNCTION__);
	framebuffers_.resize(n);
	glGenFramebuffers(n, framebuffers_.data());

	bool success = true;
	for (GLsizei i = 0; i < n; ++i) {
		ALOGI("%s: Index %d: Binding framebuffer name %d to texture ID %d", __FUNCTION__, i, framebuffers_[i],
		      swapchainImages_[i].image);
		// bind this name as the active framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffers_[i]);
		// associate a swapchain image as the texture object/image for this framebuffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swapchainImages_[i].image,
		                       0);
		// check to make sure we can actually render to this.
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			ALOGE("%s: Index %d: Failed to create framebuffer (%d)\n", __FUNCTION__, i, status);
			success = false;
			break;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!success) {
		ALOGW("%s: Cleaning up framebuffer names after an error in associating textures", __FUNCTION__);
		reset();
		return false;
	}
	return true;
}

void
GLSwapchain::reset()
{
	if (framebuffers_.empty()) {
		return;
	}
	glDeleteFramebuffers(static_cast<GLsizei>(framebuffers_.size()), framebuffers_.data());
	framebuffers_.clear();
	swapchainImages_.clear();
}
