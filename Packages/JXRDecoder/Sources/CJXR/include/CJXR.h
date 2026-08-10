#ifndef CJXR_H
#define CJXR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CJXRImage {
    size_t width;
    size_t height;
    size_t rowBytes;
    float *pixels;
    int containsExtendedRange;
} CJXRImage;

int cjxr_decode_rgba_float(
    const char *path,
    size_t maximumDimension,
    CJXRImage *output,
    char *errorMessage,
    size_t errorMessageCapacity
);

uint8_t *cjxr_create_tonemapped_srgb8(
    const CJXRImage *image,
    float exposure
);

void cjxr_release_image(CJXRImage *image);

#ifdef __cplusplus
}
#endif

#endif
