// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Wrapper around OpenGL (ES) swapchain
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_utils
 */

#pragma once

#include "xr_platform_deps.h"

#include <openxr/openxr_platform.h>
#include <vector>

#ifdef __ANDROID__
using XrSwapchainImageForGL = XrSwapchainImageOpenGLESKHR;
static constexpr XrStructureType kGLSwapchainImageType = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#else
using XrSwapchainImageForGL = XrSwapchainImageOpenGLKHR;
static constexpr XrStructureType kGLSwapchainImageType = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;

#endif

/**
 * Wraps the native OpenGL texture object names and associated framebuffers for an OpenXR swapchain.
 */
class GLSwapchain
{
public:
	GLSwapchain() = default;
	// non-copyable (but move OK)
	GLSwapchain(const GLSwapchain &) = delete;
	GLSwapchain &
	operator=(const GLSwapchain &) = delete;

	// destructor calls reset
	~GLSwapchain();

	/// Enumerate the swapchain images and generate/associate framebuffer object names with each
	bool
	enumerateAndGenerateFramebuffers(XrSwapchain swapchain);

	/// Get the number of images in the swapchain
	uint32_t
	size() const noexcept
	{
		// cast OK because the OpenXR API returns it as a uint32_t
		return static_cast<uint32_t>(swapchainImages_.size());
	}

	/// Release the generated framebuffer object names.
	void
	reset();

	/// Access the GL framebuffer object name associated with swapchain image @p i
	GLuint
	framebufferNameAtSwapchainIndex(uint32_t i) const
	{
		return framebuffers_.at(i);
	}

private:
	static constexpr XrStructureType kSwapchainImageType = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
	std::vector<XrSwapchainImageOpenGLESKHR> swapchainImages_;
	std::vector<GLuint> framebuffers_;
};
