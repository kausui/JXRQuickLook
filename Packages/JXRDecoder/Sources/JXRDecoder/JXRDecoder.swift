import CJXR
import CoreGraphics
import Foundation

public enum JXRDecoderError: LocalizedError {
    case decodingFailed(String)
    case imageCreationFailed

    public var errorDescription: String? {
        switch self {
        case .decodingFailed(let message): message
        case .imageCreationFailed: "The decoded pixels could not be converted to an image."
        }
    }
}

public enum JXRDecoder {
    public static func makeHDRImage(from url: URL, maximumDimension: Int = 0) throws -> CGImage {
        var decoded = try decode(url: url, maximumDimension: maximumDimension)
        let width = decoded.width
        let height = decoded.height
        let rowBytes = decoded.rowBytes
        let byteCount = rowBytes * height
        guard let pixels = decoded.pixels else {
            throw JXRDecoderError.imageCreationFailed
        }

        let data = Data(
            bytesNoCopy: UnsafeMutableRawPointer(pixels),
            count: byteCount,
            deallocator: .free
        )
        decoded.pixels = nil
        cjxr_release_image(&decoded)

        guard
            let provider = CGDataProvider(data: data as CFData),
            let colorSpace = CGColorSpace(name: CGColorSpace.extendedLinearSRGB),
            let image = CGImage(
                width: width,
                height: height,
                bitsPerComponent: 32,
                bitsPerPixel: 128,
                bytesPerRow: rowBytes,
                space: colorSpace,
                bitmapInfo: CGBitmapInfo(rawValue:
                    CGBitmapInfo.floatComponents.rawValue |
                    CGBitmapInfo.byteOrder32Little.rawValue |
                    CGImageAlphaInfo.last.rawValue
                ),
                provider: provider,
                decode: nil,
                shouldInterpolate: true,
                intent: .relativeColorimetric
            )
        else {
            throw JXRDecoderError.imageCreationFailed
        }
        return image
    }

    public static func makeSDRImage(
        from url: URL,
        maximumDimension: Int,
        exposure: Float = 1.5
    ) throws -> CGImage {
        var decoded = try decode(url: url, maximumDimension: maximumDimension)
        defer { cjxr_release_image(&decoded) }

        guard let pixels = cjxr_create_tonemapped_srgb8(&decoded, exposure) else {
            throw JXRDecoderError.imageCreationFailed
        }
        let byteCount = decoded.width * decoded.height * 4
        let data = Data(
            bytesNoCopy: UnsafeMutableRawPointer(pixels),
            count: byteCount,
            deallocator: .free
        )

        guard
            let provider = CGDataProvider(data: data as CFData),
            let colorSpace = CGColorSpace(name: CGColorSpace.sRGB),
            let image = CGImage(
                width: decoded.width,
                height: decoded.height,
                bitsPerComponent: 8,
                bitsPerPixel: 32,
                bytesPerRow: decoded.width * 4,
                space: colorSpace,
                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
                provider: provider,
                decode: nil,
                shouldInterpolate: true,
                intent: .relativeColorimetric
            )
        else {
            throw JXRDecoderError.imageCreationFailed
        }
        return image
    }

    private static func decode(url: URL, maximumDimension: Int) throws -> CJXRImage {
        var decoded = CJXRImage()
        var errorBuffer = [CChar](repeating: 0, count: 256)
        let result = errorBuffer.withUnsafeMutableBufferPointer { buffer in
            url.withUnsafeFileSystemRepresentation { path in
                cjxr_decode_rgba_float(
                    path,
                    max(maximumDimension, 0),
                    &decoded,
                    buffer.baseAddress,
                    buffer.count
                )
            }
        }
        guard result == 0 else {
            let message = errorBuffer.withUnsafeBufferPointer {
                String(cString: $0.baseAddress!)
            }
            throw JXRDecoderError.decodingFailed(message.isEmpty ? "JPEG XR decoding failed." : message)
        }
        return decoded
    }

}
