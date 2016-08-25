package com.pixite.graphics;

import android.graphics.Bitmap;
import android.util.Log;

public class Masker {

    private final long nativeInstance;

    private final int width, height;

    public Masker(Bitmap src) {
        width = src.getWidth();
        height = src.getHeight();
        nativeInstance = native_init(src);
    }

    public Bitmap getMask(int x, int y) {
        Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ALPHA_8);
        long start = System.currentTimeMillis();
        native_mask(nativeInstance, result, x, y);
        Log.d("Masker", "native_mask(): " + (System.currentTimeMillis() - start) + "ms");
        return result;
    }

    public long uploadMask(int x, int y) {
        return native_upload(nativeInstance, x, y);
    }

    public void reset() {
        native_reset(nativeInstance);
    }

    private native long native_init(Bitmap src);
    private native void native_mask(long nativeInstance, Bitmap result, int x, int y);
    private native long native_upload(long nativeInstance, int x, int y);
    private native void native_reset(long nativeInstance);

    static {
        System.loadLibrary("graphics");
    }

}
