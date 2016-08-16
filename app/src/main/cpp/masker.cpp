#include <jni.h>
#include <string>
#include <queue>
#include <vector>

#include <android/bitmap.h>
#include <android/log.h>

#include <GLES2/gl2.h>

#define DEBUG 1

#ifdef DEBUG
#define LOG_TAG "masker"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOG_TAG "masker"
#define LOGD(...)
#define LOGE(...)
#endif

using namespace std;

/**
 * Pixels within the region will be masked with this color, leaving
 * the last channel (blue in argb) with it's original value.
 */
const int MASK_COLOR = 0xffffff00;

/**
 * The green channel is used to mark pixels that have already been checked.
 */
const int CHECKED_MASK = 0x0000ff00;

struct range {
  int startX, endX, y;
};

class Masker {
public:
  vector <uint32_t> pixels;
  vector <uint8_t> maskPixels;
  vector <bool> checkedPixels;
  uint32_t width, height;
private:
  vector <int> ranges;

public:
  Masker(vector <uint32_t> pixels, uint32_t width, uint32_t height);
  void mask(int x, int y);

private:
  void reset();
  bool checkPixel(int x, int y);

  bool pop(int &x, int &y);

  void push(int x, int y);
};

Masker::Masker(vector <uint32_t> pixels, uint32_t width, uint32_t height) {
  this->pixels = pixels;
  this->width = width;
  this->height = height;
  this->maskPixels = vector <uint8_t> (width * height);
  this->checkedPixels = vector <bool> (width * height);
}

/**
 * Empties the queue of ranges and resets all pixels so the blue
 * channel contains the original value, and all others are 0
 */
void Masker::reset() {
  ranges.clear();
  fill(maskPixels.begin(), maskPixels.end(), 0);
  fill(checkedPixels.begin(), checkedPixels.end(), false);
}

/**
 * Checks a pixel by checking that the blue value is within
 * the threshold.
 */
bool Masker::checkPixel(int x, int y) {
  int blue = pixels[width * y + x] & 0xFF;
  return blue >= 200 && blue <= 255;
}

bool Masker::pop(int& x, int& y) {
  if (!ranges.empty()) {
    int p = ranges.back();
    x = p / height;
    y = p % height;
    ranges.pop_back();
    return 1;
  } else {
    return 0;
  }
}

void Masker::push(int x, int y) {
  ranges.push_back(height * x + y);
}

void Masker::mask(int x, int y) {
  // reset the state
  reset();

  int x1;
  bool spanAbove, spanBelow;

  push(x, y);

  while (pop(x, y)) {
    LOGD("Checking position[x=%d, y=%d]", x, y);
    x1 = x;
    while (x1 >= 0 && checkPixel(x1, y)) x1--;
    x1++;
    spanAbove = spanBelow = 0;
    while (x1 < width && !checkedPixels[width * y + x1] && checkPixel(x1, y)) {
      checkedPixels[width * y + x1] = 1;
      maskPixels[width * y + x1] = 0xff;

      if (!spanAbove && y > 0 && checkPixel(x1, y - 1)) {
        LOGD("pushing position[x=%d, y=%d]", x1, y - 1);
        push(x1, y - 1);
        spanAbove = 1;
      } else if (spanAbove && y > 0 && !checkPixel(x1, y - 1)) {
        spanAbove = 0;
      }

      if (!spanBelow && y < height - 1 && checkPixel(x1, y + 1)) {
        LOGD("pushing position[x=%d, y=%d]", x1, y + 1);
        push(x1, y + 1);
        spanBelow = 1;
      } else if (spanBelow && y < height - 1 && !checkPixel(x1, y + 1)) {
        spanBelow = 0;
      }

      x1++;
    }
  }
}

extern "C" {
JNIEXPORT jlong JNICALL Java_com_pixite_graphics_Masker_native_1init(JNIEnv *env, jobject instance, jobject src);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1mask(JNIEnv *env, jobject instance,
                                                                    jlong nativeInstance, jobject result,
                                                                    jint x, jint y);
JNIEXPORT void JNICALL
    Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance, jlong nativeInstance, jint x, jint y);
}


JNIEXPORT jlong JNICALL
Java_com_pixite_graphics_Masker_native_1init(JNIEnv *env, jobject instance, jobject src) {
  AndroidBitmapInfo bitmapInfo;

  int ret;
  if ((ret = AndroidBitmap_getInfo(env, src, &bitmapInfo)) < 0) {
    LOGE("AndroidBitmap_getInfo() failed! error=%d", ret);
    return -1;
  }

  if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format must be RGBA_8888");
    return -1;
  }

  void* bitmapPixels;
  if ((ret = AndroidBitmap_lockPixels(env, src, &bitmapPixels)) < 0) {
    LOGE("AndroidBitmap_lockPixels() failed! error=%d", ret);
    return -1;
  }

  u_long size = bitmapInfo.width * bitmapInfo.height;
  vector <uint32_t> pixels (size);
  memcpy(&pixels[0], (uint32_t *)bitmapPixels, sizeof(uint32_t) * size);

  AndroidBitmap_unlockPixels(env, src);

  Masker *masker = new Masker(pixels, bitmapInfo.width, bitmapInfo.height);
  return reinterpret_cast<jlong>(masker);
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1mask(JNIEnv *env, jobject instance, jlong nativeInstance, jobject result,
                                             jint x, jint y) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  AndroidBitmapInfo bitmapInfo;

  int ret;
  if ((ret = AndroidBitmap_getInfo(env, result, &bitmapInfo)) < 0) {
    LOGE("AndroidBitmap_getInfo() failed! error=%d", ret);
    return;
  }

  if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_A_8) {
    LOGE("Bitmap format must be ALPHA_8");
    return;
  }

  if (bitmapInfo.width != masker->width || bitmapInfo.height != masker->height) {
    LOGE("Result bitmap must match dimensions [%dx%d]!", masker->width, masker->height);
    return;
  }

  masker->mask(x, y);

  void* resultPixels;
  if ((ret = AndroidBitmap_lockPixels(env, result, &resultPixels)) < 0) {
    LOGE("AndroidBitmap_lockPixels() failed! error=%d", ret);
    return;
  }

  memcpy(resultPixels, &masker->maskPixels[0], sizeof(uint8_t) * masker->maskPixels.size());

  AndroidBitmap_unlockPixels(env, result);
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance, jlong nativeInstance, jint x, jint y) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  masker->mask(x, y);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, masker->width, masker->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &masker->maskPixels[0]);
}