package com.pixite.graphics.sample;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.ImageView;

import com.pixite.graphics.Masker;

public class MaskImageView extends ImageView {

    private Masker masker;
    private int bitmapWidth, bitmapHeight;
    private Bitmap mask;

    private final Paint maskPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint overlayBackground = new Paint();
    private final Paint overlayPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final int padding;

    long measure;

    public MaskImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        maskPaint.setColorFilter(new PorterDuffColorFilter(0xffff0000, PorterDuff.Mode.SRC_ATOP));
        overlayBackground.setColor(Color.DKGRAY);
        overlayBackground.setStyle(Paint.Style.FILL);
        overlayBackground.setAlpha(189);
        overlayPaint.setColor(Color.WHITE);
        overlayPaint.setTextSize(getResources().getDimensionPixelSize(R.dimen.overlay_text_size));
        padding = getResources().getDimensionPixelSize(R.dimen.overlay_padding);
    }

    @Override
    public void setImageBitmap(Bitmap bm) {
        super.setImageBitmap(bm);
        masker = new Masker(bm);
        bitmapWidth = bm.getWidth();
        bitmapHeight = bm.getHeight();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        int size = Math.min(width, height);
        widthMeasureSpec = MeasureSpec.makeMeasureSpec(size, MeasureSpec.EXACTLY);
        heightMeasureSpec = MeasureSpec.makeMeasureSpec(size, MeasureSpec.EXACTLY);
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mask != null) {
            long start = System.currentTimeMillis();
            canvas.drawBitmap(mask, getImageMatrix(), maskPaint);
            Log.d("Masker", "drawBitmap(): " + (System.currentTimeMillis() - start) + "ms");
            Log.d("Masker", "invalidate(): " + (System.currentTimeMillis() - measure) + "ms");
        }
        // draw the masked rect
        Rect maskRect = masker.getMaskRect();
        String overlayText = "MaskRect: " + maskRect;
        canvas.drawRect(0.0f, 0.0f,
                overlayPaint.measureText(overlayText) + padding * 2,
                overlayPaint.getTextSize() + padding * 2,
                overlayBackground);
        canvas.drawText(overlayText, padding, overlayPaint.getTextSize() + padding / 2, overlayPaint);
        Log.d("Masker", "invalidate(): " + (System.currentTimeMillis() - measure) + "ms");
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        measure = System.currentTimeMillis();
        int action = event.getAction();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                float viewX = event.getX() - getLeft();
                float viewY = event.getY() - getTop();
                int x = Math.round(bitmapWidth * (viewX / getWidth()));
                int y = Math.round(bitmapHeight * (viewY / getHeight()));
                long start = System.currentTimeMillis();
                mask = masker.getMask(x, y);
                Log.d("Masker", "masker.getMask(): " + (System.currentTimeMillis() - start) + "ms");
                invalidate();
                return true;
            case MotionEvent.ACTION_UP:
                mask = null;
                //masker.reset();
                //invalidate();
                return true;
        }
        return super.onTouchEvent(event);
    }
}
