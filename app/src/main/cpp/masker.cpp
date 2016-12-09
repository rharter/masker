#include <jni.h>
#include <string>
#include <queue>
#include <vector>

#include <android/rect.h>
#include <android/bitmap.h>
#include <android/log.h>

#include <GLES2/gl2.h>

#define DEBUG 0

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

struct range {
  int startX, endX, y;
};

class Masker {
public:
  vector <uint32_t> pixels;
  vector <uint8_t> maskPixels;
  vector <bool> checkedPixels;
  uint32_t width, height;
  ARect maskedRect;
private:
  vector <range> ranges;

public:
  Masker(vector <uint32_t> pixels, uint32_t width, uint32_t height);
  long mask(int x, int y);
  void reset();

private:
  long linearFill(int x, int y);
  bool pixelChecked(int position);
  bool checkPixel(int position);
};

Masker::Masker(vector <uint32_t> pixels, uint32_t width, uint32_t height) {
  this->pixels = pixels;
  this->width = width;
  this->height = height;
  this->maskPixels = vector <uint8_t> (width * height);
  this->checkedPixels = vector <bool> (width * height);
  this->maskedRect = ARect();
  this->maskedRect.right = width;
  this->maskedRect.bottom = height;
}

/**
 * Empties the queue of ranges and resets all pixels so the blue
 * channel contains the original value, and all others are 0
 */
void Masker::reset() {
  ranges.clear();
  fill(maskPixels.begin(), maskPixels.end(), 0);
  fill(checkedPixels.begin(), checkedPixels.end(), false);
  maskedRect.left = 0;
  maskedRect.top = 0;
  maskedRect.right = width;
  maskedRect.bottom = height;
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
  return blue > 100;
}

long Masker::mask(int x, int y) {
  // if this isn't a white pixel, or we're already masking this
  // region because we didn't clear, skip it
  if (!checkPixel(width * y + x) || pixelChecked(width * y + x)) {
    return 0;
  }

  // initialize the mask rect
  maskedRect.left = maskedRect.right = x;
  maskedRect.top = maskedRect.bottom = y;

  // initialize the ranges
  long maskedPixels = linearFill(x, y);

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
        maskedPixels += linearFill(i, upY);
      }

      // start fill downwards if we're not at the bottom and the pixel below
      // is within the color tolerance
      if (range.y < (height - 1) && !pixelChecked(downPxIdx) && checkPixel(downPxIdx)) {
        maskedPixels += linearFill(i, downY);
      }

      downPxIdx++;
      upPxIdx++;
    }
  }

  return maskedPixels;
}

long Masker::linearFill(int x, int y) {
  int maskedPixels = 0;

  // expand the masked area
  if (y < maskedRect.top) {
    maskedRect.top = y;
  }
  if (y > maskedRect.bottom) {
    maskedRect.bottom = y;
  }

  // find the left edge of the colored area
  int left = x;
  int i = (width * y) + x;
  while (true) {
    // mark this pixel
    maskPixels[i] = 0xff;
    checkedPixels[i] = true;
    maskedPixels++;

    // expand the masked area
    if (left < maskedRect.left) {
      maskedRect.left = left;
    }

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
    maskedPixels++;

    // expand the masked area
    if (right > maskedRect.right) {
      maskedRect.right = right;
    }

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

  return maskedPixels;
}


extern "C" {
JNIEXPORT jlong JNICALL Java_com_pixite_graphics_Masker_native_1init(JNIEnv *env, jobject instance, jobject src);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1mask(JNIEnv *env, jobject instance,
                                                                    jlong nativeInstance, jobject result,
                                                                    jint x, jint y);
JNIEXPORT jlong JNICALL Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance,
                                                                       jlong nativeInstance, jint x, jint y);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1getMaskRect(JNIEnv *env, jobject instance,
                                                                           jlong nativeInstance, jobject out);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_native_1reset(JNIEnv *env, jobject instance,
                                                                     jlong nativeInstance);
JNIEXPORT void JNICALL Java_com_pixite_graphics_Masker_finalizer(JNIEnv *env, jobject instance,
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

JNIEXPORT jlong JNICALL
Java_com_pixite_graphics_Masker_native_1upload(JNIEnv *env, jobject instance, jlong nativeInstance, jint x, jint y) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  long maskedPixels = masker->mask(x, y);

  GLint unpackAlignment;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, masker->width, masker->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &masker->maskPixels[0]);

  glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
  return maskedPixels;
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1reset(JNIEnv *env, jobject instance, jlong nativeInstance) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  masker->reset();
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_native_1getMaskRect(JNIEnv *env, jobject instance, jlong nativeInstance, jobject out) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  jclass rectClass = (*env).GetObjectClass(out);
  (*env).SetIntField(out, (*env).GetFieldID(rectClass, "left", "I"), masker->maskedRect.left);
  (*env).SetIntField(out, (*env).GetFieldID(rectClass, "top", "I"), masker->maskedRect.top);
  (*env).SetIntField(out, (*env).GetFieldID(rectClass, "right", "I"), masker->maskedRect.right);
  (*env).SetIntField(out, (*env).GetFieldID(rectClass, "bottom", "I"), masker->maskedRect.bottom);
}

JNIEXPORT void JNICALL
Java_com_pixite_graphics_Masker_finalizer(JNIEnv *env, jobject instance, jlong nativeInstance) {
  Masker *masker = reinterpret_cast<Masker*>(nativeInstance);
  delete masker;
}