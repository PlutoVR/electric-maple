// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Pluto HMD device
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
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

#include "pl_server_internal.h"



void
accept_client_connection(struct pluto_program &ph)
{
	socklen_t clilen = sizeof(ph.client_socket_address);

	U_LOG_E("Waiting for client connection...");
	fd_set set;
	int ret = 0;

	while (!ph.comms_thread_should_stop && ret == 0) {
		// Get the socket.
		int socket = ph.server_socket_fd;

		// Select can modify timeout, reset each loop.
		struct timeval timeout = {};
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		// Reset each loop.
		FD_ZERO(&set);
		FD_SET(socket, &set);

		ret = select(socket + 1, &set, NULL, NULL, &timeout);
	}

	if (ret < 0) {
		U_LOG_E("select: %i", ret);
		return;
	} else if (ret > 0) {
		// Ok!
	} else {
		U_LOG_E("select: %i", ret);
		return;
	}

	ph.client_socket_fd = accept(ph.server_socket_fd, (struct sockaddr *)&ph.client_socket_address, &clilen);

	U_LOG_E("Got client connection!");
}


//!@todo So cursed
void
run_comms_thread(struct pluto_program *ph_ptr)
{
	struct pluto_program &ph = *ph_ptr;
	accept_client_connection(ph);
	while (!ph.comms_thread_should_stop) {

		pb_byte_t server_message_bytes[8192] = {};



		int n = recv(ph.client_socket_fd, server_message_bytes, 8192 - 1, 0);

		if (n == 0) {
			U_LOG_E("Client disconnected!");
			accept_client_connection(ph);
		}


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

		ph.head->pose.position.x = message.P_localSpace_viewSpace.position.x;
		ph.head->pose.position.y = message.P_localSpace_viewSpace.position.y;
		ph.head->pose.position.z = message.P_localSpace_viewSpace.position.z;

		ph.head->pose.orientation.w = message.P_localSpace_viewSpace.orientation.w;
		ph.head->pose.orientation.x = message.P_localSpace_viewSpace.orientation.x;
		ph.head->pose.orientation.y = message.P_localSpace_viewSpace.orientation.y;
		ph.head->pose.orientation.z = message.P_localSpace_viewSpace.orientation.z;
	}
}


void
make_connect_socket(struct pluto_program &ph)
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
