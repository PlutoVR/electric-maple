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
```
your Quest should be able to connect to the server on your PC and you should be able to get going :)

