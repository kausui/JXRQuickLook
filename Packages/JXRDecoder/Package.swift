// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "JXRDecoder",
    platforms: [.macOS(.v13)],
    products: [
        .library(name: "JXRDecoder", targets: ["JXRDecoder"])
    ],
    targets: [
        .target(
            name: "CJXR",
            exclude: [
                "Vendor/image/decode/strdec_x86.c",
                "Vendor/image/encode/strenc_x86.c"
            ],
            sources: [
                "JXRDecoder.c",
                "Vendor/image/sys",
                "Vendor/image/decode",
                "Vendor/image/encode",
                "Vendor/jxrgluelib"
            ],
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("Vendor/common/include"),
                .headerSearchPath("Vendor/image/sys"),
                .headerSearchPath("Vendor/image/decode"),
                .headerSearchPath("Vendor/image/encode"),
                .headerSearchPath("Vendor/jxrgluelib"),
                .define("__ANSI__"),
                .define("DISABLE_PERF_MEASUREMENT"),
                .unsafeFlags([
                    "-include", "wchar.h",
                    "-Wno-error=implicit-function-declaration",
                    "-w"
                ])
            ],
            linkerSettings: [.linkedLibrary("m")]
        ),
        .target(
            name: "JXRDecoder",
            dependencies: ["CJXR"]
        )
    ],
    cLanguageStandard: .c99
)
