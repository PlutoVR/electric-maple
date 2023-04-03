# linux-streaming


## Structure
This repo has _two_ main projects:
* `server` - a regular desktop/server Linux CMake build. To do development on the server, open the `server` folder with your IDE of choice and build it like a regular CMake project. We're not doing anything crazy in that repo, should just work.

* `client` - an Android-only Gradle/CMake project. (It should take maybe a couple hours to port to desktop Linux.) To do development on the client, open the `client` folder with Android Studio and let it do its thing.

We also have 


For now, use adb port *reversing*. It's way easier than hardcoding your IP into the code, and you need a cable to keep your Quest charged. Run this:
```
adb reverse tcp:61943 tcp:61943
```
and your Quest should be able to connect to the server on your PC.

