class HelixShellBin < Formula
  desc "Modern Unix shell with built-in AI assistant — pre-built binary"
  homepage "https://github.com/adarshanand67/Helix-Shell"
  version "1.0.0"
  license "MIT"

  on_macos do
    if Hardware::CPU.arm?
      url "https://github.com/adarshanand67/Helix-Shell/releases/download/v#{version}/helix-v#{version}-macos-arm64.tar.gz"
      sha256 :no_check
    else
      url "https://github.com/adarshanand67/Helix-Shell/releases/download/v#{version}/helix-v#{version}-macos-x86_64.tar.gz"
      sha256 :no_check
    end
  end

  on_linux do
    url "https://github.com/adarshanand67/Helix-Shell/releases/download/v#{version}/helix-v#{version}-linux-x86_64.tar.gz"
    sha256 :no_check
  end

  # No build dependencies — ships as a pre-built binary
  depends_on "readline"

  def install
    bin.install "helix"
    doc.install "README.md" if File.exist?("README.md")
  end

  test do
    assert_match "Hello", shell_output("echo 'echo Hello' | #{bin}/helix 2>&1")
    assert_match(/helix|hsh/i, shell_output("echo 'exit' | #{bin}/helix 2>&1", 0))
  end

  def caveats
    <<~EOS
      Helix Shell has been installed as 'helix'.

      To use as your default shell:
        sudo sh -c 'echo #{bin}/helix >> /etc/shells'
        chsh -s #{bin}/helix

      AI assistant (set any one key):
        export ANTHROPIC_API_KEY=<key>   # Claude
        export OPENAI_API_KEY=<key>      # GPT
        export GROQ_API_KEY=<key>        # Groq
        export GOOGLE_API_KEY=<key>      # Gemini
        # No key needed for local Ollama

      Config: ~/.helixrc   History: ~/.helix_history
    EOS
  end
end
