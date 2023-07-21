import android.app.Application;
import org.freedesktop.gstreamer.GStreamer

class StreamingApplication :Application()  {
    override fun onCreate() {
        super.onCreate()
    }
    init {
        System.loadLibrary("pluto_vf")
        System.loadLibrary("pluto_webrtc_runtime")
    }
}
