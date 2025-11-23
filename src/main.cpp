#include "shell.h" // Provides the Shell class for the main REPL loop, offering run() method to execute the shell, and exception handling for fatal errors.
#include <iostream> // Provides standard I/O stream objects: std::cout for output, std::cerr for error messages - used for fatal error reporting.
#include <csignal> // Provides signal handling functions like signal(), kill(), raise() - intended for setting up basic signal handlers (SIGINT, SIGTSTP) at the main level.

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Set up signal handlers (basic for now - will be expanded later)
    // For now, just ignore SIGINT/SIGTSTP at the main level
    // The shell will handle these internally for foreground processes

    try {
        hshell::Shell shell;
        return shell.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
