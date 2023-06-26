I will be distributing my Gstreamer PATCH to ryan, but I am too
worried that some variability slips in the process due to high
complexity. Therefore I am distributing my  gstreamer-android
prebuilts (arm64) in the below tarball :

https://share.collabora.com/index.php/f/3753760
    
WARNING : The gstreamer_android deps are fetched by running the
../download_gst.sh script. The gstreamer version we're using is
version 1.22.3 BUT NOTE THAT SOME FILES ARE TAINTED, as I had
to manually fix some stuff.
    
For it, I manually cloned the gstreamer_android repo with CERBERO
https://gstreamer.freedesktop.org/documentation/installing/building-from-source-using-cerbero.html?gi-language=c
and I built for arm64 following the instructions.
    
THEN, I touched/fixed some files and those diffs are in the
below patch, also checked-in with this commit :
deps/0001-GSTREAMER_ANDROID_FIXES-FOR-PROPER-AMC-INITIALIZATION.patch
   
I then copied the tainted .a files in the gstreamer_android folder and tarballed it... Please untar the tarball OVER your existing gstreamer_android.

Of course, for a fresh start, simply remove
deps/gstreamer_android and run the download_gst.sh script again, but
you'll probably lose the ability to init. AMC plugin.
    
G'luck.

