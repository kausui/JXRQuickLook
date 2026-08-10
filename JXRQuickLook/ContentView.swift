//
//  ContentView.swift
//  JXRQuickLook
//
//  Created by usuikanae on 2026/08/10.
//

import SwiftUI

struct ContentView: View {
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "photo.on.rectangle.angled")
                .font(.system(size: 48))
                .foregroundStyle(.tint)
            Text("JPEG XR Quick Look")
                .font(.title2)
                .fontWeight(.semibold)
            Text("JPEG XR preview and thumbnail extensions are included in this app.")
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
        }
        .padding(32)
        .frame(minWidth: 440, minHeight: 260)
    }
}
