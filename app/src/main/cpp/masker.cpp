#include <jni.h>
#include <string>
#include <queue>

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
  uint32_t *pixels;
  uint32_t width, height;
private:
  queue <range> ranges;

public:
  Masker(uint32_t *pixels, uint32_t width, uint32_t height);
  void mask(int x, int y);

private:
  void reset();
  void linearFill(int x, int y);
  bool pixelChecked(int position);
  bool checkPixel(int position);
};

Masker::Masker(uint32_t *pixels, uint32_t width, uint32_t height) {
  this->pixels = pixels;
  this->width = width;
  this->height = height;
}

/**
 * Empties the queue of ranges and resets all pixels so the blue
 * channel contains the original value, and all others are 0
 */
void Masker::reset() {
  queue<range> empty;
  swap(ranges, empty);
  for (int i = 0; i < width * height; i++) {
    this->pixels[i] &= 0x000000ff;
  }
}

/**
 * Returns true if a pixel has already been checked this round.
 */
bool Masker::pixelChecked(int position) {
  return (pixels[position] & CHECKED_MASK) == CHECKED_MASK;
}

/**
 * Checks a pixel by checking that the blue value is within
 * the threshold.
 */
bool Masker::checkPixel(int position) {
  int blue = pixels[position] & 0xFF;
  return blue >= 200 && blue <= 255;
}

void Masker::mask(int x, int y) {
  // reset the state
  reset();

  // if this isn't a white pixel, skip it
  if (!checkPixel((width * y) + x)) {
    return;
  }

  // initialize the ranges
  linearFill(x, y);

  range range;
  while (ranges.size() > 0) {
    range = ranges.front();
    ranges.pop();

    // check above and below each pixel in the flood fill range
    int downPxIdx = (width * (range.y + 1)) + range.startX;
    int upPxIdx = (width * (range.y - 1)) + range.startX;
    int downY = range.y + 1;
    int upY = range.y - 1;

    for (int i = range.startX; i <= range.endX; i++) {
      // start fill upwards if we're not at the top and the pixel above
      // this one is within the color tolerance
      if (range.y > 0 && !pixelChecked(upPxIdx) && checkPixel(upPxIdx)) {
        linearFill(i, upY);
      }

      // start fill downwards if we're not at the bottom and the pixel below
      // is within the color tolerance
      if (range.y < (height - 1) && !pixelChecked(downPxIdx) && checkPixel(downPxIdx)) {
        linearFill(i, downY);
      }

      downPxIdx++;
      upPxIdx++;
    }
  }
}

void Masker::linearFill(int x, int y) {

  // find the left edge of the colored area
  int left = x;
  int i = (width * y) + x;
  while (true) {
    // mark this pixel
    pixels[i] |= MASK_COLOR;

    // decrement
    left--;
    i--;

    // exit the loop if we are at the edge of the bitmap or colored area
    if (left < 0 || pixelChecked(i) || !checkPixel(i)) {
      break;
    }
  }
  left++;

  // find the right edge of the colored area
  int right = x;
  i = (width * y) + x;
  while (true) {
    // mark this pixel
    pixels[i] |= MASK_COLOR;

    // increment
    right++;
    i++;

    // exit the loop if we are at the edge of the bitmap or colored area
    if (right >= width || pixelChecked(i) || !checkPixel(i)) {
      break;
    }
  }
  right--;

  range r = range();
  r.startX = left;
  r.endX = right;
  r.y = y;
  ranges.push(r);
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

  int size = bitmapInfo.width * bitmapInfo.height;
  uint32_t *pixels = new uint32_t[size];
  memcpy(pixels, (uint32_t *)bitmapPixels, sizeof(uint32_t) * size);

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

  if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format must be RGBA_8888");
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

  memcpy(resultPixels, masker->pixels, sizeof(uint32_t) * masker->width * masker->height);

  AndroidBitmap_unlockPixels(env, result);
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance, jlong nativeInstance, jint x, jint y) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  masker->mask(x, y);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, masker->width, masker->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, masker->pixels);
}