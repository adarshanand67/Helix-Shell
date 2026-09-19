class HelixShell < Formula
  desc "Modern Unix shell with built-in AI assistant (suggest, explain, fix commands)"
  homepage "https://github.com/adarshanand67/Helix-Shell"
  url "https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "e153f18e86e1be877e2b5f9f3ed1378d4a67ce3d9d4038eb42d697a4ced93d35"
  license "MIT"
  head "https://github.com/adarshanand67/Helix-Shell.git", branch: "master"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "cppunit" => :build
  depends_on "readline"

  def install
    system "cmake", "-S", ".", "-B", "build",
           "-DCMAKE_BUILD_TYPE=Release",
           "-DCMAKE_INSTALL_PREFIX=#{prefix}",
           *std_cmake_args
    system "cmake", "--build", "build", "-j"
    system "cmake", "--install", "build"

    doc.install "README.md"
    doc.install "docs/PROMPT.md" if File.exist?("docs/PROMPT.md")
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

      AI assistant setup (pick one):
        export ANTHROPIC_API_KEY=<key>   # Claude
        export OPENAI_API_KEY=<key>      # GPT
        export GROQ_API_KEY=<key>        # Groq
        export GOOGLE_API_KEY=<key>      # Gemini
        # No key needed for local Ollama

      Usage:
        ai find all log files older than 7 days
        ai explain "rsync -avz --delete src/ dest/"
        ai fix "permission denied error"
        ai run compress this directory

      Config file: ~/.helixrc
      History:     ~/.helix_history
      Docs:        #{doc}/README.md
    EOS
  end
end
