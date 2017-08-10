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

    /**
     * Creates a mask based on the pixel coordinates and returns it as a bitmap.
     *
     * @param x The horizontal location of the target point.
     * @param y The vertical location of the target point.
     * @return An ALPHA_8 bitmap containing the mask pixels.
     */
    public Bitmap getMask(int x, int y) {
        Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ALPHA_8);
        if (x < 0 || x > width || y < 0 || y > height) {
            return result;
        }
        native_mask(nativeInstance, x, y);
        native_download(nativeInstance, result);
        return result;
    }

    /**
     * Checks whether the pixel at the given coordinates is part of the current mask.
     *
     * @param x The horizontal location of the target point.
     * @param y The vertical location of the target point.
     * @return True if the pixel is part of the mask, otherwise false.
     */
    public boolean isInMask(int x, int y) {
        return native_isInMask(nativeInstance, x, y);
    }

    /**
     * Creates a mask based on the pixel coordinates and uploads it to the currently
     * bound texture unit, with a GL_ALPHA format.
     *
     * @param x The horizontal location of the target point.
     * @param y The vertical location of the target point.
     * @return The number of pixels contained in the mask.
     */
    public long uploadMask(int x, int y) {
        if (x < 0 || x > width || y < 0 || y > height) {
            return 0;
        }
        long pixels = native_mask(nativeInstance, x, y);
        native_upload(nativeInstance);
        return pixels;
    }

    /**
     * Uploads the current mask to the currently bound texture unit, with a GL_ALPHA format.
     */
    public void upload() {
        native_upload(nativeInstance);
    }

    /**
     * Gets the currently masked area, in pixels, within the image.
     * @return The smallest rect containing the masked area.
     */
    public Rect getMaskRect() {
        Rect out = new Rect();
        native_getMaskRect(nativeInstance, out);
        return out;
    }

    /**
     * Resets the mask, which makes it entirely masked.
     */
    public void reset() {
        native_reset(nativeInstance);
    }

    /**
     * Clears the mask so it's ready to be uploaded with no areas masked.
     */
    public void clear() {
        native_clear(nativeInstance);
    }

    /**
     * Deletes the native assets used by the Masker. This must be called when finished
     * with the masker.
     */
    public void destroy() {
        try {
            finalizer(nativeInstance);
            nativeInstance = 0;  // Other finalizers can still call us
        } catch (Exception e) {
            // no op
        }
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
    private native boolean native_isInMask(long nativeInstance, int x, int y);
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
