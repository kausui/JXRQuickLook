#include "CJXR.h"

#include <JXRGlue.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cjxr_set_error(char *message, size_t capacity, const char *text, int code) {
    if (message == NULL || capacity == 0) return;
    snprintf(message, capacity, "%s (JPEG XR error %d)", text, code);
}

static float cjxr_srgb_encode(float linear) {
    if (linear <= 0.0031308f) return linear * 12.92f;
    return 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
}

static float cjxr_srgb_decode(float encoded) {
    if (encoded <= 0.04045f) return encoded / 12.92f;
    return powf((encoded + 0.055f) / 1.055f, 2.4f);
}

int cjxr_decode_rgba_float(
    const char *path,
    size_t maximumDimension,
    CJXRImage *output,
    char *errorMessage,
    size_t errorMessageCapacity
) {
    PKCodecFactory *factory = NULL;
    PKImageDecode *decoder = NULL;
    PKFormatConverter *converter = NULL;
    PKRect rectangle = {0, 0, 0, 0};
    I32 originalWidth = 0;
    I32 originalHeight = 0;
    size_t width = 0;
    size_t height = 0;
    size_t pixelCount = 0;
    size_t componentCount = 0;
    size_t index = 0;
    float *pixels = NULL;
    uint8_t *packedPixels = NULL;
    size_t packedRowBytes = 0;
    size_t packedComponents = 0;
    size_t sourceBytesPerPixel = 0;
    int decodedAsFloat = 0;
    int error = WMP_errSuccess;

    if (output == NULL || path == NULL) return WMP_errInvalidParameter;
    memset(output, 0, sizeof(*output));
    if (errorMessage != NULL && errorMessageCapacity > 0) errorMessage[0] = '\0';

    error = PKCreateCodecFactory(&factory, WMP_SDK_VERSION);
    if (error != WMP_errSuccess) {
        cjxr_set_error(errorMessage, errorMessageCapacity, "Could not create the codec factory", error);
        goto cleanup;
    }

    error = factory->CreateDecoderFromFile(path, &decoder);
    if (error != WMP_errSuccess) {
        cjxr_set_error(errorMessage, errorMessageCapacity, "Could not open the JPEG XR image", error);
        goto cleanup;
    }

    error = decoder->GetSize(decoder, &originalWidth, &originalHeight);
    if (error != WMP_errSuccess || originalWidth <= 0 || originalHeight <= 0) {
        cjxr_set_error(errorMessage, errorMessageCapacity, "The image dimensions are invalid", error);
        goto cleanup;
    }

    width = (size_t)originalWidth;
    height = (size_t)originalHeight;
    if (width > 32768 || height > 32768 || width > SIZE_MAX / height) {
        error = WMP_errInvalidParameter;
        cjxr_set_error(errorMessage, errorMessageCapacity, "The image is too large", error);
        goto cleanup;
    }

    if (maximumDimension > 0 && (width > maximumDimension || height > maximumDimension)) {
        size_t factor = 0;
        size_t largest = width > height ? width : height;
        while (factor < 8 && ((largest + (((size_t)1 << factor) - 1)) >> factor) > maximumDimension) {
            factor++;
        }
        size_t divisor = (size_t)1 << factor;
        width = (width + divisor - 1) / divisor;
        height = (height + divisor - 1) / divisor;
        decoder->WMP.wmiI.cThumbnailWidth = width;
        decoder->WMP.wmiI.cThumbnailHeight = height;
        if (decoder->WMP.wmiI.cfColorFormat == YUV_420 || decoder->WMP.wmiI.cfColorFormat == YUV_422) {
            decoder->WMP.wmiI.cfColorFormat = YUV_444;
        }
    } else {
        decoder->WMP.wmiI.cThumbnailWidth = width;
        decoder->WMP.wmiI.cThumbnailHeight = height;
    }

    decoder->WMP.wmiI.cROILeftX = 0;
    decoder->WMP.wmiI.cROITopY = 0;
    decoder->WMP.wmiI.cROIWidth = width;
    decoder->WMP.wmiI.cROIHeight = height;
    decoder->WMP.wmiSCP.uAlphaMode = decoder->WMP.bHasAlpha ? 2 : 0;

    error = factory->CreateFormatConverter(&converter);
    if (error != WMP_errSuccess) {
        cjxr_set_error(errorMessage, errorMessageCapacity, "Could not create the pixel converter", error);
        goto cleanup;
    }

    error = converter->Initialize(
        converter,
        decoder,
        ".tif",
        GUID_PKPixelFormat128bppRGBAFloat
    );
    if (error == WMP_errSuccess) {
        decodedAsFloat = 1;
    } else {
        PKPixelFormatGUID sourceFormat;
        PKPixelInfo sourceInfo;
        PKPixelFormatGUID destinationFormat;

        converter->Release(&converter);
        memset(&sourceInfo, 0, sizeof(sourceInfo));
        error = decoder->GetPixelFormat(decoder, &sourceFormat);
        if (error != WMP_errSuccess) goto unsupported_format;
        sourceInfo.pGUIDPixFmt = &sourceFormat;
        error = PixelFormatLookup(&sourceInfo, LOOKUP_FORWARD);
        if (error != WMP_errSuccess) goto unsupported_format;
        sourceBytesPerPixel = (sourceInfo.cbitUnit + 7) / 8;

        if (sourceInfo.cfColorFormat == Y_ONLY) {
            destinationFormat = GUID_PKPixelFormat8bppGray;
            packedComponents = 1;
        } else if (sourceInfo.cfColorFormat == CF_RGB &&
                   (sourceInfo.grBit & PK_pixfmtHasAlpha) == 0) {
            destinationFormat = GUID_PKPixelFormat24bppRGB;
            packedComponents = 3;
        } else if (sourceInfo.cfColorFormat == CF_RGB &&
                   (sourceInfo.grBit & PK_pixfmtPreMul) == 0) {
            destinationFormat = GUID_PKPixelFormat32bppRGBA;
            packedComponents = 4;
        } else {
            goto unsupported_format;
        }

        error = factory->CreateFormatConverter(&converter);
        if (error != WMP_errSuccess) goto unsupported_format;
        error = converter->Initialize(converter, decoder, NULL, destinationFormat);
        if (error != WMP_errSuccess) goto unsupported_format;
    }

    pixelCount = width * height;
    if (pixelCount > SIZE_MAX / (4 * sizeof(float))) {
        error = WMP_errOutOfMemory;
        cjxr_set_error(errorMessage, errorMessageCapacity, "The decoded image is too large", error);
        goto cleanup;
    }
    componentCount = pixelCount * 4;
    pixels = (float *)calloc(componentCount, sizeof(float));
    if (pixels == NULL) {
        error = WMP_errOutOfMemory;
        cjxr_set_error(errorMessage, errorMessageCapacity, "Not enough memory to decode this image", error);
        goto cleanup;
    }

    rectangle.Width = (I32)width;
    rectangle.Height = (I32)height;
    if (decodedAsFloat) {
        error = converter->Copy(converter, &rectangle, (U8 *)pixels, (U32)(width * 16));
    } else {
        packedRowBytes = width * (sourceBytesPerPixel > packedComponents
            ? sourceBytesPerPixel
            : packedComponents);
        packedPixels = (uint8_t *)malloc(packedRowBytes * height);
        if (packedPixels == NULL) {
            error = WMP_errOutOfMemory;
            cjxr_set_error(errorMessage, errorMessageCapacity, "Not enough memory to decode this image", error);
            goto cleanup;
        }
        error = converter->Copy(converter, &rectangle, packedPixels, (U32)packedRowBytes);
        if (error == WMP_errSuccess) {
            size_t y = 0;
            for (y = 0; y < height; y++) {
                size_t x = 0;
                const uint8_t *sourceRow = packedPixels + y * packedRowBytes;
                for (x = 0; x < width; x++) {
                    const uint8_t *source = sourceRow + x * packedComponents;
                    float *destination = pixels + (y * width + x) * 4;
                    if (packedComponents == 1) {
                        float gray = cjxr_srgb_decode(source[0] / 255.0f);
                        destination[0] = gray;
                        destination[1] = gray;
                        destination[2] = gray;
                    } else {
                        destination[0] = cjxr_srgb_decode(source[0] / 255.0f);
                        destination[1] = cjxr_srgb_decode(source[1] / 255.0f);
                        destination[2] = cjxr_srgb_decode(source[2] / 255.0f);
                    }
                    destination[3] = packedComponents == 4 ? source[3] / 255.0f : 1.0f;
                }
            }
        }
    }
    if (error != WMP_errSuccess) {
        cjxr_set_error(errorMessage, errorMessageCapacity, "JPEG XR decoding failed", error);
        goto cleanup;
    }

    output->containsExtendedRange = 0;
    for (index = 0; index < componentCount; index += 4) {
        size_t channel = 0;
        for (channel = 0; channel < 3; channel++) {
            float value = pixels[index + channel];
            if (!isfinite(value)) value = 0.0f;
            pixels[index + channel] = value;
            if (value > 1.0f || value < 0.0f) output->containsExtendedRange = 1;
        }
        // Screenshot-oriented JPEG XR files commonly declare an RGBA float
        // pixel format while storing an unused, all-zero alpha plane. Quick
        // Look must present those RGB pixels as an opaque image.
        pixels[index + 3] = 1.0f;
    }

    output->width = width;
    output->height = height;
    output->rowBytes = width * 16;
    output->pixels = pixels;
    pixels = NULL;

    goto cleanup;

unsupported_format:
    cjxr_set_error(errorMessage, errorMessageCapacity, "This JPEG XR pixel format is not supported", error);

cleanup:
    free(packedPixels);
    free(pixels);
    if (converter != NULL) converter->Release(&converter);
    if (decoder != NULL) decoder->Release(&decoder);
    if (factory != NULL) factory->Release(&factory);
    return error;
}

uint8_t *cjxr_create_tonemapped_srgb8(const CJXRImage *image, float exposure) {
    size_t pixelCount = 0;
    size_t index = 0;
    uint8_t *result = NULL;

    if (image == NULL || image->pixels == NULL || image->width == 0 || image->height == 0) return NULL;
    if (image->width > SIZE_MAX / image->height) return NULL;
    pixelCount = image->width * image->height;
    if (pixelCount > SIZE_MAX / 4) return NULL;
    result = (uint8_t *)malloc(pixelCount * 4);
    if (result == NULL) return NULL;
    if (!isfinite(exposure) || exposure <= 0.0f) exposure = 1.5f;

    for (index = 0; index < pixelCount; index++) {
        const float *source = image->pixels + index * 4;
        uint8_t *destination = result + index * 4;
        float red = fmaxf(source[0], 0.0f);
        float green = fmaxf(source[1], 0.0f);
        float blue = fmaxf(source[2], 0.0f);
        float luminance = red * 0.2126f + green * 0.7152f + blue * 0.0722f;
        float mappedLuminance = 1.0f - expf(-luminance * exposure);
        float scale = luminance > 0.000001f ? mappedLuminance / luminance : exposure;
        red = cjxr_srgb_encode(fminf(fmaxf(red * scale, 0.0f), 1.0f));
        green = cjxr_srgb_encode(fminf(fmaxf(green * scale, 0.0f), 1.0f));
        blue = cjxr_srgb_encode(fminf(fmaxf(blue * scale, 0.0f), 1.0f));
        destination[0] = (uint8_t)lrintf(fminf(fmaxf(red, 0.0f), 1.0f) * 255.0f);
        destination[1] = (uint8_t)lrintf(fminf(fmaxf(green, 0.0f), 1.0f) * 255.0f);
        destination[2] = (uint8_t)lrintf(fminf(fmaxf(blue, 0.0f), 1.0f) * 255.0f);
        destination[3] = (uint8_t)lrintf(fminf(fmaxf(source[3], 0.0f), 1.0f) * 255.0f);
    }

    return result;
}

void cjxr_release_image(CJXRImage *image) {
    if (image == NULL) return;
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}
