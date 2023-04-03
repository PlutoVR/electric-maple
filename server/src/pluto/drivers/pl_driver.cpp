// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Pluto HMD device
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Ryan Pavlik <ryan.pavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @ingroup drv_sample
 */

#include "xrt/xrt_device.h"

#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_distortion_mesh.h"

#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <thread>

#include "pluto.pb.h"
#include "pb_decode.h"


extern "C" {

/*
 *
 * Structs and defines.
 *
 */

/*!
 * A sample HMD device.
 *
 * @implements xrt_device
 */
struct pluto_hmd
{
	struct xrt_device base;

	struct xrt_pose pose;

	// renameto: bind_sockfd
	int server_socket_fd;
	struct sockaddr_in server_socket_address;

	// renameto: client_sockfd
	int client_socket_fd;
	struct sockaddr_in client_socket_address;

	std::thread aaaaa = {};

	enum u_logging_level log_level;
};


/// Casting helper function
static inline struct pluto_hmd *
pluto_hmd(struct xrt_device *xdev)
{
	return (struct pluto_hmd *)xdev;
}

DEBUG_GET_ONCE_LOG_OPTION(sample_log, "PLUTO_LOG", U_LOGGING_WARN)

#define PL_TRACE(p, ...) U_LOG_XDEV_IFL_T(&p->base, p->log_level, __VA_ARGS__)
#define PL_DEBUG(p, ...) U_LOG_XDEV_IFL_D(&p->base, p->log_level, __VA_ARGS__)
#define PL_ERROR(p, ...) U_LOG_XDEV_IFL_E(&p->base, p->log_level, __VA_ARGS__)

static void
pluto_hmd_destroy(struct xrt_device *xdev)
{
	struct pluto_hmd *ph = pluto_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(ph);

	u_device_free(&ph->base);
}

static void
pluto_hmd_update_inputs(struct xrt_device *xdev)
{
	// Empty, you should put code to update the attached input fields (if any)
}

static void
pluto_hmd_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           uint64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
	struct pluto_hmd *ph = pluto_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		PL_ERROR(ph, "unknown input name");
		return;
	}

	// Estimate pose at timestamp at_timestamp_ns!
	math_quat_normalize(&ph->pose.orientation);
	out_relation->pose = ph->pose;
	out_relation->relation_flags = (enum xrt_space_relation_flags)(XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
	                                                               XRT_SPACE_RELATION_POSITION_VALID_BIT |
	                                                               XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT);
}

static void
pluto_hmd_get_view_poses(struct xrt_device *xdev,
                         const struct xrt_vec3 *default_eye_relation,
                         uint64_t at_timestamp_ns,
                         uint32_t view_count,
                         struct xrt_space_relation *out_head_relation,
                         struct xrt_fov *out_fovs,
                         struct xrt_pose *out_poses)
{
	u_device_get_view_poses(xdev, default_eye_relation, at_timestamp_ns, view_count, out_head_relation, out_fovs,
	                        out_poses);
}

void
accept_client_connection(struct pluto_hmd &ph)
{
	socklen_t clilen = sizeof(ph.client_socket_address);

	U_LOG_E("Waiting for client connection...");

	ph.client_socket_fd = accept(ph.server_socket_fd, (struct sockaddr *)&ph.client_socket_address, &clilen);
	U_LOG_E("Got client connection!");
}


//!@todo So cursed
void
run_comms_thread(struct pluto_hmd *ph_ptr)
{
	struct pluto_hmd &ph = *ph_ptr;
	accept_client_connection(ph);
	while (true) {

		pb_byte_t server_message_bytes[8192] = {};



		int n = recv(ph.client_socket_fd, server_message_bytes, 8192 - 1, 0);


		// !!!HACK!!! TCP SOCK_STREAM sockets don't guarantee any delineation between messages, it's just
		// emergent behaviour that we usually get one packet at a time.
		// There's no guarantee this'll work; it just usually does on eg. home networks. We have to figure out
		// how to do this correctly once we're on the WebRTC data_channel.
		if (n != pluto_TrackingMessage_size) {
			U_LOG_E("Message of wrong size %d! Expected %d! You probably have bad network conditions.", n,
			        pluto_TrackingMessage_size);
			continue;
		}

		pluto_TrackingMessage message = pluto_TrackingMessage_init_default;

		pb_istream_t our_istream = pb_istream_from_buffer(server_message_bytes, n);


#if 0
		// Seems to always fail with `zero tag`.
		bool result = pb_decode(&our_istream, pluto_TrackingMessage_fields, &message);
#else
		// I don't understand why this works and not the above. I don't think I asked for it to be
		// null-terminated on the client side, so really confused.
		bool result =
		    pb_decode_ex(&our_istream, pluto_TrackingMessage_fields, &message, PB_DECODE_NULLTERMINATED);
#endif



		if (!result) {
			U_LOG_E("Error! %s", PB_GET_ERROR(&our_istream));
			continue;
		}

		ph.pose.position.x = message.P_localSpace_viewSpace.position.x;
		ph.pose.position.x = message.P_localSpace_viewSpace.position.y;
		ph.pose.position.x = message.P_localSpace_viewSpace.position.z;

		ph.pose.orientation.w = message.P_localSpace_viewSpace.orientation.w;
		ph.pose.orientation.x = message.P_localSpace_viewSpace.orientation.x;
		ph.pose.orientation.y = message.P_localSpace_viewSpace.orientation.y;
		ph.pose.orientation.z = message.P_localSpace_viewSpace.orientation.z;
	}
}


void
make_connect_socket(struct pluto_hmd &ph)
{

#if 0
	const char *HARDCODED_IP = "192.168.69.168";
#else
	const char *HARDCODED_IP = "127.0.0.1";
#endif


	socklen_t clilen;
	// char *buffer = (char *)malloc(BUFSIZE);

	// struct sockaddr_in serv_addr, cli_addr;
	// int n;
#if 1
	ph.server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
#else
	// Doesn't work :(
	ph.server_socket_fd = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
#endif

	if (ph.server_socket_fd < 0) {
		perror("socket");
		exit(1);
	}

	int flag = 1;
	// SO_REUSEADDR makes the OS reap this socket right after we quit.
	setsockopt(ph.server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (ph.server_socket_fd < 0) {
		perror("setsockopt");
		exit(1);
	}

	socklen_t addrlen = sizeof(ph.server_socket_address);


	ph.server_socket_address.sin_family = AF_INET;
	ph.server_socket_address.sin_port = htons(61943); // Randomly chosen, doesn't mean anything


	if (inet_pton(AF_INET, HARDCODED_IP, &ph.server_socket_address.sin_addr) <= 0) {
		perror("inet_pton");
		exit(1);
	}


	if (bind(ph.server_socket_fd, (struct sockaddr *)&ph.server_socket_address, addrlen) < 0) {
		perror("bind");
		exit(1);
	}


	//!@todo This allows for 128 pending connections. We only really need one.
	if (listen(ph.server_socket_fd, 128) < 0) {
		perror("listen");
		exit(1);
	}
	// clilen = sizeof(cli_addr);
}



struct xrt_device *
pluto_hmd_create(void)
{
	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	struct pluto_hmd *ph = U_DEVICE_ALLOCATE(struct pluto_hmd, flags, 1, 0);

	// This list should be ordered, most preferred first.
	size_t idx = 0;
	ph->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	ph->base.hmd->blend_mode_count = idx;

	ph->base.update_inputs = pluto_hmd_update_inputs;
	ph->base.get_tracked_pose = pluto_hmd_get_tracked_pose;
	ph->base.get_view_poses = pluto_hmd_get_view_poses;
	ph->base.destroy = pluto_hmd_destroy;

	ph->pose = (struct xrt_pose)XRT_POSE_IDENTITY;
	ph->log_level = debug_get_log_option_sample_log();

	// Print name.
	snprintf(ph->base.str, XRT_DEVICE_NAME_LEN, "Pluto HMD");
	snprintf(ph->base.serial, XRT_DEVICE_NAME_LEN, "Pluto HMD S/N");

	// Setup input.
	ph->base.name = XRT_DEVICE_GENERIC_HMD;
	ph->base.device_type = XRT_DEVICE_TYPE_HMD;
	ph->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	ph->base.orientation_tracking_supported = true;
	ph->base.position_tracking_supported = false;

	// TODO: Find out the framerate that the remote device runs at
	ph->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 90.0f);

	// TODO: Find out the remote device's actual FOV. Or maybe remove this because I think get_view_poses lets us
	// set the FOV dynamically.
	const double hFOV = 90 * (M_PI / 180.0);
	const double vFOV = 96.73 * (M_PI / 180.0);
	// center of projection
	const double hCOP = 0.529;
	const double vCOP = 0.5;
	if (
	    /* right eye */
	    !math_compute_fovs(1, hCOP, hFOV, 1, vCOP, vFOV, &ph->base.hmd->distortion.fov[1]) ||
	    /*
	     * left eye - same as right eye, except the horizontal center of projection is moved in the opposite
	     * direction now
	     */
	    !math_compute_fovs(1, 1.0 - hCOP, hFOV, 1, vCOP, vFOV, &ph->base.hmd->distortion.fov[0])) {
		// If those failed, it means our math was impossible.
		PL_ERROR(ph, "Failed to setup basic device info");
		pluto_hmd_destroy(&ph->base);
		return NULL;
	}
	// TODO: Ditto, figure out the device's actual resolution
	const int panel_w = 1080;
	const int panel_h = 1200;

	// Single "screen" (always the case)
	ph->base.hmd->screens[0].w_pixels = panel_w * 2;
	ph->base.hmd->screens[0].h_pixels = panel_h;

	// Left, Right
	for (uint8_t eye = 0; eye < 2; ++eye) {
		ph->base.hmd->views[eye].display.w_pixels = panel_w;
		ph->base.hmd->views[eye].display.h_pixels = panel_h;
		ph->base.hmd->views[eye].viewport.y_pixels = 0;
		ph->base.hmd->views[eye].viewport.w_pixels = panel_w;
		ph->base.hmd->views[eye].viewport.h_pixels = panel_h;
		// if rotation is not identity, the dimensions can get more complex.
		ph->base.hmd->views[eye].rot = u_device_rotation_ident;
	}
	// left eye starts at x=0, right eye starts at x=panel_width
	ph->base.hmd->views[0].viewport.x_pixels = 0;
	ph->base.hmd->views[1].viewport.x_pixels = panel_w;

	// TODO: Doing anything wiht distortion here makes no sense
	u_distortion_mesh_set_none(&ph->base);

	// TODO: Are we going to have any actual useful info to show here?
	// Setup variable tracker: Optional but useful for debugging
	u_var_add_root(ph, "Pluto HMD", true);
	u_var_add_pose(ph, &ph->pose, "pose");
	u_var_add_log_level(ph, &ph->log_level, "log_level");

	make_connect_socket(*ph);

	ph->aaaaa = std::thread(run_comms_thread, ph);


	return &ph->base;
}
} // extern "C"
