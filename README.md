# Helix Shell

[![CI/CD](https://github.com/adarshanand67/Helix-Shell/actions/workflows/ci.yml/badge.svg)](https://github.com/adarshanand67/Helix-Shell/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey)](#install)

**A modern Unix shell with a built-in AI assistant.**

Ask it to find files, compress things, debug errors — in plain English. It suggests the command, you can edit it, then run it.

```
✓ adarsh@MacBook ~/projects on ± main ● took 4s
❯ ai find all log files older than 7 days and compress them
→ find . -name "*.log" -mtime +7 -exec gzip {} \;

❯ ai explain "rsync -avz --delete src/ dest/"
Copies src/ to dest/ using rsync with archive mode (preserves permissions,
timestamps, symlinks), verbose output, compression over the wire, and deletes
files in dest/ that no longer exist in src/. Risks: the --delete flag is
destructive.
```

---

## What makes Helix different

| | Feature | No other shell has this |
|---|---|---|
| **AI assistant** | `ai <description>` → command suggestion | ✓ |
| **`ai explain`** | Plain-English explanation of any command | ✓ |
| **`ai fix`** | Paste an error, get a fix suggestion | ✓ |
| **`ai run`** | Suggest + confirm + execute | ✓ |
| **Git status in prompt** | Staged ● / dirty ● / untracked ● icons | Most shells need plugins |
| **Slow command timer** | `took 4s` shown after commands ≥ 2s | Most shells need plugins |
| **`~/.helixrc`** | Startup config file, no plugin manager needed | ✓ |
| **Persistent history** | `~/.helix_history` survives restarts + `Ctrl+R` | Standard |
| **`source` builtin** | Execute a file in current shell context | Standard |
| **`which` builtin** | Shows alias/builtin/path — no external `which` needed | ✓ |
| **`type` builtin** | Shows what a name resolves to | ✓ |
| **`!!` / `!n`** | Repeat last or nth command | Standard |
| **`&&` / `\|\|`** | Conditional chaining | Standard |

---

## Install

**macOS — pre-built binary (fastest, no compiler needed)**
```bash
brew tap adarshanand67/helix-shell
brew install helix-shell-bin
```

**macOS — GUI .app**
```bash
brew tap adarshanand67/helix-shell
brew install --cask helix-shell
```

**macOS — build from source**
```bash
brew tap adarshanand67/helix-shell
brew install helix-shell
```

**Linux / macOS — curl installer**
```bash
curl -fsSL https://raw.githubusercontent.com/adarshanand67/Helix-Shell/master/scripts/install.sh | bash
```

**Docker**
```bash
docker run -it --rm adarshanand67/helixshell:latest
```

**From source**
```bash
git clone https://github.com/adarshanand67/Helix-Shell.git
cd Helix-Shell
./scripts/setup.sh    # installs cmake + all deps
make build
./build/helix
```

Full setup guide: [SETUP.md](SETUP.md)

---

## AI Assistant

Works with any major AI provider — no vendor lock-in. Set whichever key you have:

```bash
export ANTHROPIC_API_KEY=sk-ant-...    # Claude (recommended)
export OPENAI_API_KEY=sk-...           # GPT-4o-mini
export GOOGLE_API_KEY=AIza...          # Gemini 2.0 Flash
export GROQ_API_KEY=gsk_...            # Llama3 via Groq (free tier)
# No key? Install Ollama — runs llama3 locally, completely free
```

Auto-detection picks the first key found. Override with:
```bash
export HELIX_AI_PROVIDER=groq          # force a specific provider
export HELIX_AI_MODEL=llama3-70b       # override the default model
```

### Commands

```bash
# Suggest a command from a description
ai find all python files modified in the last week
# → find . -name "*.py" -mtime -7

# Explain what a command does
ai explain "awk '{sum += $1} END {print sum}' data.txt"
# Sums the first column of data.txt and prints the total.

# Get a fix for an error
ai fix "npm ERR! EACCES permission denied /usr/local/lib/node_modules"
# → sudo chown -R $(whoami) ~/.npm /usr/local/lib/node_modules

# Suggest and run with confirmation
ai run delete all .DS_Store files recursively
# → find . -name ".DS_Store" -delete
# Run this command? [y/N]
```

---

## Prompt

```
✓ adarsh@MacBook ~/projects/helix on ± main ● ● took 3s
❯
```

| | Meaning |
|---|---|
| `✓` / `✗(1)` | Last exit code — green success, red with code on failure |
| `on ± main` | Git branch (traverses parent dirs automatically) |
| `●` green | Staged changes |
| `●` orange | Unstaged changes |
| `●` gray | Untracked files |
| `took 3s` | Only shown when last command took ≥ 2 seconds |

---

## Configuration

Create `~/.helixrc` — it's sourced on every startup:

```bash
# ~/.helixrc
export EDITOR=nvim
export ANTHROPIC_API_KEY=sk-ant-...

alias ll='ls -la'
alias gs='git status'
alias glog='git log --oneline --graph --decorate'
alias ..='cd ..'
```

No plugin manager. No framework. Just a file.

---

## Full Feature Reference

```bash
# Builtins
cd, pwd, echo -n, export, unset
alias, unalias -a, type, which, source / .
history, jobs, fg, bg, exit, help

# AI
ai <description>        # suggest command
ai explain <command>    # explain it
ai fix <error>          # suggest a fix
ai run <description>    # suggest + confirm + run

# History expansion
!!       repeat last command
!5       repeat command #5
Ctrl+R   reverse search

# Operators
cmd1; cmd2          sequence
cmd1 && cmd2        run cmd2 only if cmd1 succeeds
cmd1 || cmd2        run cmd2 only if cmd1 fails
cmd &               run in background

# Pipelines
ls | grep foo | wc -l

# Redirection
cmd > out.txt       overwrite
cmd >> out.txt      append
cmd < in.txt        stdin from file
cmd 2> err.txt      stderr
cmd 2>> err.txt     stderr append
```

---

## Build & Test

```bash
make build    # build → ./build/helix
make test     # run tests
make clean    # remove build dir
```

CI: macOS + Linux builds, code coverage, valgrind memory check, cppcheck static analysis.

---

## Architecture

```
src/
  shell.cpp                  REPL, chaining, alias expansion, history, RC file, timer
  tokenizer.cpp              state-machine lexer
  parser.cpp                 pipeline + redirection AST
  executor.cpp               fork/exec coordinator
  prompt.cpp                 colored prompt, git branch + status, duration
  readline_support.cpp       TAB completion
  shell/
    builtin_handler.cpp      all builtins including ai, source, which, type
    job_manager.cpp          SIGCHLD background job tracking
  executor/
    executable_resolver.cpp  PATH lookup
    environment_expander.cpp $VAR / ${VAR} / ~ expansion (single-pass)
    fd_manager.cpp           I/O redirections
    pipeline_manager.cpp     N-stage pipe orchestration
```

Every executor component depends on a pure interface — independently unit-testable.

---

## Docker

```bash
docker build -t helixshell .
docker run -it --rm -v $(pwd):/workspace helixshell
```

See [DOCKER-QUICKSTART.md](DOCKER-QUICKSTART.md) and [SETUP.md#docker-usage](SETUP.md#docker-usage).

---

## License

MIT — see [LICENSE](LICENSE).
