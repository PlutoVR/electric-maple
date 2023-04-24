# linux-streaming


## Structure
This repo has _two_ main projects:
* `server` - a regular desktop/server Linux CMake build. To do development on the server, open the `server` folder with your IDE of choice and build it like a regular CMake project. We're not doing anything crazy in that repo, should just work.

* `client` - an Android-only Gradle/CMake project. (It should take maybe a couple hours to port to desktop Linux.) To do development on the client, open the `client` folder with Android Studio and let it do its thing.

Also, both projects use the `monado` and `proto` folders (via CMake add_subdirectory) for general dependencies and message encoding/decoding.


## ADB port reversing
For now, we're going to assume everybody's debugging this with a Quest over ADB. The client and server are hard-coded (for now) to port `61943`, so if you run this:
```
adb reverse tcp:61943 tcp:61943
adb reverse tcp:8080 tcp:8080
```
your Quest should be able to connect to the server on your PC and you should be able to get going :)

## Native WebRTC test

Right now, we have a native WebRTC receiver test at `server/test/webrtc_client`. All it does is set up the stream and shove video into an autovideosink.

Not all of the state tracking is set up yet, so the only way it'll work is if you do things in this order:
* Launch pluto_streaming_server
* Launch an OpenXR application (to trigger the GStreamer pipeline and webserver to start up)
* Launch the WebRTC client
