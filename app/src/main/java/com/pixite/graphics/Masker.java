package com.pixite.graphics;

import android.graphics.Bitmap;
import android.graphics.Rect;

public class Masker {

    private long nativeInstance;

    private final int width, height;

    public Masker(Bitmap src) {
        width = src.getWidth();
        height = src.getHeight();
        nativeInstance = native_init(src);
    }

    public Bitmap getMask(int x, int y) {
        Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ALPHA_8);
        if (x < 0 || x > width || y < 0 || y > height) {
            return result;
        }
        native_mask(nativeInstance, x, y);
        native_download(nativeInstance, result);
        return result;
    }

    public long uploadMask(int x, int y) {
        if (x < 0 || x > width || y < 0 || y > height) {
            return 0;
        }
        long pixels = native_mask(nativeInstance, x, y);
        native_upload(nativeInstance);
        return pixels;
    }

    public void upload() {
        native_upload(nativeInstance);
    }

    public Rect getMaskRect() {
        Rect out = new Rect();
        native_getMaskRect(nativeInstance, out);
        return out;
    }

    public void reset() {
        native_reset(nativeInstance);
    }

    public void clear() {
        native_clear(nativeInstance);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            finalizer(nativeInstance);
            nativeInstance = 0;  // Other finalizers can still call us
        } finally {
            //noinspection ThrowFromFinallyBlock
            super.finalize();
        }
    }

    private native long native_init(Bitmap src);
    private native void native_download(long nativeInstance, Bitmap result);
    private native long native_mask(long nativeInstance, int x, int y);
    private native void native_upload(long nativeInstance);
    private native void native_getMaskRect(long nativeInstance, Rect out);
    private native void native_reset(long nativeInstance);
    private native void native_clear(long nativeInstance);
    private native void finalizer(long nativeInstance);

    static {
        System.loadLibrary("graphics");
    }

}
