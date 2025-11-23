class HelixShell < Formula
  desc "Modern Unix shell with advanced features, autocompletion, and job control"
  homepage "https://github.com/adarshanand67/Helix-Shell"
  url "https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "e153f18e86e1be877e2b5f9f3ed1378d4a67ce3d9d4038eb42d697a4ced93d35"
  license "MIT"
  head "https://github.com/adarshanand67/Helix-Shell.git", branch: "master"

  depends_on "pkg-config" => :build
  depends_on "cppunit" => :build
  depends_on "readline"

  def install
    # Build the project
    system "make", "build"

    # Install the binary
    bin.install "build/hsh" => "helix"

    # Install documentation
    doc.install "README.md"
    doc.install "docs/PROMPT.md" if File.exist?("docs/PROMPT.md")

    # Install man page if exists
    # man1.install "docs/helix.1" if File.exist?("docs/helix.1")
  end

  test do
    # Test that the binary runs and exits properly
    output = shell_output("echo 'exit' | #{bin}/helix 2>&1", 0)
    assert_match(/HelixShell|helix/, output.downcase)

    # Test basic command execution
    assert_match "Hello", shell_output("echo 'echo Hello' | #{bin}/helix 2>&1")
  end

  def caveats
    <<~EOS
      HelixShell has been installed as 'helix'

      To use it as your default shell:
        1. Add #{bin}/helix to /etc/shells
        2. Run: chsh -s #{bin}/helix

      Features:
        • Advanced autocompletion (TAB)
        • Colored prompt with Git integration
        • Command history (↑/↓ arrows)
        • Job control (jobs, fg, bg)
        • Pipeline support (|)
        • I/O redirection (>, <, >>)

      Documentation: #{doc}/README.md
    EOS
  end
end
