#include <iostream> // Provides std::cout for test suite status output, std::cerr for exception messages, and std::cin implicitly via getline.
#include <exception> // Provides std::exception base class for catching runtime test failures via catch (const std::exception& e).

// Forward declarations for test functions
void run_tokenizer_tests();

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "Helix Shell (hsh) Test Suite\n";
    std::cout << "==========================\n\n";

    try {
        run_tokenizer_tests();

        std::cout << "\n🎉 All tests passed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception\n";
        return 1;
    }
}
