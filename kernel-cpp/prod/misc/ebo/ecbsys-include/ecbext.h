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

#ifdef ECB_CPLUS
#define EXT	extern "C"
#else
#define EXT extern                   
#endif

#ifdef WIN32
#define DLL __declspec( dllimport )
#else
#define DLL
#endif

