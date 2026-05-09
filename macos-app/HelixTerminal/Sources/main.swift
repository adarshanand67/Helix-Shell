import AppKit
import Foundation

// ── Entry point ──────────────────────────────────────────────────────────────

final class AppDelegate: NSObject, NSApplicationDelegate {
    var window: NSWindow!
    var controller: TerminalController!

    func applicationDidFinishLaunching(_ notification: Notification) {
        let size = NSRect(x: 0, y: 0, width: 900, height: 600)
        window = NSWindow(
            contentRect: size,
            styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
            backing: .buffered,
            defer: false
        )
        window.title = "Helix Shell"
        window.titlebarAppearsTransparent = true
        window.backgroundColor = NSColor(red: 0.08, green: 0.08, blue: 0.12, alpha: 1)
        window.minSize = NSSize(width: 500, height: 300)
        window.center()

        controller = TerminalController(window: window)
        window.contentView = controller.view
        window.makeKeyAndOrderFront(nil)
        window.delegate = controller
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.activate(ignoringOtherApps: true)
app.run()
