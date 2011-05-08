#ifdef EXT
#undef EXT
#endif
#ifdef DLL
#undef DLL
#endif
#ifdef THLOCAL
#undef THLOCAL
#endif

#define THLOCAL

#ifdef CPLUSEXT 
#define EXT	extern "C"
#else
#define EXT extern                   
#endif

#define DLL

