// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Electric Maple Server motion controller device
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @ingroup drv_ems
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



#include "electricmaple.pb.h"
#include "pb_decode.h"

#include "ems_server_internal.h"

#include <thread>

#include <cstdio>
#include <cassert>



/*
 *
 * Structs and defines.
 *
 */


/// Casting helper function
static inline struct ems_motion_controller *
ems_motion_controller(struct xrt_device *xdev)
{
	return (struct ems_motion_controller *)xdev;
}

DEBUG_GET_ONCE_LOG_OPTION(sample_log, "EMS_LOG", U_LOGGING_WARN)

#define EMS_TRACE(p, ...) U_LOG_XDEV_IFL_T(&p->base, p->log_level, __VA_ARGS__)
#define EMS_DEBUG(p, ...) U_LOG_XDEV_IFL_D(&p->base, p->log_level, __VA_ARGS__)
#define EMS_ERROR(p, ...) U_LOG_XDEV_IFL_E(&p->base, p->log_level, __VA_ARGS__)

static void
controller_destroy(struct xrt_device *xdev)
{
	struct ems_motion_controller *emc = ems_motion_controller(xdev);

	// Remove the variable tracking.
	u_var_remove_root(emc);

	u_device_free(&emc->base);
}

static void
controller_update_inputs(struct xrt_device *xdev)
{
	// Empty, you should put code to update the attached input fields (if any)
}

static void
controller_set_output(struct xrt_device *xdev, enum xrt_output_name name, const union xrt_output_value *value)
{
	// Since we don't have a data channel yet, this is a no-op.
}

static void
controller_get_tracked_pose(struct xrt_device *xdev,
                            enum xrt_input_name name,
                            uint64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	struct ems_motion_controller *emc = ems_motion_controller(xdev);

	switch (name) {
	case XRT_INPUT_TOUCH_GRIP_POSE:
	case XRT_INPUT_TOUCH_AIM_POSE: break;
	default: EMS_ERROR(emc, "unknown input name"); return;
	}

	// Estimate pose at timestamp at_timestamp_ns!
	math_quat_normalize(&emc->pose.orientation);
	out_relation->pose = emc->pose;
	out_relation->relation_flags = (enum xrt_space_relation_flags)( //
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |                  //
	    XRT_SPACE_RELATION_POSITION_VALID_BIT |                     //
	    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |                //
	    XRT_SPACE_RELATION_POSITION_TRACKED_BIT);                   //
}

static void
controller_get_view_poses(struct xrt_device *xdev,
                          const struct xrt_vec3 *default_eye_relation,
                          uint64_t at_timestamp_ns,
                          uint32_t view_count,
                          struct xrt_space_relation *out_head_relation,
                          struct xrt_fov *out_fovs,
                          struct xrt_pose *out_poses)
{
	assert(false);
}


/*
 *
 * Bindings
 *
 */

static struct xrt_binding_input_pair simple_inputs_touch[4] = {
    {XRT_INPUT_SIMPLE_SELECT_CLICK, XRT_INPUT_TOUCH_TRIGGER_VALUE},
    {XRT_INPUT_SIMPLE_MENU_CLICK, XRT_INPUT_TOUCH_MENU_CLICK},
    {XRT_INPUT_SIMPLE_GRIP_POSE, XRT_INPUT_TOUCH_GRIP_POSE},
    {XRT_INPUT_SIMPLE_AIM_POSE, XRT_INPUT_TOUCH_AIM_POSE},
};

static struct xrt_binding_output_pair simple_outputs_touch[1] = {
    {XRT_OUTPUT_NAME_SIMPLE_VIBRATION, XRT_OUTPUT_NAME_TOUCH_HAPTIC},
};

static struct xrt_binding_profile binding_profiles_touch[1] = {
    {
        .name = XRT_DEVICE_SIMPLE_CONTROLLER,
        .inputs = simple_inputs_touch,
        .input_count = ARRAY_SIZE(simple_inputs_touch),
        .outputs = simple_outputs_touch,
        .output_count = ARRAY_SIZE(simple_outputs_touch),
    },
};


/*
 *
 * 'Exported' functions.
 *
 */

struct ems_motion_controller *
ems_motion_controller_create(ems_instance &emsi, enum xrt_device_name device_name, enum xrt_device_type device_type)
{
	uint32_t input_count = 0;
	uint32_t output_count = 0;
	switch (device_name) {
	case XRT_DEVICE_TOUCH_CONTROLLER:
		input_count = 14;
		output_count = 1;
		break;
	default: U_LOG_E("Device name not supported!"); return nullptr;
	}

	const char *hand_str = nullptr;
	xrt_pose default_pose;
	switch (device_type) {
	case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER:
		hand_str = "Right";
		default_pose = (xrt_pose){XRT_QUAT_IDENTITY, {0.2f, 1.4f, -0.4f}};
		break;
	case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER:
		hand_str = "Left";
		default_pose = (xrt_pose){XRT_QUAT_IDENTITY, {-0.2f, 1.4f, -0.4f}};
		break;
	default: U_LOG_E("Device type not supported!"); return nullptr;
	}

	// We don't need anything special from allocate except inputs and outputs.
	u_device_alloc_flags flags{};
	struct ems_motion_controller *emc =
	    U_DEVICE_ALLOCATE(struct ems_motion_controller, flags, input_count, output_count);

	// Functions.
	emc->base.update_inputs = controller_update_inputs;
	emc->base.set_output = controller_set_output;
	emc->base.get_tracked_pose = controller_get_tracked_pose;
	emc->base.get_view_poses = controller_get_view_poses;
	emc->base.destroy = controller_destroy;

	// Data.
	emc->base.tracking_origin = &emsi.tracking_origin;
	emc->base.binding_profiles = binding_profiles_touch;
	emc->base.binding_profile_count = ARRAY_SIZE(binding_profiles_touch);
	emc->base.orientation_tracking_supported = true;
	emc->base.position_tracking_supported = true;
	emc->base.name = device_name;
	emc->base.device_type = device_type;

	// Private fields.
	emc->instance = &emsi;
	emc->pose = default_pose;
	emc->log_level = debug_get_log_option_sample_log();

	// Print name.
	snprintf(emc->base.str, XRT_DEVICE_NAME_LEN, "Touch %s Controller (Electric Maple)", hand_str);
	snprintf(emc->base.serial, XRT_DEVICE_NAME_LEN, "N/A S/N");


	// Setup input.
	switch (device_name) {
	case XRT_DEVICE_TOUCH_CONTROLLER:
		emc->base.inputs[0].name = XRT_INPUT_TOUCH_SQUEEZE_VALUE;
		emc->base.inputs[1].name = XRT_INPUT_TOUCH_TRIGGER_TOUCH;
		emc->base.inputs[2].name = XRT_INPUT_TOUCH_TRIGGER_VALUE;
		emc->base.inputs[3].name = XRT_INPUT_TOUCH_THUMBSTICK_CLICK;
		emc->base.inputs[4].name = XRT_INPUT_TOUCH_THUMBSTICK_TOUCH;
		emc->base.inputs[5].name = XRT_INPUT_TOUCH_THUMBSTICK;
		emc->base.inputs[6].name = XRT_INPUT_TOUCH_THUMBREST_TOUCH;
		emc->base.inputs[7].name = XRT_INPUT_TOUCH_GRIP_POSE;
		emc->base.inputs[8].name = XRT_INPUT_TOUCH_AIM_POSE;

		if (device_type == XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER) {
			emc->base.inputs[9].name = XRT_INPUT_TOUCH_X_CLICK;
			emc->base.inputs[10].name = XRT_INPUT_TOUCH_X_TOUCH;
			emc->base.inputs[11].name = XRT_INPUT_TOUCH_Y_CLICK;
			emc->base.inputs[12].name = XRT_INPUT_TOUCH_Y_TOUCH;
			emc->base.inputs[13].name = XRT_INPUT_TOUCH_MENU_CLICK;
		} else {
			emc->base.inputs[9].name = XRT_INPUT_TOUCH_A_CLICK;
			emc->base.inputs[10].name = XRT_INPUT_TOUCH_A_TOUCH;
			emc->base.inputs[11].name = XRT_INPUT_TOUCH_B_CLICK;
			emc->base.inputs[12].name = XRT_INPUT_TOUCH_B_TOUCH;
			emc->base.inputs[13].name = XRT_INPUT_TOUCH_SYSTEM_CLICK;
		}

		emc->base.outputs[0].name = XRT_OUTPUT_NAME_TOUCH_HAPTIC;
		break;
	default: assert(false);
	}

	// Lastly setup variable tracking.
	u_var_add_root(emc, emc->base.str, true);
	u_var_add_pose(emc, &emc->pose, "pose");
	u_var_add_log_level(emc, &emc->log_level, "log_level");

	return emc;
}

// Has to be standard layout because of first element casts we do.
static_assert(std::is_standard_layout<struct ems_motion_controller>::value);
