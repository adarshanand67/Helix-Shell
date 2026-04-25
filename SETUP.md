# Helix Shell — Setup & User Guide

Helix is a modern Unix shell that runs everywhere bash/zsh does, and adds things no other shell has — a built-in AI assistant, beautiful prompts, and zero configuration to get started.

---

## Table of Contents

1. [Installation](#installation)
2. [First Run](#first-run)
3. [Setting Up the AI Assistant](#setting-up-the-ai-assistant)
4. [Configuration (~/.helixrc)](#configuration-helixrc)
5. [Prompt](#prompt)
6. [Builtins Reference](#builtins-reference)
7. [AI Commands](#ai-commands)
8. [Job Control](#job-control)
9. [History](#history)
10. [Aliases](#aliases)
11. [Operators & Chaining](#operators--chaining)
12. [I/O Redirection](#io-redirection)
13. [Making Helix Your Default Shell](#making-helix-your-default-shell)
14. [Docker Usage](#docker-usage)
15. [Troubleshooting](#troubleshooting)

---

## Installation

### macOS (Homebrew) — recommended

```bash
brew tap adarshanand67/helix-shell
brew install helix-shell
```

Verify:
```bash
helix --version   # or just: helix
```

### From source (macOS & Linux)

**Prerequisites:**

| Tool | macOS | Ubuntu/Debian |
|---|---|---|
| C++23 compiler | `brew install llvm` or Xcode | `sudo apt install g++` |
| CMake ≥ 3.20 | `brew install cmake` | `sudo apt install cmake` |
| readline | `brew install readline` | `sudo apt install libreadline-dev` |
| CppUnit (tests) | `brew install cppunit` | `sudo apt install libcppunit-dev` |

```bash
git clone https://github.com/adarshanand67/Helix-Shell.git
cd Helix-Shell
./scripts/setup.sh    # installs all deps automatically
make build
./build/helix
```

### Docker (any platform)

```bash
docker pull adarshanand67/helixshell:latest
docker run -it --rm adarshanand67/helixshell:latest
```

With persistent workspace:
```bash
docker run -it --rm \
  -v $(pwd):/home/shelluser/workspace \
  adarshanand67/helixshell:latest
```

---

## First Run

```
Helix Shell v3.0  (type 'help' for commands, 'ai <query>' for AI assist)
✓ adarsh@MacBook ~/
❯
```

Type `help` to see all built-in commands. Type `exit` or press `Ctrl+D` to quit.

---

## Setting Up the AI Assistant

Helix works with any major AI provider. Set whichever key you have — it auto-detects:

| Provider | Env var | Default model | Cost |
|---|---|---|---|
| **Anthropic** | `ANTHROPIC_API_KEY` | `claude-haiku-4-5-20251001` | ~$0.001/query |
| **OpenAI** | `OPENAI_API_KEY` | `gpt-4o-mini` | ~$0.001/query |
| **Google** | `GOOGLE_API_KEY` or `GEMINI_API_KEY` | `gemini-2.0-flash` | free tier available |
| **Groq** | `GROQ_API_KEY` | `llama3-8b-8192` | free tier available |
| **Ollama** | *(no key)* | `llama3` | free, local |

**Quick start — pick any one:**
```bash
# Add to ~/.helixrc to persist
export ANTHROPIC_API_KEY=sk-ant-...
# or
export OPENAI_API_KEY=sk-...
# or
export GROQ_API_KEY=gsk_...     # free tier at console.groq.com
# or install Ollama (ollama.ai) for fully local, no-cost AI
```

**Override provider or model:**
```bash
export HELIX_AI_PROVIDER=groq
export HELIX_AI_MODEL=llama3-70b
```

**Test it:**
```bash
ai list all files larger than 100MB
# → find . -size +100M -type f
```

---

## Configuration (~/.helixrc)

`~/.helixrc` is sourced automatically when Helix starts. Use it for:

```bash
# ~/.helixrc

# Environment variables
export EDITOR=nvim
export ANTHROPIC_API_KEY=sk-ant-...

# Aliases
alias ll='ls -la'
alias la='ls -A'
alias l='ls -CF'
alias gs='git status'
alias gc='git commit'
alias gp='git push'
alias ..='cd ..'
alias ...='cd ../..'

# Custom prompt style (future: setopt)
```

Any valid Helix commands work — `cd`, `export`, `alias`, pipelines, etc.

---

## Prompt

The prompt shows:
```
✓ user@host ~/current/dir on ± branch-name ● ● took 3s
❯
```

| Symbol | Meaning |
|---|---|
| `✓` green | Last command succeeded (exit code 0) |
| `✗(1)` red | Last command failed (shows exit code) |
| `±` | You're in a git repository |
| `branch-name` | Current git branch (or short hash if detached HEAD) |
| `●` green | Staged changes |
| `●` orange | Unstaged changes |
| `●` gray | Untracked files |
| `took Ns` | Last command took ≥ 2s — only shown for slow commands |

---

## Builtins Reference

| Command | Usage | Description |
|---|---|---|
| `cd` | `cd [dir]` | Change directory. `cd -` goes to previous dir, `cd` goes home |
| `pwd` | `pwd` | Print working directory |
| `echo` | `echo [-n] [args]` | Print arguments. `-n` suppresses newline |
| `export` | `export VAR=val` | Set environment variable. `export` alone lists all |
| `unset` | `unset VAR` | Remove environment variable |
| `alias` | `alias [name=val]` | Define or list aliases |
| `unalias` | `unalias [-a] name` | Remove alias. `-a` removes all |
| `source` | `source file` or `. file` | Execute commands from a file in current shell context |
| `which` | `which cmd` | Show where a command resolves (path, alias, or builtin) |
| `type` | `type name` | Show what a name is (alias / builtin / external) |
| `history` | `history` | Show command history with numbers |
| `jobs` | `jobs` | List background jobs |
| `fg` | `fg <id>` | Bring background job to foreground |
| `bg` | `bg <id>` | Resume stopped job in background |
| `exit` | `exit [code]` | Exit the shell. Returns code to parent |
| `help` | `help` | Show all commands |
| `ai` | `ai <query>` | AI assistant — see below |

---

## AI Commands

Helix has a built-in AI assistant powered by Claude.

### `ai <description>` — suggest a command

```bash
ai find all python files modified in the last week
# → find . -name "*.py" -mtime -7

ai show disk usage by folder sorted by size
# → du -sh */ | sort -rh

ai count lines of code excluding tests
# → find . -name "*.cpp" ! -path "*/tests/*" | xargs wc -l
```

### `ai explain <command>` — understand a command

```bash
ai explain "find . -name '*.log' -mtime +30 -delete"
# The command searches for files ending in .log that were last modified
# more than 30 days ago and deletes them. The -delete flag is applied
# after matching so all criteria must match first. Risks: permanent deletion.
```

### `ai fix <error>` — get help from error output

```bash
ai fix "permission denied: /usr/local/bin/npm"
# Try: sudo chown -R $(whoami) /usr/local/bin

ai fix "git push rejected non-fast-forward"
# Run: git pull --rebase origin main
# Then retry your push.
```

### `ai run <description>` — suggest and confirm before running

```bash
ai run compress all logs older than 7 days
# → find . -name "*.log" -mtime +7 -exec gzip {} \;
# Run this command? [y/N] y
# Running: find . -name "*.log" -mtime +7 -exec gzip {} \;
```

---

## Job Control

Run a command in the background:
```bash
sleep 30 &          # starts in background, prints PID
jobs                # list all background jobs
fg 1                # bring job 1 to foreground
bg 1                # resume stopped job 1 in background
```

Stop a foreground job:
- `Ctrl+Z` — stops (suspends) the job, returns prompt
- Then use `fg` or `bg` to continue it

---

## History

```bash
history             # show all history with line numbers
!!                  # repeat the last command
!5                  # repeat command number 5
Ctrl+R              # reverse-incremental search through history
```

History is saved to `~/.helix_history` and loaded on every start — your history persists across sessions.

---

## Aliases

```bash
alias ll='ls -la'           # define
alias ll                    # print definition: alias ll='ls -la'
alias                       # list all aliases
unalias ll                  # remove single alias
unalias -a                  # remove all aliases
```

Aliases are expanded before any other processing, so they can contain pipes, redirections, and arguments:
```bash
alias glog="git log --oneline --graph --decorate"
glog                        # works exactly as if you typed the full command
```

To persist aliases, add them to `~/.helixrc`.

---

## Operators & Chaining

```bash
# Sequence — run commands one after another regardless of exit code
mkdir tmp; cd tmp; ls

# AND — run next only if previous succeeded
make build && ./build/helix

# OR — run next only if previous failed
git pull || echo "pull failed, check connection"

# Background — run command without waiting
long-running-task &

# Pipelines — connect stdout of one command to stdin of next
cat access.log | grep ERROR | sort | uniq -c | sort -rn | head -20
```

Operators can be combined:
```bash
make clean && make build && make test || echo "build failed"
```

---

## I/O Redirection

```bash
command > out.txt       # stdout to file (overwrite)
command >> out.txt      # stdout to file (append)
command < in.txt        # stdin from file
command 2> err.txt      # stderr to file
command 2>> err.txt     # stderr to file (append)
command > out.txt 2>&1  # both stdout and stderr (use shell built-in)

# Examples
ls -la > listing.txt
find . -name "*.tmp" 2> /dev/null
make build 2>&1 | tee build.log
```

---

## Making Helix Your Default Shell

```bash
# Add helix to /etc/shells (macOS/Linux)
which helix | sudo tee -a /etc/shells

# Set as default
chsh -s $(which helix)
```

To switch back to zsh/bash at any time: `chsh -s /bin/zsh`.

---

## Docker Usage

**Quick start:**
```bash
docker run -it --rm adarshanand67/helixshell:latest
```

**Mount your project:**
```bash
docker run -it --rm -v $(pwd):/workspace adarshanand67/helixshell:latest
```

**Docker Compose:**
```yaml
# docker-compose.yml
services:
  shell:
    image: adarshanand67/helixshell:latest
    volumes:
      - ./workspace:/home/shelluser/workspace
    stdin_open: true
    tty: true
```

```bash
docker compose run shell
```

---

## Troubleshooting

**readline not found during build**
```bash
# macOS
brew install readline
export LDFLAGS="-L$(brew --prefix readline)/lib"
export CPPFLAGS="-I$(brew --prefix readline)/include"

# Linux
sudo apt install libreadline-dev
```

**`ai` command says ANTHROPIC_API_KEY not set**
```bash
export ANTHROPIC_API_KEY=sk-ant-...
# or add to ~/.helixrc for persistence
```

**Git branch not showing in prompt**

Helix traverses parent directories to find `.git`. Make sure you're inside a git repository. Run `git status` to confirm git recognizes the directory.

**Helix is slow on large repos (git status in prompt)**

The prompt runs `git status --porcelain` on every prompt render. In repos with hundreds of thousands of files this can be slow. You can disable the status indicators by setting:
```bash
export HELIX_NO_GIT_STATUS=1
```
(requires rebuilding from source with this env var check added)

**History not persisting**

Check that `~/.helix_history` is writable:
```bash
ls -la ~/.helix_history
touch ~/.helix_history   # create if missing
```

**`fg` / `bg` not working**

Job control requires the shell to be running in a terminal (not piped). Make sure you started `helix` interactively.
