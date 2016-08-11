package com.pixite.graphics.sample;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PorterDuff;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;

import com.pixite.graphics.Masker;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity implements View.OnTouchListener {

    Bitmap bitmap;
    Masker masker;

    ImageView image;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        image = (ImageView) findViewById(R.id.image);
        image.setOnTouchListener(this);

        loadImage();
    }

    private void loadImage() {
        InputStream in = null;
        try {
            in = getAssets().open("sample.jpg");
            bitmap = BitmapFactory.decodeStream(in);
            masker = new Masker(bitmap);
            image.setImageBitmap(bitmap);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (in != null) {
                    in.close();
                }
            } catch (IOException e) {
                // no op
            }
        }
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        int action = motionEvent.getAction();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                int x = Math.round(bitmap.getWidth() * (motionEvent.getX() / image.getWidth()));
                int y = Math.round(bitmap.getHeight() * (motionEvent.getY() / image.getHeight()));
                Bitmap mask = masker.getMask(x, y);
                image.setImageBitmap(mask);
                image.setColorFilter(0xffff0000, PorterDuff.Mode.SRC_ATOP);
                return true;
            case MotionEvent.ACTION_UP:
                image.setImageBitmap(bitmap);
                image.setColorFilter(null);
                return true;
        }
        return false;
    }
}
