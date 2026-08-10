//
//  PreviewViewController.swift
//  JXRPreviewExtension
//
//  Created by usuikanae on 2026/08/10.
//

import Cocoa
import JXRDecoder
import Quartz

class PreviewViewController: NSViewController, QLPreviewingController {
    private var previewImage: CGImage?

    override func loadView() {
        let rootView = NSView(frame: NSRect(x: 0, y: 0, width: 960, height: 540))
        rootView.wantsLayer = true
        rootView.layer?.backgroundColor = NSColor.black.cgColor

        if #available(macOS 14.0, *) {
            rootView.layer?.wantsExtendedDynamicRangeContent = true
        }
        rootView.layer?.contentsGravity = .resizeAspect
        view = rootView
    }

    /*
    func preparePreviewOfSearchableItem(identifier: String, queryString: String?) async throws {
        // Implement this method and set QLSupportsSearchableItems to YES in the Info.plist of the extension if you support CoreSpotlight.

        // Perform any setup necessary in order to prepare the view.
        // Quick Look will display a loading spinner until this returns.
    }
    */

    func preparePreviewOfFile(at url: URL) async throws {
        // Test configuration: decode up to native 4K resolution. A 3840 x 2160
        // RGBA Float image uses about 127 MiB before Core Animation creates its
        // render copy, so this may expose Quick Look extension memory pressure.
        let cgImage = try JXRDecoder.makeHDRImage(from: url, maximumDimension: 4_096)
        previewImage = cgImage
        view.layer?.contents = cgImage

        let maximumPreviewSize = NSSize(width: 7680, height: 7680)
        let scale = min(
            maximumPreviewSize.width / CGFloat(cgImage.width),
            maximumPreviewSize.height / CGFloat(cgImage.height),
            1
        )
        preferredContentSize = NSSize(
            width: CGFloat(cgImage.width) * scale,
            height: CGFloat(cgImage.height) * scale
        )
    }

}
