import android.app.Application;
class StreamingApplication :Application()  {

    init {
        System.loadLibrary("pluto_vf")
        System.loadLibrary("pluto_webrtc_runtime")
    }
}
