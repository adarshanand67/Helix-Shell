#include "shell.h"
#include <cstring>
#include <iostream>

static void usage() {
    std::cerr <<
        "Usage: helix [options] [script [args...]]\n"
        "  -c <cmd>       execute CMD and exit\n"
        "  -s             read commands from stdin (default when no args)\n"
        "  --version      print version and exit\n"
        "  --help         print this message\n";
}

int main(int argc, char* argv[]) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--version") == 0) {
                std::cout << "helix 1.0.0\n";
                return 0;
            }
            if (std::strcmp(argv[i], "--help") == 0) {
                usage();
                return 0;
            }
            if (std::strcmp(argv[i], "-c") == 0) {
                if (i + 1 >= argc) {
                    std::cerr << "helix: -c requires an argument\n";
                    return 2;
                }
                helix::Shell shell;
                return shell.runCommand(argv[i + 1]);
            }
            if (std::strcmp(argv[i], "-s") == 0) {
                helix::Shell shell;
                return shell.runStdin();
            }
            // Positional: treat as script file
            helix::Shell shell;
            return shell.runScript(argv[i], argc - i - 1, argv + i + 1);
        }

        // Interactive REPL
        helix::Shell shell;
        return shell.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
