package com.pixite.graphics.sample;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    MaskImageView image;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        image = (MaskImageView) findViewById(R.id.image);

        loadImage();
    }

    private void loadImage() {
        InputStream in = null;
        try {
            in = getAssets().open("sample.jpg");
            Bitmap bitmap = BitmapFactory.decodeStream(in);
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
}
