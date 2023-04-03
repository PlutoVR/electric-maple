#include "math/m_space.h"
#include "stereokit.h"
#include "stereokit_ui.h"
#include "xrt/xrt_defines.h"
#include "util/u_logging.h"
#include "pluto.pb.h"
using namespace sk;

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct state
{
	bool palm_pose;


	// renameto: bind_sockfd
	int socket_fd;
	struct sockaddr_in socket_addr;
};

// todo: what exactly does this do? this won't make sense on android
void
error(const char *msg)
{
	perror(msg);
	exit(1);
}

// Inefficient, but we don't care.
xrt_pose
xrt_from_sk(sk::pose_t sk)
{
	xrt_pose ret;
	ret.position.x = sk.position.x;
	ret.position.y = sk.position.y;
	ret.position.z = sk.position.z;

	ret.orientation.x = sk.orientation.x;
	ret.orientation.y = sk.orientation.y;
	ret.orientation.z = sk.orientation.z;

	ret.orientation.w = sk.orientation.w;
	return ret;
}



void
run(void *ptr)
{
	struct state &st = *(state *)ptr;

	sk::handed_ hands[2] = {sk::handed_left, sk::handed_right};
	const char *names[2] = {"Left", "Right"};

	pluto::TrackingMessage message = {};

	const sk::pose_t *head_sk = sk::input_head();

	xrt_pose head = xrt_from_sk(*head_sk);



	message.mutable_p_localspace_viewspace()->mutable_position()->set_x(head.position.x);
	message.mutable_p_localspace_viewspace()->mutable_position()->set_y(head.position.y);
	message.mutable_p_localspace_viewspace()->mutable_position()->set_z(head.position.z);

	message.mutable_p_localspace_viewspace()->mutable_orientation()->set_w(head.orientation.w);
	message.mutable_p_localspace_viewspace()->mutable_orientation()->set_x(head.orientation.x);
	message.mutable_p_localspace_viewspace()->mutable_orientation()->set_y(head.orientation.y);
	message.mutable_p_localspace_viewspace()->mutable_orientation()->set_z(head.orientation.z);

	size_t size = message.ByteSizeLong();

	U_LOG_E("Size is %zu", size);

	char *array = new char[size];

	message.SerializeToArray(array, size);

	int iResult = send(st.socket_fd, array, size, 0);
}


void
really_make_socket(struct state &st)
{
	st.socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Connect to the parent process
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;

	// todo: use inet_pton
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(61943);

	int iResult = connect(st.socket_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

	if (iResult < 0) {
		U_LOG_E("Bad!");
		perror("rawr");
	}

	U_LOG_E("Result is %d", iResult);
}

int
main()
{
	sk_settings_t settings = {};
	settings.app_name = "StereoKit C";
	settings.assets_folder = "/2/XR/skNotes/Assets";
	settings.display_preference = display_mode_mixedreality;
	settings.overlay_app = true;
	settings.overlay_priority = 1;
	if (!sk_init(settings))
		return 1;

	struct state &st = *(new state);

	really_make_socket(st);


	sk_run_data(run, &st, run, &st);

	return 0;
}
