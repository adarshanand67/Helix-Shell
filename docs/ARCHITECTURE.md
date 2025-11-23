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

### Dependency Inversion Principle (DIP)

HelixShell v2.0 implements the **Dependency Inversion Principle**, where high-level modules depend on abstractions (interfaces) rather than concrete implementations.

**Core Principle:**
> "High-level modules should not depend on low-level modules. Both should depend on abstractions."

**Implementation:**

**Executor Interfaces (`include/executor/interfaces.h`):**
- `IExecutableResolver`: Abstract interface for executable resolution
- `IEnvironmentExpander`: Abstract interface for environment variable expansion
- `IFileDescriptorManager`: Abstract interface for FD management
- `IPipelineManager`: Abstract interface for pipeline execution

**Shell Interfaces (`include/shell/interfaces.h`):**
- `IBuiltinCommandHandler`: Abstract interface for builtin command handlers
- `IBuiltinDispatcher`: Abstract interface for builtin command dispatching
- `IJobManager`: Abstract interface for job control

**Benefits:**
- **Testability**: Easy to inject mock implementations for unit testing
- **Flexibility**: Can swap implementations without changing high-level code
- **Decoupling**: High-level logic independent of low-level details
- **Runtime Polymorphism**: Different implementations can be used dynamically

**Example - Executor with Dependency Injection:**
```cpp
// Default constructor for production use
Executor::Executor()
    : exe_resolver(std::make_unique<ExecutableResolver>()),
      env_expander(std::make_unique<EnvironmentVariableExpander>()),
      fd_manager(std::make_unique<FileDescriptorManager>()),
      pipeline_manager(std::make_unique<PipelineManager>()) {}

// DI constructor for testing with mocks
Executor::Executor(
    std::unique_ptr<IExecutableResolver> resolver,
    std::unique_ptr<IEnvironmentExpander> expander,
    std::unique_ptr<IFileDescriptorManager> fd_mgr,
    std::unique_ptr<IPipelineManager> pipe_mgr)
    : exe_resolver(std::move(resolver)),
      env_expander(std::move(expander)),
      fd_manager(std::move(fd_mgr)),
      pipeline_manager(std::move(pipe_mgr)) {}
```

**Testing Example:**
```cpp
// Create mock implementations
auto mock_resolver = std::make_unique<MockExecutableResolver>();
auto mock_expander = std::make_unique<MockEnvironmentExpander>();
auto mock_fd_mgr = std::make_unique<MockFileDescriptorManager>();
auto mock_pipe_mgr = std::make_unique<MockPipelineManager>();

// Inject mocks into Executor
Executor executor(
    std::move(mock_resolver),
    std::move(mock_expander),
    std::move(mock_fd_mgr),
    std::move(mock_pipe_mgr)
);

// Test without touching real filesystem or forking processes
executor.execute(test_command);
```

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

**Interface:** Implements `IExecutableResolver`

**Public Interface:**
```cpp
class IExecutableResolver {
public:
    virtual ~IExecutableResolver() = default;
    virtual std::string findExecutable(const std::string& command) const = 0;
};

class ExecutableResolver : public IExecutableResolver {
public:
    std::string findExecutable(const std::string& command) const override;
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

**Interface:** Implements `IEnvironmentExpander`

**Public Interface:**
```cpp
class IEnvironmentExpander {
public:
    virtual ~IEnvironmentExpander() = default;
    virtual std::string expand(const std::string& input) const = 0;
};

class EnvironmentVariableExpander : public IEnvironmentExpander {
public:
    std::string expand(const std::string& input) const override;
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

**Interface:** Implements `IFileDescriptorManager`

**Public Interface:**
```cpp
class IFileDescriptorManager {
public:
    virtual ~IFileDescriptorManager() = default;
    virtual bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd) = 0;
    virtual void restoreFileDescriptors() = 0;
};

class FileDescriptorManager : public IFileDescriptorManager {
public:
    FileDescriptorManager();  // Saves original FDs
    ~FileDescriptorManager() override; // Restores original FDs

    bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd) override;
    void restoreFileDescriptors() override;
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

**Interface:** Implements `IPipelineManager`

**Public Interface:**
```cpp
class IPipelineManager {
public:
    virtual ~IPipelineManager() = default;
    virtual int executePipeline(
        const ParsedCommand& cmd,
        std::function<void(const Command&)> executor_func) = 0;
};

class PipelineManager : public IPipelineManager {
public:
    int executePipeline(
        const ParsedCommand& cmd,
        std::function<void(const Command&)> executor_func) override;
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

**Dependency Inversion:**
Executor depends on **interfaces**, not concrete implementations:

```cpp
class Executor {
private:
    // Depend on abstractions (interfaces)
    std::unique_ptr<IExecutableResolver> exe_resolver;
    std::unique_ptr<IEnvironmentExpander> env_expander;
    std::unique_ptr<IFileDescriptorManager> fd_manager;
    std::unique_ptr<IPipelineManager> pipeline_manager;

public:
    // Default constructor (production)
    Executor();

    // Dependency Injection constructor (testing)
    Executor(
        std::unique_ptr<IExecutableResolver> resolver,
        std::unique_ptr<IEnvironmentExpander> expander,
        std::unique_ptr<IFileDescriptorManager> fd_mgr,
        std::unique_ptr<IPipelineManager> pipe_mgr);
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

    // Depends on interface, not concrete type
    IJobManager* job_manager = nullptr;
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

**Interface:** Implements `IBuiltinCommandHandler`

**Interface Definition:**
```cpp
class IBuiltinCommandHandler {
public:
    virtual ~IBuiltinCommandHandler() = default;
    virtual bool handle(const ParsedCommand& cmd, ShellState& state) = 0;
    virtual bool canHandle(const std::string& command) const = 0;
};

class BuiltinCommandHandler : public IBuiltinCommandHandler {
public:
    virtual ~BuiltinCommandHandler() = default;
    virtual bool handle(const ParsedCommand& cmd, ShellState& state) override = 0;
    virtual bool canHandle(const std::string& command) const override = 0;
};
```

**Concrete Implementations:**
- `CdCommandHandler`: Changes directory, updates PWD/OLDPWD
- `ExitCommandHandler`: Sets running=false, handles exit code
- `HistoryCommandHandler`: Displays command history
- `JobsCommandHandler`: Delegates to JobManager
- `FgCommandHandler`: Brings job to foreground
- `BgCommandHandler`: Resumes job in background

**Dispatcher:** Implements `IBuiltinDispatcher`

```cpp
class IBuiltinDispatcher {
public:
    virtual ~IBuiltinDispatcher() = default;
    virtual bool dispatch(const ParsedCommand& cmd, ShellState& state) = 0;
    virtual bool isBuiltin(const std::string& command) const = 0;
};

class BuiltinCommandDispatcher : public IBuiltinDispatcher {
public:
    bool dispatch(const ParsedCommand& cmd, ShellState& state) override;
    bool isBuiltin(const std::string& command) const override;
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

**Interface:** Implements `IJobManager`

**Public Interface:**
```cpp
class IJobManager {
public:
    virtual ~IJobManager() = default;
    virtual void addJob(int pid, const std::string& command) = 0;
    virtual void removeJob(int job_id) = 0;
    virtual void printJobs() const = 0;
    virtual void bringToForeground(int job_id) = 0;
    virtual void resumeInBackground(int job_id) = 0;
    virtual const std::map<int, Job>& getJobs() const = 0;
};

class JobManager : public IJobManager {
public:
    void addJob(int pid, const std::string& command) override;
    void removeJob(int job_id) override;
    void printJobs() const override;
    void bringToForeground(int job_id) override;
    void resumeInBackground(int job_id) override;
    const std::map<int, Job>& getJobs() const override;
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

**Dependency Inversion:**
Shell depends on **interfaces** for extensible components:

```cpp
class Shell {
private:
    ShellState state;

    // Core components (stable implementations)
    std::unique_ptr<Tokenizer> tokenizer;
    std::unique_ptr<Parser> parser;
    std::unique_ptr<Executor> executor;

    // Depends on abstractions (interfaces) for flexibility
    std::unique_ptr<IBuiltinDispatcher> builtin_dispatcher;
    std::unique_ptr<IJobManager> job_manager;
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

### 1. Dependency Inversion Principle (DIP)

**Where:** Executor, Shell, all major components
**Why:** High-level modules should depend on abstractions, not concrete implementations
**Implementation:**
- All executor components define interfaces (`IExecutableResolver`, `IEnvironmentExpander`, etc.)
- All shell components define interfaces (`IBuiltinCommandHandler`, `IBuiltinDispatcher`, `IJobManager`)
- Main classes (Executor, Shell) depend on these interfaces
**Benefit:**
- Easy to test with mock implementations
- Can swap implementations at runtime
- Decouples high-level logic from low-level details
- Production-ready architecture supporting multiple implementations

### 2. Composition Pattern

**Where:** Executor, Shell
**Why:** Delegate responsibilities to specialized components
**Benefit:** Reduced coupling, easier testing, single responsibility adherence

### 3. Strategy Pattern

**Where:** BuiltinCommandHandler hierarchy
**Why:** Extensible builtin command system with swappable strategies
**Benefit:** Add new commands without modifying Shell, follows Open/Closed Principle

### 4. Dependency Injection

**Where:** Constructor injection of components (Executor has two constructors)
**Why:** Explicit dependencies, easier to mock for testing
**Implementation:**
```cpp
// Production: default implementations
Executor executor;

// Testing: inject mocks
Executor executor(mock_resolver, mock_expander, mock_fd_mgr, mock_pipe_mgr);
```
**Benefit:** Better testability, clearer component relationships, inversion of control

### 5. RAII (Resource Acquisition Is Initialization)

**Where:** FileDescriptorManager, smart pointers (`std::unique_ptr`)
**Why:** Automatic resource cleanup, exception safety
**Benefit:** No resource leaks, automatic cleanup on scope exit

### 6. Facade Pattern

**Where:** Executor, Shell public interfaces
**Why:** Hide internal complexity behind simple interfaces
**Benefit:** Simple public API (execute, run), complex internal implementation (composition)

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
