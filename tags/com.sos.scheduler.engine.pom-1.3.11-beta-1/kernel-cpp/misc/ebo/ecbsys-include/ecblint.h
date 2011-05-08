
#ifdef EXT
#undef EXT
#endif
#ifdef DLL
#undef DLL
#endif
#ifdef THLOCAL
#undef THLOCAL
#endif

#define EXT
#define DLL

#ifdef WIN32
#define THLOCAL 
#else
#define THLOCAL
#endif

