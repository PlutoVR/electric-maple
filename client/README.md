# ElectricMaple XR streaming client

<!--
Copyright 2023, Collabora, Ltd.

SPDX-License-Identifier: CC-BY-4.0
-->

This is a pretty normal Gradle+CMake build for Android. The only caveats are
that you need a recent GStreamer 1.22.x (at least 1.22.5 iirc) build for Android
available, and that you need `libeigen3-dev` installed on your system.

## Build dep: Eigen 3

If the Eigen headers aren't in `/usr/include/eigen3`, then specify
`eigenIncludeDir=/home/user/eigen3` changing the path as appropriate.

## Build dep: GStreamer

You can get an upstream build of GStreamer by running `./download_gst.sh` which
will extract it to `deps/gstreamer_android`. This is the default search
location. If you are intending to use a different build (such as a local build
from Cerbero), you will need to set one of these in `local.properties`:

- `gstreamerArchDir=/home/user/src/cerbero/build/dist/android_arm64` - if you've
  done a single-arch cerbero build in `~/src/cerbero`
- `gstreamerBaseDir=/home/user/gstreamer_android_universal` - if you have a
  universal (all architectures) build like the one downloaded by the script.

## Directories

- Gradle or Java/Kotlin
  - `app` - Main app gradle module, with a customized NativeActivity and
    Application that calls our startup routines. (No native code in here, it's
    elsewhere.)
  - `gstreamer_java` - A Gradle module to grab the required Java classes from the
    GStreamer tree and build them. There are some Java classes required for the
    hardware decoder usage (callback classes)
- Native (C/C++) code
  - `gstreamer_android` - A CMake sub-build to build the GStreamer (and related)
    dependency static libraries into a single `.so` with initialization code.
  - `src` - Root of main sources. Files directly in here are the main app.
    - `src/em` - The ElectricMaple XR streaming client module. This part can be
      reused by other projects.
  - `egl` - Some EGL utilities used by EM. Your client app code will also need
    these to control access to the EGL context.
  - `tests` - Native tests that can be run if you build on desktop rather than Android.
- Other:
  - `cmake` - Additional helper modules for CMake.
  - `deps` - where `./download_gst.sh` will put GStreamer
  - `scripts` - some possibly-outdated scripts.
