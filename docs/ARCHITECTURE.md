# HelixShell Architecture Documentation

## Overview

HelixShell v2.0 employs a **composition-based architecture** that prioritizes modularity, testability, and maintainability over traditional monolithic design. This document provides a comprehensive overview of the architectural decisions, component interactions, and design patterns used throughout the codebase.

## Design Philosophy

### Composition Over Inheritance

The architecture follows the principle of **composition over inheritance**, where complex functionality is built by composing simple, focused components rather than creating deep inheritance hierarchies.

**Benefits:**
- Greater flexibility in component reuse
- Easier to test components in isolation
- Reduced coupling between components
- Simpler to understand and modify

### Single Responsibility Principle

Each class has a single, well-defined responsibility:
- **ExecutableResolver**: Only PATH searching and executable validation
- **EnvironmentVariableExpander**: Only variable expansion
- **FileDescriptorManager**: Only FD redirection management
- **PipelineManager**: Only pipeline coordination

## Component Architecture

### Directory Structure

```
HelixShell/
├── include/
│   ├── executor/              # Executor components
│   │   ├── executable_resolver.h
│   │   ├── environment_expander.h
│   │   ├── fd_manager.h
│   │   └── pipeline_manager.h
│   ├── shell/                 # Shell components
│   │   ├── builtin_handler.h
│   │   ├── job_manager.h
│   │   └── shell_state.h
│   ├── executor.h             # Main executor (composition)
│   ├── shell.h                # Main shell (composition)
│   ├── parser.h
│   ├── tokenizer.h
│   ├── prompt.h
│   ├── readline_support.h
│   └── types.h
├── src/
│   ├── executor/              # Executor implementations
│   ├── shell/                 # Shell implementations
│   └── [other source files]
└── tests/
    └── [unit and integration tests]
```

## Executor Subsystem

### Architecture Diagram

```
┌─────────────────────────────────────────────────┐
│              Executor (Main)                    │
│  - Coordinates command execution                │
│  - Delegates to specialized components          │
└───────────┬──────────────────────────────┬──────┘
            │                              │
    ┌───────▼────────┐            ┌────────▼──────────┐
    │ Single Command │            │   Pipeline        │
    │   Execution    │            │   Execution       │
    └───────┬────────┘            └────────┬──────────┘
            │                              │
┌───────────▼────────────────────────────────▼─────────┐
│          Composition Components                      │
├──────────────────────────────────────────────────────┤
│  • ExecutableResolver                                │
│  • EnvironmentVariableExpander                       │
│  • FileDescriptorManager                             │
│  • PipelineManager                                   │
└──────────────────────────────────────────────────────┘
```

### Component Details

#### ExecutableResolver (~60 lines)

**Responsibility:** Locate executables in the system PATH

**Public Interface:**
```cpp
class ExecutableResolver {
public:
    std::string findExecutable(const std::string& command) const;
private:
    bool isExecutable(const std::string& path) const;
    std::string searchInPath(const std::string& command) const;
};
```

**Algorithm:**
1. Check if command contains `/` (absolute/relative path)
2. If yes, validate it directly
3. If no, iterate through PATH directories
4. For each directory, check if `directory/command` exists and is executable
5. Return first match or empty string

**Key Design Decisions:**
- Stateless: No member variables, purely functional
- Const methods: Safe to call from multiple contexts
- Early return: Optimize for common cases

#### EnvironmentVariableExpander (~45 lines)

**Responsibility:** Expand `$VAR` and `${VAR}` syntax

**Public Interface:**
```cpp
class EnvironmentVariableExpander {
public:
    std::string expand(const std::string& input) const;
private:
    std::string getVariableValue(const std::string& name) const;
};
```

**Algorithm:**
1. Use regex to find all `$VAR` and `${VAR}` patterns
2. For each match, look up environment variable
3. Replace with value (or empty string if not found)
4. Return expanded string

**Regex Pattern:** `\$\{([^}]+)\}|\$([A-Za-z_][A-Za-z0-9_]*)`

#### FileDescriptorManager (~130 lines)

**Responsibility:** Manage file descriptor redirections

**Public Interface:**
```cpp
class FileDescriptorManager {
public:
    FileDescriptorManager();  // Saves original FDs
    ~FileDescriptorManager(); // Restores original FDs

    bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd);
    void restoreFileDescriptors();
private:
    bool setupInputRedirection(const Command& cmd, int& input_fd);
    bool setupOutputRedirection(const Command& cmd, int& output_fd);
    bool setupErrorRedirection(const Command& cmd);

    int original_stdin, original_stdout, original_stderr;
};
```

**RAII Pattern:**
- Constructor saves original FDs using `dup()`
- Destructor restores FDs and closes saved copies
- Ensures cleanup even if exceptions occur

**Redirection Types:**
- Input (`<`): `open(O_RDONLY)` + `dup2(fd, STDIN_FILENO)`
- Output (`>`): `open(O_WRONLY|O_CREAT|O_TRUNC)` + `dup2(fd, STDOUT_FILENO)`
- Append (`>>`): `open(O_WRONLY|O_CREAT|O_APPEND)` + `dup2(fd, STDOUT_FILENO)`
- Error (`2>`): `open(O_WRONLY|O_CREAT|O_TRUNC)` + `dup2(fd, STDERR_FILENO)`

#### PipelineManager (~125 lines)

**Responsibility:** Execute multi-command pipelines

**Public Interface:**
```cpp
class PipelineManager {
public:
    int executePipeline(
        const ParsedCommand& cmd,
        std::function<void(const Command&)> executor_func);
private:
    std::vector<std::pair<int, int>> createPipes(size_t count);
    void cleanupPipes(std::vector<std::pair<int, int>>& pipes);
    int waitForPipeline(const std::vector<pid_t>& pids);
};
```

**Pipeline Execution Flow:**
1. Create N-1 pipes for N commands
2. For each command:
   - Fork a child process
   - In child:
     - Connect stdin to previous pipe (if not first)
     - Connect stdout to next pipe (if not last)
     - Close all pipe FDs
     - Call executor_func to execute command
   - In parent:
     - Track child PID
     - Close used pipe ends
3. Wait for all children
4. Return exit status of last command

**Critical FD Management:**
- Parent must close ALL pipe ends after forking
- Children must close ALL pipes after dup2
- Failure to close causes deadlocks (writer keeps pipe open)

### Main Executor

**Composition:**
```cpp
class Executor {
private:
    std::unique_ptr<ExecutableResolver> exe_resolver;
    std::unique_ptr<EnvironmentVariableExpander> env_expander;
    std::unique_ptr<FileDescriptorManager> fd_manager;
    std::unique_ptr<PipelineManager> pipeline_manager;
};
```

**Execution Flow:**
1. Parse command to determine if single or pipeline
2. If single:
   - Fork child
   - Use `fd_manager` to setup redirections
   - Use `env_expander` to expand variables
   - Use `exe_resolver` to find executable
   - `execvp` the command
3. If pipeline:
   - Create executor lambda that captures components
   - Pass lambda to `pipeline_manager`
   - Pipeline manager handles fork/pipe/exec coordination

## Shell Subsystem

### Architecture Diagram

```
┌─────────────────────────────────────────────────┐
│              Shell (Main REPL)                  │
│  - Read input                                   │
│  - Display prompt                               │
│  - Coordinate execution                         │
└───────────┬─────────────────────────────────────┘
            │
    ┌───────▼────────┐
    │   ShellState   │
    │  (Centralized) │
    └───────┬────────┘
            │
┌───────────▼──────────────────────────────────────┐
│          Composition Components                  │
├──────────────────────────────────────────────────┤
│  • BuiltinCommandDispatcher                      │
│  • JobManager                                    │
│  • Tokenizer                                     │
│  • Parser                                        │
│  • Executor                                      │
│  • Prompt                                        │
└──────────────────────────────────────────────────┘
```

### Component Details

#### ShellState (~30 lines)

**Responsibility:** Centralize all shell state

**Structure:**
```cpp
struct ShellState {
    std::string current_directory;
    std::string home_directory;
    int last_exit_status = 0;
    bool running = true;

    std::vector<std::string> command_history;
    std::map<std::string, std::string> environment;

    JobManager* job_manager = nullptr;
    Prompt* prompt = nullptr;
};
```

**Benefits:**
- Single source of truth for state
- Easy to pass to builtin handlers
- Clear dependency on JobManager and Prompt
- Simplifies testing (can mock state)

#### BuiltinCommandHandler (~160 lines total)

**Responsibility:** Implement builtin commands using Strategy pattern

**Interface:**
```cpp
class BuiltinCommandHandler {
public:
    virtual ~BuiltinCommandHandler() = default;
    virtual bool handle(const ParsedCommand& cmd, ShellState& state) = 0;
    virtual bool canHandle(const std::string& command) const = 0;
};
```

**Concrete Implementations:**
- `CdCommandHandler`: Changes directory, updates PWD/OLDPWD
- `ExitCommandHandler`: Sets running=false, handles exit code
- `HistoryCommandHandler`: Displays command history
- `JobsCommandHandler`: Delegates to JobManager
- `FgCommandHandler`: Brings job to foreground
- `BgCommandHandler`: Resumes job in background

**Dispatcher:**
```cpp
class BuiltinCommandDispatcher {
public:
    bool dispatch(const ParsedCommand& cmd, ShellState& state);
    bool isBuiltin(const std::string& command) const;
private:
    std::map<std::string, std::unique_ptr<BuiltinCommandHandler>> handlers;
};
```

**Adding New Builtins:**
1. Create new handler class inheriting from `BuiltinCommandHandler`
2. Implement `handle()` and `canHandle()` methods
3. Register in dispatcher constructor
4. No changes to Shell class needed!

#### JobManager (~50 lines)

**Responsibility:** Manage background and foreground jobs

**Public Interface:**
```cpp
class JobManager {
public:
    void addJob(int pid, const std::string& command);
    void removeJob(int job_id);
    void printJobs() const;
    void bringToForeground(int job_id);
    void resumeInBackground(int job_id);
    const std::map<int, Job>& getJobs() const;
private:
    std::map<int, Job> jobs;
    int next_job_id = 1;
};
```

**Job Lifecycle:**
1. Command executed with `&` → `addJob()`
2. User runs `jobs` → `printJobs()`
3. User runs `fg %1` → `bringToForeground(1)`
4. Job completes → `removeJob(1)`

### Main Shell

**Composition:**
```cpp
class Shell {
private:
    ShellState state;

    Tokenizer tokenizer;
    Parser parser;
    Executor executor;
    Prompt prompt;

    std::unique_ptr<BuiltinCommandDispatcher> builtin_dispatcher;
    std::unique_ptr<JobManager> job_manager;
};
```

**REPL Flow:**
1. `showPrompt()`: Display prompt
2. `readInput()`: Get user input via readline
3. `processInput()`:
   - Tokenize input
   - Parse tokens
   - Check if builtin (dispatch)
   - If not builtin, execute via Executor
   - Update state
4. Repeat until `state.running == false`

## Design Patterns

### 1. Composition Pattern

**Where:** Executor, Shell
**Why:** Delegate responsibilities to specialized components
**Benefit:** Reduced coupling, easier testing

### 2. Strategy Pattern

**Where:** BuiltinCommandHandler hierarchy
**Why:** Extensible builtin command system
**Benefit:** Add new commands without modifying Shell

### 3. Dependency Injection

**Where:** Constructor injection of components
**Why:** Explicit dependencies, easier to mock for testing
**Benefit:** Better testability, clearer component relationships

### 4. RAII (Resource Acquisition Is Initialization)

**Where:** FileDescriptorManager, smart pointers
**Why:** Automatic resource cleanup
**Benefit:** Exception safety, no resource leaks

### 5. Facade Pattern

**Where:** Executor, Shell public interfaces
**Why:** Hide internal complexity
**Benefit:** Simple public API, complex internal implementation

## Testing Strategy

### Unit Testing

Each component can be tested independently:

**ExecutableResolver:**
```cpp
ExecutableResolver resolver;
ASSERT_EQ("/bin/ls", resolver.findExecutable("ls"));
ASSERT_EQ("", resolver.findExecutable("nonexistent"));
```

**EnvironmentVariableExpander:**
```cpp
setenv("TEST_VAR", "value", 1);
EnvironmentVariableExpander expander;
ASSERT_EQ("value", expander.expand("$TEST_VAR"));
ASSERT_EQ("value", expander.expand("${TEST_VAR}"));
```

### Integration Testing

Full shell functionality tested through `Shell::processInputString()`:

```cpp
Shell shell;
ASSERT_TRUE(shell.processInputString("echo hello"));
ASSERT_TRUE(shell.processInputString("cd /tmp"));
ASSERT_FALSE(shell.processInputString("exit"));
```

### Current Test Coverage

- 48 unit and integration tests
- All tests passing after refactoring
- 100% functional parity with pre-refactoring code

## Performance Characteristics

### Memory Usage

- Smart pointers ensure automatic cleanup
- No memory leaks (verified with Valgrind)
- Minimal memory overhead from composition (~8 pointers per Executor/Shell)

### Execution Overhead

- Component calls are function calls (minimal overhead)
- No virtual function calls in hot paths (except builtin dispatch)
- Pipeline performance identical to monolithic version

### Code Metrics

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Executor LOC | 435 | 180 | -59% |
| Shell LOC | 425 | 315 | -26% |
| Total Classes | 7 | 17 | +143% |
| Avg Class Size | ~200 | ~70 | -65% |
| Cyclomatic Complexity | High | Low | ↓↓↓ |

## Future Enhancements

### Extensibility Points

1. **New Builtin Commands:**
   - Implement `BuiltinCommandHandler`
   - Register in dispatcher
   - Zero changes to Shell class

2. **Alternative Execution Strategies:**
   - Swap `PipelineManager` implementation
   - E.g., use `posix_spawn` instead of fork/exec

3. **Plugin System:**
   - Load handlers dynamically at runtime
   - Extend shell without recompilation

### Refactoring Opportunities

1. **Tokenizer/Parser:**
   - Could benefit from similar composition approach
   - Extract quote handling, escape processing

2. **Prompt:**
   - Already relatively clean
   - Could extract GitInfoProvider, PathFormatter

3. **Configuration:**
   - Add ConfigManager component
   - Load settings from `.helixrc`

## Conclusion

The composition-based architecture provides:

✅ **Modularity**: Each component has clear boundaries
✅ **Testability**: Components testable in isolation
✅ **Maintainability**: Changes localized to specific components
✅ **Extensibility**: Easy to add new features
✅ **Readability**: Smaller, focused classes easier to understand

This architecture demonstrates professional software engineering practices while maintaining the shell's simplicity and performance.
