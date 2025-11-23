# Aesthetic PS1 Prompt & Autocompletion

## Beautiful Prompt System

HelixShell features a modern, colorful prompt inspired by popular shells like Fish and Starship.

### Prompt Components

```
✓ user@hostname ~/Documents/project on ± master
❯
```

#### Status Indicator
- **✓** (green): Last command succeeded (exit code 0)
- **✗** (red): Last command failed (non-zero exit code)

#### User & Host
- Displayed in **bright cyan** with bold formatting
- Format: `user@hostname:`

#### Current Directory
- Displayed in **bright blue**
- Home directory shown as `~`
- Long paths intelligently shortened:
  - Paths > 40 characters show first directory + "..." + last 20 chars
  - Example: `/very/long/path/...Documents/project`

#### Git Integration
- Automatically detects if current directory is in a Git repository
- Shows branch name or commit hash (detached HEAD)
- Format: `on ± branch-name` in yellow
- Only displayed when in a Git repository

#### Prompt Character
- **❯** (green): Ready for input
- Multi-line design for better readability

### Color Scheme

| Element | Color | Purpose |
|---------|-------|---------|
| Status (✓) | Green | Success feedback |
| Status (✗) | Red | Error feedback |
| User@Host | Bright Cyan (Bold) | Identity |
| Directory | Bright Blue | Location |
| Git Symbol (±) | Bright Magenta | VCS indicator |
| Git Branch | Yellow | Branch name |
| Prompt (❯) | Bright Green | Input indicator |

### Examples

**Success in non-Git directory:**
```
✓ alice@macbook ~/Documents
❯
```

**Error in Git repository:**
```
✗ alice@macbook ~/code/myproject on ± feature-branch
❯
```

**Long path with Git:**
```
✓ alice@macbook ~/very/long/.../final/directory on ± master
❯
```

**Detached HEAD state:**
```
✓ alice@macbook ~/project on ± a1b2c3d
❯
```

## Autocompletion System

HelixShell provides intelligent tab completion for commands and paths.

### Command Completion

When typing a command (first token), pressing TAB completes from:
- **Built-in commands**: `cd`, `pwd`, `history`, `exit`, `jobs`, `fg`, `bg`
- **System commands**: All executables in `$PATH`

#### Examples

```bash
❯ ec[TAB]
→ echo

❯ his[TAB]
→ history

❯ gre[TAB]
→ grep
```

### Path Completion

When typing paths (after the first token), pressing TAB completes from:
- Files and directories in current directory
- Directories marked with trailing `/`
- Hidden files (starting with `.`) when explicitly typed

#### Examples

```bash
❯ cd Doc[TAB]
→ cd Documents/

❯ cat ~/.bash[TAB]
→ cat ~/.bashrc

❯ ls /usr/loc[TAB]
→ ls /usr/local/
```

### Multiple Completions

When multiple matches exist, all options are displayed:

```bash
❯ cd D[TAB]
Documents/  Downloads/  Desktop/
❯ cd D
```

User can then type more characters and TAB again to narrow down.

### Completion Behavior

1. **Single Match**: Auto-completes immediately
2. **Multiple Matches**: Shows all options, waits for more input
3. **No Matches**: No change, no output

### Supported Features

✅ Command completion from PATH
✅ Built-in command completion
✅ File and directory path completion
✅ Home directory expansion (`~`)
✅ Absolute path completion
✅ Relative path completion
✅ Directory trailing slash indication

### Future Enhancements

Planned improvements:
- [ ] Command history-based completion
- [ ] Smart suggestions (most used commands)
- [ ] Inline completions (Fish-style)
- [ ] Fuzzy matching
- [ ] Option/flag completion (e.g., `git --[TAB]`)
- [ ] Multi-cursor position completion

## Technical Implementation

### Prompt Generation

The prompt is generated in `showPrompt()` using ANSI escape codes:
- Color codes defined in `Colors` namespace
- Git branch detected via `.git/HEAD` file parsing
- Exit status from `last_exit_status` variable

### Autocompletion

Three main functions power autocompletion:

1. **`getCommandCompletions()`**
   - Searches PATH directories
   - Filters executable files
   - Includes built-in commands

2. **`getPathCompletions()`**
   - Scans directory entries
   - Handles ~ expansion
   - Distinguishes files from directories

3. **`handleTabCompletion()`**
   - Determines context (command vs. path)
   - Calls appropriate completion function
   - Formats output for user

### ANSI Color Codes

```cpp
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string GREEN = "\033[32m";
    const std::string CYAN = "\033[36m";
    const std::string YELLOW = "\033[33m";
    // ... more colors
}
```

Usage:
```cpp
std::cout << Colors::GREEN << "Success!" << Colors::RESET << "\n";
```

## Configuration

### Disabling Colors

If your terminal doesn't support colors, they can be disabled by setting `NO_COLOR` environment variable:

```bash
export NO_COLOR=1
./hsh
```

(Future feature - not yet implemented)

### Customizing Prompt

Future versions will support custom prompt formats via:
- Environment variables (`HSH_PROMPT_FORMAT`)
- Configuration file (`~/.hshrc`)
- Runtime commands (`prompt set`)

## Compatibility

### Terminal Support

The prompt works best with terminals supporting:
- ✅ 256-color ANSI
- ✅ Unicode characters (✓, ✗, ❯, ±)
- ✅ Bold/bright text

### Tested Terminals

| Terminal | Status | Notes |
|----------|--------|-------|
| iTerm2 (macOS) | ✅ Full | All features work |
| Terminal.app (macOS) | ✅ Full | All features work |
| GNOME Terminal (Linux) | ✅ Full | All features work |
| Konsole (Linux) | ✅ Full | All features work |
| Windows Terminal | ⚠️ Partial | Unicode may vary |
| PuTTY | ⚠️ Partial | Limited color support |

## Performance

- **Prompt Generation**: < 1ms
- **Git Branch Detection**: < 5ms (cached by filesystem)
- **Command Completion**: 10-50ms (depends on PATH size)
- **Path Completion**: < 10ms (single directory scan)

All operations are non-blocking and provide instant feedback.

## Examples in Practice

### Development Workflow

```bash
✓ alice@dev ~/project on ± feature-ui-redesign
❯ git status
On branch feature-ui-redesign
...

✗ alice@dev ~/project on ± feature-ui-redesign  # Failed test
❯ make test
...

✓ alice@dev ~/project on ± feature-ui-redesign  # Tests pass
❯ git commit -m "Fix UI layout"

✓ alice@dev ~/project on ± feature-ui-redesign
❯ cd ../other-project

✓ alice@dev ~/other-project  # No Git here
❯
```

### Autocompletion in Action

```bash
❯ cd Doc[TAB]
→ cd Documents/

❯ ls D[TAB]
Desktop/  Documents/  Downloads/
❯ ls Doc[TAB]
→ ls Documents/

❯ cat README[TAB]
→ cat README.md

❯ ec[TAB]
→ echo

❯ ./scr[TAB]
scripts/  src/
❯ ./scripts/[TAB]
setup.sh  setup-hooks.sh  README.md
```

---

**Version**: 2.0
**Last Updated**: 2025-01-23
**Author**: HelixShell Team
