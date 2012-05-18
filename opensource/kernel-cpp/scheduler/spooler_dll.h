
#ifdef _WIN32
#    define SCHEDULER_EXPORT __declspec(dllexport)
#else
#    define SCHEDULER_EXPORT  // Siehe Makefile linker-export-symbols
#endif

extern "C" int SCHEDULER_EXPORT spooler_program(int argc, char** argv);
