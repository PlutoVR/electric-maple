# Pluto ElectricMaple streaming server

<!--
Copyright 2023, Collabora, Ltd.

SPDX-License-Identifier: CC-BY-4.0
-->

This is a normal CMake build. It does depend on the submodules in this repo.

From this (the server directory) something like the following will work on Linux:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build
```

## Build Dependencies

- libeigen3-dev
- gstreamer1.0-plugins-good
- gstreamer1.0-nice
- libgstreamer-plugins-bad1.0-dev
- libgstreamer-plugins-base1.0-dev
- libgstreamer1.0-dev
- glslang-tools
- libbsd-dev
- libgl1-dev
- libsystemd-dev
- libvulkan-dev
- libx11-dev
- libx11-xcb-dev
- libxxf86vm-dev
- pkg-config

```sh
sudo apt install libeigen3-dev \
    gstreamer1.0-plugins-good \
    gstreamer1.0-nice \
    libgstreamer-plugins-bad1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-dev \
    glslang-tools \
    libbsd-dev \
    libgl1-dev \
    libsystemd-dev \
    libvulkan-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxxf86vm-dev \
    pkg-config
```

Plus some version of libsoup. Run the following, and see if it shows
`libsoup-3.0-0` at the end:

```sh
apt info gstreamer1.0-plugins-good | grep libsoup-3.0
```

- If so, you can use libsoup3:
  - libsoup-3.0-dev
- Otherwise you must use libsoup 2:
  - libsoup2.4-dev
  - In this case, you must also pass `-DPL_LIBSOUP2=ON` to CMake.

Best to only have one of the two libsoup dev packages installed at a time.

## Test Client

There is a desktop test client built to `build/src/test/webrtc_client` that just
shows the frames on a desktop window, with no upstream data or VR rendering.

## Running

Due to the early stage of the project, you must start this up in this particular order:

- this server
- OpenXR application
- streaming client app

Assuming you followed the build steps above, you can start the server with:

```sh
build/src/pluto/pluto_streaming_server
```

To run an OpenXR app, use the build-tree OpenXR runtime manifest at
`build/openxr_monado-dev.json` by symlinking it to the active runtime path,
using something like XR Picker to do that for you, or:

```sh
env XR_RUNTIME_JSON=$HOME/src/linux-streaming/server/build/openxr_monado-dev.json hello_xr -G vulkan2
```

to apply the active runtime just for a single command. (Change the path to the
build as applicable.)
