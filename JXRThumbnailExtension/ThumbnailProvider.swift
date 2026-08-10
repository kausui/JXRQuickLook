//
//  ThumbnailProvider.swift
//  JXRThumbnailExtension
//
//  Created by usuikanae on 2026/08/10.
//

import JXRDecoder
import QuickLookThumbnailing

class ThumbnailProvider: QLThumbnailProvider {
    override func provideThumbnail(for request: QLFileThumbnailRequest, _ handler: @escaping (QLThumbnailReply?, Error?) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                let requestedPixels = Int(ceil(
                    max(request.maximumSize.width, request.maximumSize.height) * request.scale
                ))
                let image = try JXRDecoder.makeSDRImage(
                    from: request.fileURL,
                    maximumDimension: min(max(requestedPixels, 256), 2048)
                )
                let contextSize = Self.aspectFitSize(
                    imageSize: CGSize(width: image.width, height: image.height),
                    containerSize: request.maximumSize
                )
                let reply = QLThumbnailReply(contextSize: contextSize) { context in
                    context.interpolationQuality = .high
                    // Quick Look owns the backing scale and coordinate transform.
                    // Drawing into its user-space clip avoids applying Retina scale
                    // a second time and produces a tightly fitted thumbnail canvas.
                    context.draw(image, in: context.boundingBoxOfClipPath)
                    return true
                }
                handler(reply, nil)
            } catch {
                handler(nil, error)
            }
        }
    }

    private static func aspectFitSize(imageSize: CGSize, containerSize: CGSize) -> CGSize {
        guard imageSize.width > 0, imageSize.height > 0 else {
            return containerSize
        }
        let scale = min(
            containerSize.width / imageSize.width,
            containerSize.height / imageSize.height
        )
        return CGSize(
            width: imageSize.width * scale,
            height: imageSize.height * scale
        )
    }
}
