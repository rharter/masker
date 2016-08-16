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
  vector <range> ranges;

public:
  Masker(vector <uint32_t> pixels, uint32_t width, uint32_t height);
  void mask(int x, int y);
  void reset();

private:
  void linearFill(int x, int y);
  bool pixelChecked(int position);
  bool checkPixel(int position);
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
 * Returns true if a pixel has already been checked this round.
 */
bool Masker::pixelChecked(int position) {
  return checkedPixels[position];
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
  // if this isn't a white pixel, skip it
  if (!checkPixel(width * y + x)) {
    return;
  }

  // initialize the ranges
  linearFill(x, y);

  range range;
  while (ranges.size() > 0) {
    range = ranges.back();
    ranges.pop_back();

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
    maskPixels[i] = 0xff;
    checkedPixels[i] = true;

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
    maskPixels[i] = 0xff;
    checkedPixels[i] = true;

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
  ranges.push_back(r);
}

extern "C" {
JNIEXPORT jlong JNICALL Java_com_pixite_graphics_Masker_native_1init(JNIEnv *env, jobject instance, jobject src);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1mask(JNIEnv *env, jobject instance,
                                                                    jlong nativeInstance, jobject result,
                                                                    jint x, jint y);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance,
                                                                      jlong nativeInstance, jint x, jint y);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1reset(JNIEnv *env, jobject instance,
                                                                     jlong nativeInstance);
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

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1reset(JNIEnv *env, jobject instance, jlong nativeInstance) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  masker->reset();
}