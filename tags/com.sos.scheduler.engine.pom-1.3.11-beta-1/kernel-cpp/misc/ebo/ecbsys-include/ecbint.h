
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

#ifdef WIN32
#define DLL _declspec( dllexport ) 
/* #define THLOCAL __declspec( thread )	*/
#define THLOCAL
#else
#define DLL
#define THLOCAL
#endif /* WIN 32*/

