#include "shell.h"
#include <iostream>
#include <csignal>

int main(int argc, char* argv[]) {
    // Set up signal handlers (basic for now - will be expanded later)
    // For now, just ignore SIGINT/SIGTSTP at the main level
    // The shell will handle these internally for foreground processes

    try {
        hshell::Shell shell;
        shell.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
