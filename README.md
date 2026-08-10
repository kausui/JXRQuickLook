# JXRQuickLook

macOS FinderでJPEG XR（`.jxr`、`.wdp`、`.hdp`）のQuick Lookプレビューとサムネイルを表示するアプリです。

## 構成

- `JXRPreviewExtension`: Spaceキーで表示するQuick Lookプレビュー
- `JXRThumbnailExtension`: Finderサムネイル
- `Packages/JXRDecoder`: 同梱JPEG XRデコーダーとCore Graphics変換層

プレビューは32-bit Float RGBAをExtended Linear sRGBの`CGImage`として表示します。Finderサムネイルは、HDR値をSDR sRGBへトーンマッピングして生成します。

## 実行

1. `JXRQuickLook.xcodeproj`をXcodeで開きます。
2. Schemeで`JXRQuickLook`を選択します。
3. My Macを対象にRunします。
4. FinderでJPEG XRファイルを選択し、Spaceキーを押します。

macOSが古いExtension情報をキャッシュしている場合は、アプリを停止してもう一度Runするか、Finderを再起動してください。

## 対応環境

- macOS 13以降
- Apple Siliconで動作確認

## ライセンス

JPEG XRデコーダーにはMicrosoftのJPEG XR Porting Kit由来の`jxrlib`を使用しています。ライセンス本文は`Packages/JXRDecoder/LICENSE-jxrlib.txt`に収録しています。
