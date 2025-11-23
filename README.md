# Custom Shell (Helix Shell - hsh)

## Product Requirements Document (PRD)

### Objective
Develop a Unix-like custom shell in C++ that provides a command-line interface for users to interact with the operating system. The shell should support essential features such as command parsing, piping, input/output redirection, background job management, and signal handling. The project aims to deepen understanding of process management, system calls, and memory management, while demonstrating proficiency in C++ and systems programming.

### Features

#### Interactive command prompt with support for built-in and external commands
- Command parsing supporting multiple arguments, quotes, and escape characters
- Support for piping commands to pass output of one command as input to another
- Input and output redirection using `<`, `>`, and `>>`
- Error redirection using `2>`, `2>>`
- Background job execution using `&` with job control (listing, foreground, background)
- Signal handling for interrupts (e.g., Ctrl+C) and job control signals (e.g., Ctrl+Z)
- Environment variable management and command history
- Support for simple scripting with command sequences and conditional execution

### Functional Requirements

- **Command Parsing and Execution**: The shell must accept user input and parse commands correctly into executable components
- **Process Management**: It must fork child processes to execute external commands, handling process creation and termination
- **Piping**: Implement piping to connect standard output of one process to standard input of another
- **I/O Redirection**: Support redirection operators to read input from files and write output to files, with append functionality
- **Background Processes**: Manage background processes, allowing the shell to continue accepting commands without waiting for process completion
- **Signal Handling**: Handle Unix signals gracefully, ensuring the shell remains stable and responsive
- **Command History**: Maintain a command history accessible via keyboard shortcuts or commands
- **Built-in Commands**: Provide built-in commands such as `cd`, `exit`, `jobs`, `fg`, and `bg`

### Non-Functional Requirements

- **Performance**: The shell should be responsive with minimal latency in command execution
- **Code Quality**: Codebase should be modular and well-documented to facilitate maintenance and extensibility
- **Reliability**: Robust error handling and user-friendly error messages for invalid commands or syntax
- **Compatibility**: Compatible with Unix-like operating systems (Linux/macOS)
- **Resource Management**: Efficient memory management to avoid leaks or excessive resource consumption
- **Standards**: Use of modern C++ standards (C++11 or later) for safety and clarity

### Technical Stack

- **Programming Language**: C++ (C++11 or later)
- **System APIs**: POSIX system calls (fork, exec, pipe, dup2, wait, signal)
- **Build Tools**: CMake
- **Development Environment**: Unix-like OS (Linux/macOS)
- **Version Control**: Git

### Milestones/Timeline

- **Week 1**: Research and design shell architecture; set up development environment
- **Week 2**: Implement basic command parsing and execution of external commands
- **Week 3**: Add support for piping and I/O redirection
- **Week 4**: Implement background job management and signal handling
- **Week 5**: Add built-in commands and command history support
- **Week 6**: Testing, debugging, and documentation
- **Week 7**: Final refinements, performance optimization, and project packaging

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./build/hsh
```

## Deliverables

- **Fully functional custom shell executable** supporting all specified features
- **Source code repository** with clear commit history and version control
- **Comprehensive README** with build instructions, usage guide, and feature descriptions
- **Test cases** demonstrating functionality and edge case handling
- **Documentation** covering design decisions, architecture, and future improvement ideas

## Requirements

- C++11/17 compatible compiler
- CMake 3.10+
- Unix-like system (Linux, macOS)

## Implementation Notes

This implementation focuses on core shell functionality following Unix principles:
- Emphasis on correctness, performance, and memory safety
- Educational value in understanding Unix process model, IPC, and job control
- Modular architecture for maintainability and extensibility
