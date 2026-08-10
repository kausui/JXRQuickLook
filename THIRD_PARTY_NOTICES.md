# Third-Party Notices

JXRQuickLook includes source code from the following third-party project.

## jxrlib

- Project: jxrlib — JPEG XR reference implementation
- Immediate upstream: https://github.com/4creators/jxrlib
- Vendored revision: `f752187` (2017-06-15)
- Original source: Microsoft's JPEG XR Porting Kit, formerly published on
  CodePlex at `https://jxrlib.codeplex.com`
- Copyright: Microsoft Corp.
- License: BSD 2-Clause License
- License text:
  [`Packages/JXRDecoder/LICENSE-jxrlib.txt`](Packages/JXRDecoder/LICENSE-jxrlib.txt)
- Vendored location:
  `Packages/JXRDecoder/Sources/CJXR/Vendor/`

The `4creators/jxrlib` repository describes itself as a clone of the source
released by Microsoft from the original CodePlex location. The files vendored
by JXRQuickLook are the codec, image-processing, glue-library, and supporting
header sources required by the decoder package.

JXRQuickLook does not claim authorship of the vendored sources. Their existing
copyright notices must be retained in source distributions, and the BSD
2-Clause license text must accompany source or binary redistributions as
required by that license.

Code outside the `Vendor/` directory—including the `CJXR` wrapper,
Swift interface, Core Graphics conversion code, and Quick Look extensions—is
maintained as part of JXRQuickLook unless a file states otherwise.
