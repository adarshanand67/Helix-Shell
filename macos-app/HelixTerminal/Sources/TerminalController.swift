import AppKit
import Foundation

// ── PTY-backed terminal view that runs helix ─────────────────────────────────

final class TerminalController: NSObject, NSWindowDelegate {

    let view: NSView
    private let textView: NSTextView
    private let scrollView: NSScrollView

    private var masterFD: Int32 = -1
    private var childPID: pid_t = -1
    private var readSource: DispatchSourceRead?

    private var cols: Int = 120
    private var rows: Int = 30
    private let font: NSFont

    init(window: NSWindow) {
        font = NSFont(name: "Menlo", size: 13)
              ?? NSFont.monospacedSystemFont(ofSize: 13, weight: .regular)

        scrollView = NSScrollView(frame: window.contentView?.bounds ?? .zero)
        scrollView.autoresizingMask = [.width, .height]
        scrollView.hasVerticalScroller = true
        scrollView.scrollerStyle = .overlay
        scrollView.backgroundColor = NSColor(red: 0.08, green: 0.08, blue: 0.12, alpha: 1)
        scrollView.drawsBackground = true

        let tv = NSTextView(frame: scrollView.bounds)
        tv.autoresizingMask = [.width, .height]
        tv.isEditable = true
        tv.isSelectable = true
        tv.isRichText = true
        tv.drawsBackground = true
        tv.backgroundColor = NSColor(red: 0.08, green: 0.08, blue: 0.12, alpha: 1)
        tv.insertionPointColor = NSColor(red: 0.39, green: 0.92, blue: 0.71, alpha: 1)
        tv.selectedTextAttributes = [
            .backgroundColor: NSColor(red: 0.2, green: 0.4, blue: 0.6, alpha: 0.5)
        ]
        tv.textContainerInset = NSSize(width: 8, height: 8)
        tv.isVerticallyResizable = true
        tv.isHorizontallyResizable = false
        tv.textContainer?.widthTracksTextView = true
        tv.textContainer?.lineBreakMode = .byCharWrapping
        textView = tv

        scrollView.documentView = textView
        view = scrollView

        super.init()
        textView.delegate = self
        spawnHelix()
    }

    // ── Spawn helix in a PTY using posix_spawn + TIOCSWINSZ ─────────────────

    private func spawnHelix() {
        var master: Int32 = -1
        var slave: Int32 = -1
        var ws = winsize(ws_row: UInt16(rows), ws_col: UInt16(cols), ws_xpixel: 0, ws_ypixel: 0)
        guard openpty(&master, &slave, nil, nil, &ws) == 0 else {
            appendText("Error: openpty failed\n", color: .red); return
        }
        masterFD = master

        let helix = helixPath()
        var envPairs: [String] = []
        var i = 0
        while let e = environ[i] { envPairs.append(String(cString: e)); i += 1 }
        envPairs.removeAll { $0.hasPrefix("TERM=") || $0.hasPrefix("COLORTERM=") }
        envPairs.append("TERM=xterm-256color")
        envPairs.append("COLORTERM=truecolor")

        // Build C arrays for posix_spawn
        var cArgs: [UnsafeMutablePointer<CChar>?] = [strdup(helix), nil]
        var cEnv: [UnsafeMutablePointer<CChar>?] = envPairs.map { strdup($0) }
        cEnv.append(nil)

        // Use a FileActions object to dup slave fd to stdin/stdout/stderr
        var fa: posix_spawn_file_actions_t?
        posix_spawn_file_actions_init(&fa)
        posix_spawn_file_actions_adddup2(&fa, slave, STDIN_FILENO)
        posix_spawn_file_actions_adddup2(&fa, slave, STDOUT_FILENO)
        posix_spawn_file_actions_adddup2(&fa, slave, STDERR_FILENO)
        posix_spawn_file_actions_addclose(&fa, master)

        var attr: posix_spawnattr_t?
        posix_spawnattr_init(&attr)
        posix_spawnattr_setflags(&attr, Int16(POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK))

        let rc = posix_spawn(&childPID, helix, &fa, &attr, &cArgs, &cEnv)
        posix_spawn_file_actions_destroy(&fa)
        posix_spawnattr_destroy(&attr)
        cArgs.forEach { free($0) }
        cEnv.forEach { free($0) }
        close(slave)

        guard rc == 0 else {
            appendText("Error: could not launch helix (rc=\(rc))\n", color: .red)
            close(master); return
        }

        readSource = DispatchSource.makeReadSource(fileDescriptor: master, queue: .global())
        readSource?.setEventHandler { [weak self] in self?.readFromPTY() }
        readSource?.setCancelHandler { close(master) }
        readSource?.resume()
    }

    private func helixPath() -> String {
        let bundled = Bundle.main.bundlePath + "/Contents/MacOS/helix"
        if FileManager.default.isExecutableFile(atPath: bundled) { return bundled }
        for p in ["/opt/homebrew/bin/helix", "/usr/local/bin/helix"] {
            if FileManager.default.isExecutableFile(atPath: p) { return p }
        }
        return "/usr/local/bin/helix"
    }

    // ── Read PTY output ───────────────────────────────────────────────────────

    private func readFromPTY() {
        var buf = [UInt8](repeating: 0, count: 4096)
        let n = read(masterFD, &buf, buf.count)
        guard n > 0 else {
            readSource?.cancel()
            DispatchQueue.main.async { [weak self] in
                self?.appendText("\n[Helix exited]\n", color: .systemGray)
            }
            return
        }
        if let raw = String(bytes: buf[0..<n], encoding: .utf8) {
            let clean = stripAnsi(raw)
            DispatchQueue.main.async { [weak self] in self?.appendText(clean, color: nil) }
        }
    }

    // Basic ANSI escape stripper
    private func stripAnsi(_ s: String) -> String {
        var out = "", i = s.startIndex
        while i < s.endIndex {
            let c = s[i]
            if c == "\u{1B}" {
                let j = s.index(after: i)
                if j < s.endIndex && (s[j] == "[" || s[j] == "]" || s[j] == "(" || s[j] == ")") {
                    var k = s.index(after: j)
                    while k < s.endIndex && !s[k].isLetter && s[k] != "\u{7}" { k = s.index(after: k) }
                    if k < s.endIndex { k = s.index(after: k) }
                    i = k; continue
                }
            }
            out.append(c); i = s.index(after: i)
        }
        return out
    }

    // ── Append styled text ────────────────────────────────────────────────────

    private func appendText(_ text: String, color: NSColor?) {
        let attrs: [NSAttributedString.Key: Any] = [
            .font: font,
            .foregroundColor: color ?? NSColor(red: 0.85, green: 0.87, blue: 0.91, alpha: 1)
        ]
        textView.textStorage?.append(NSAttributedString(string: text, attributes: attrs))
        textView.scrollToEndOfDocument(nil)
    }

    // ── Send keystrokes to PTY ────────────────────────────────────────────────

    func sendToPTY(_ s: String) {
        guard masterFD != -1 else { return }
        var d = Array(s.utf8)
        _ = Darwin.write(masterFD, &d, d.count)
    }

    // ── NSWindowDelegate ──────────────────────────────────────────────────────

    func windowWillClose(_ notification: Notification) {
        readSource?.cancel()
        if childPID > 0 { kill(childPID, SIGTERM) }
    }
}

// ── NSTextViewDelegate — route keystrokes to PTY ─────────────────────────────

extension TerminalController: NSTextViewDelegate {

    func textView(_ textView: NSTextView, doCommandBy sel: Selector) -> Bool {
        switch sel {
        case #selector(NSResponder.insertNewline(_:)):         sendToPTY("\r");       return true
        case #selector(NSResponder.deleteBackward(_:)):        sendToPTY("\u{7F}");   return true
        case #selector(NSResponder.moveUp(_:)):                sendToPTY("\u{1B}[A"); return true
        case #selector(NSResponder.moveDown(_:)):              sendToPTY("\u{1B}[B"); return true
        case #selector(NSResponder.moveRight(_:)):             sendToPTY("\u{1B}[C"); return true
        case #selector(NSResponder.moveLeft(_:)):              sendToPTY("\u{1B}[D"); return true
        case #selector(NSResponder.cancelOperation(_:)):       sendToPTY("\u{3}");    return true
        case #selector(NSResponder.moveToBeginningOfLine(_:)): sendToPTY("\u{1}");    return true
        case #selector(NSResponder.moveToEndOfLine(_:)):       sendToPTY("\u{5}");    return true
        default: return false
        }
    }

    func textView(_ textView: NSTextView, shouldChangeTextIn range: NSRange,
                  replacementString text: String?) -> Bool {
        guard let text, !text.isEmpty else { return false }
        sendToPTY(text)
        return false
    }
}
