#define CATCH_CONFIG_RUNNER
#include <test_helper.h>

int main (int argc, char ** argv)
{
    #ifndef USE_TEST_DEVICE_A
    std::cerr << "Warning: Tests involving Test Device A will be skipped," << std::endl;
    std::cerr << "  so a lot of the library's features will not be tested." << std::endl;
    #endif

    #ifndef USE_TEST_DEVICE_B
    std::cerr << "Warning: Tests involving Test Device B will be skipped," << std::endl;
    std::cerr << "  so bugs related to non-composite devices might be missed." << std::endl;
    #endif

    #ifdef NDEBUG
    std::cerr << "Warning: skipping unit tests because this is not a debug build.\n";
    #endif

    // If the last argument is "-p", then pause after the tests are run.
    // This allows us to run "leaks" on Mac OS X to check for memory leaks.
    bool pause_after_test = false;
    if (argc && std::string(argv[argc - 1]) == "-p")
    {
        pause_after_test = true;
        argc--;
    }

    int result = Catch::Session().run(argc, argv);

    if (pause_after_test)
    {
        printf("Press enter to continue.");
        std::string s;
        std::cin >> s;
    }

    return result;
}
