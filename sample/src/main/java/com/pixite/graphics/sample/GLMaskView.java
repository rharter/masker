package com.pixite.graphics.sample;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;

import com.pixite.graphics.Masker;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_DEPTH_BUFFER_BIT;
import static android.opengl.GLES20.GL_FLOAT;
import static android.opengl.GLES20.GL_TRIANGLE_STRIP;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static android.opengl.GLES20.glDisableVertexAttribArray;
import static android.opengl.GLES20.glDrawArrays;
import static android.opengl.GLES20.glEnableVertexAttribArray;
import static android.opengl.GLES20.glUniform1i;
import static android.opengl.GLES20.glVertexAttribPointer;

public class GLMaskView extends GLSurfaceView implements GLSurfaceView.Renderer {

    private static final FloatBuffer QUAD_VERTICES = ByteBuffer
            .allocateDirect(32)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()
            .put(new float [] {1, -1, 1, 1, -1, -1, -1, 1});

    private Bitmap image;

    private Program program;
    private Texture imageTexture;
    private Texture maskTexture;
    private Masker masker;

    private int[] maskPoints = new int[2];

    public GLMaskView(Context context) {
        this(context, null);
    }

    public GLMaskView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        setRenderer(this);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
    }

    public void setImageBitmap(Bitmap image) {
        this.image = image;
        requestRender();
    }

    public void mask(int x, int y) {
        maskPoints = new int[] {x, y};
        requestRender();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                float viewX = event.getX() - getLeft();
                float viewY = event.getY() - getTop();
                int x = Math.round(image.getWidth() * (viewX / getWidth()));
                int y = Math.round(image.getHeight() * (viewY / getHeight()));
                long start = System.currentTimeMillis();
                mask(x, y);
                Log.d("Masker", "masker.getMask(): " + (System.currentTimeMillis() - start) + "ms");
                return true;
            case MotionEvent.ACTION_UP:
                mask(-1, -1);
                return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int i, int i1) {

    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        if (image == null) {
            return;
        }

        if (program == null) {
            initProgram();
        }

        if (image != null && imageTexture == null) {
            initImageTexture(image);
        }

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        imageTexture.bind(0);
        glUniform1i(program.uniformLocation("image_texture"), 0);
        //
        //maskTexture.bind(1);
        //glUniform1i(program.uniformLocation("mask_texture"), 1);
        //masker.uploadMask(maskPoints[0], maskPoints[1]);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, QUAD_VERTICES.rewind());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(0);
    }

    private void initProgram() {
        program = Program.load("mask", VERTEX_SHADER, FRAGMENT_SHADER);
    }

    private void initImageTexture(Bitmap image) {
        imageTexture = new Texture(image);
        maskTexture = new Texture();
        masker = new Masker(image);
    }

    private static final String VERTEX_SHADER = "" +
            "attribute vec4 position;\n" +
            "varying highp vec2 v_textureCoord;\n" +
            "void main() {\n" +
            "  v_textureCoord = position.xy * 0.5 + 0.5;\n" +
            "  gl_Position = position;\n" +
            "}";

    private static final String FRAGMENT_SHADER = "" +
            "uniform sampler2D image_texture;\n" +
            "uniform sampler2D mask_texture;\n" +
            "varying highp vec2 v_textureCoord;\n" +
            "void main() {\n" +
            "  lowp vec4 imageTexelColor = texture2D(image_texture, v_textureCoord);\n" +
            "  lowp vec4 maskTexelColor = texture2D(mask_texture, v_textureCoord);\n" +
            "  gl_FragColor = imageTexelColor;// * maskTexelColor.r;\n" +
            "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n" +
            "}";
}
