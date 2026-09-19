cask "helix-shell" do
  version "1.0.0"
  sha256 :no_check

  url "https://github.com/adarshanand67/Helix-Shell/releases/download/v#{version}/Helix-Shell-v#{version}-macos.dmg"
  name "Helix Shell"
  desc "Modern Unix shell with built-in AI assistant"
  homepage "https://github.com/adarshanand67/Helix-Shell"

  app "HelixShell.app"

  # Expose helix CLI from inside the bundle
  binary "#{appdir}/HelixShell.app/Contents/MacOS/helix"

  postflight do
    # Strip Gatekeeper quarantine and re-sign ad-hoc so macOS doesn't kill the binary
    system_command "/usr/bin/xattr",
                   args: ["-dr", "com.apple.quarantine", "#{appdir}/HelixShell.app"]
    system_command "/usr/bin/codesign",
                   args: ["--force", "--deep", "--sign", "-", "#{appdir}/HelixShell.app"]

    # Register as a valid login shell
    helix_path = "#{appdir}/HelixShell.app/Contents/MacOS/helix"
    unless File.read("/etc/shells").include?(helix_path)
      system_command "/bin/sh",
                     args: ["-c", "echo '#{helix_path}' | sudo tee -a /etc/shells"],
                     sudo: true
    end
  end

  uninstall_preflight do
    system_command "/bin/sh",
                   args: ["-c", "sudo sed -i '' '/HelixShell.app/d' /etc/shells"],
                   sudo: true
  end

  zap trash: [
    "~/.helix_history",
    "~/.helixrc",
    "~/Library/Logs/HelixShell",
    "~/Library/Preferences/com.adarshanand67.helix-shell.plist",
  ]
end
