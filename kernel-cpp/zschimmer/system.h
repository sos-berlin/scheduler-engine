// zschimmer.h                                      ©2000 Joacim Zschimmer
// $Id$

#ifndef __ZSCHIMMER_SYSTEM_H
#define __ZSCHIMMER_SYSTEM_H

//---------------------------------------------------------------------------------PATCH FUER HP-UX
#ifdef __hpux
// Joacim Zschimmer, 23.9.03: Kopie aus sys/types.h einfuegt. sys/types.h selbst will sbsize_t nicht deklarieren.
#    include <sys/types.h>
//#  define _LABEL_T
#    if defined(_APP32_64BIT_OFF_T) || defined(_KERNEL)
        typedef int64_t  sbsize_t;      /* signed length in bytes */
        typedef uint64_t bsize_t;       /* unsigned length in bytes */
#    else
        typedef long  sbsize_t;
        typedef unsigned long bsize_t;
#    endif

//#  ifndef _BSIZE64_T
//#    define _BSIZE64_T
#    if !defined(__STDC_32_MODE__)
        typedef int64_t  sbsize64_t;
        typedef uint64_t bsize64_t;
#    endif
//#  endif /* _BSIZE64_T */
// Joacim Zschimmer, 23.9.03
#endif
//-------------------------------------------------------------------------------------------------

#if defined _MSC_VER

#define _CRT_NONSTDC_NO_DEPRECATE   // open() (statt _open()) etc. soll nicht bemängelt werden.

#   pragma warning( disable:4010 )   // warning C4010: single-line comment contains line-continuation character
#   pragma warning( disable:4065 )   // warning C4065: switch statement contains 'default' but no 'case' labels
#   pragma warning( disable:4097 )   // warning C4097: typedef-name 'Base_class' used as synonym for class-name 'Field_descr::Sos_self_deleting'
//# pragma warning( 3      :4100 )   // Unreferenzierter formaler Parameter (--> zschimmer.h)
#   pragma warning( disable:4127 )   // warning C4127: Bedingter Ausdruck ist konstant.   while(1)
#   pragma warning( error  :4150 )   // warning C4150: deletion of pointer to incomplete type 'Soscopy_dialog'; no destructor called
#   pragma warning( error  :4172 )   // warning C4172: Adresse einer lokalen Variablen oder eines temporären Werts wird zurückgegeben
#   pragma warning( error  :4190 )   // warning C4190: 'sos_new_ptr' has C-linkage specified, but returns UDT 'Sos_ptr<struct Hostapi>' which is incompatible with C.  SOS_NEW() liefert gelöschtes Objekt!
#   pragma warning( disable:4237 )   // warning C4237: nonstandard extension used : 'false' keyword is reserved for future use
#   pragma warning( disable:4355 )   // warning C4355: 'this' : used in base member initializer list
#   pragma warning( error  :4307 )   // warning C4307: '*' : integral constant overflow
#   pragma warning( 3      :4505 )   // Nichtreferenzierte lokale Funktion wurde entfernt
//# pragma warning( disable:4510 )   // warning C4510: 'Store_msg' : default constructor could not be generated
#   pragma warning( disable:4511 )   // warning C4511: Kopierkonstruktor konnte nicht generiert werden
#   pragma warning( disable:4512 )   // warning C4512: Zuweisungsoperator konnte nicht generiert werden
#   pragma warning( disable:4514 )   // warning C4514: 'logic_error::logic_error' : unreferenced inline function has been removed
#   pragma warning( disable:4702 )   // warning C4702: Unerreichbarer Code
#   pragma warning( error  :4706 )   // warning C4706: Zuweisung in bedingtem Ausdruck
#   pragma warning( disable:4711 )   // warning C4711: function 'ff' selected for automatic inline expansion
#   pragma warning( error  :4715 )   // errorg  C4715: Nicht alle Steuerelementpfade geben einen Wert zurück
#   pragma warning( disable:4786 )   // Bezeichner wurde auf '255' Zeichen in den Debug-Informationen reduziert
#   pragma warning( disable:4675 )   // warning C4675: 'size_t zschimmer::com::hash_value(const zschimmer::com::Bstr &)': aufgelöste Überladung wurde mit argumentbezogener Suche gefunden
#   pragma warning( error  :4717 )   // warning C4717: Rekursiv für alle Steuerelementpfade. Die Funktion verursacht einen Stapelüberlauf zur Laufzeit.
#   pragma warning( error  :4800 )   // warning C4800: Variable wird auf booleschen Wert ('True' oder 'False') gesetzt (Auswirkungen auf Leistungsverhalten moeglich)

#endif

//-------------------------------------------------------------------------------------------------

#if defined __hpux  &&  !defined __IA64__
#   include <bits/c++config.h>
#   undef _GLIBCXX_HAVE_STDINT_H        // gcc 4.1.2 und HP11.11 PA-RISC kennen nicht stdint.h
#endif

//-------------------------------------------------------------------------------------------------

#include <limits.h>
#include <string>

#include <time.h>

#ifdef __GNUC__ 
#   include <sys/types.h>               // uint etc.
#   include <typeinfo>                  // Für typeid
#endif

//-----------------------------------------------------------------------------Systemanhängigkeiten

#if defined _MSC_VER

#   define Z_MICROSOFT
#   define Z_MICROSOFT_ONLY( statement )  statement
#   define Z_WINDOWS
#   define Z_WINDOWS_ONLY( statement )  statement
#   define Z_WIN32
#   define Z_NORETURN                   __declspec( noreturn ) 
#   define Z_DEPRECATED                 __declspec( deprecated )
#   define __declspec_uuid(UUID)        __declspec( uuid(UUID) )

#   if _MSC_VER >= 1600 
#       define Z_HAS_MOVE_CONSTRUCTOR
#   endif

#elif defined __GNUC__

#   define __GNUC_VERSION__             ( __GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__ )       // 3.2.1 => 30201

#   define Z_UNIX
#   define Z_GNU_ONLY( statement )      statement
#   define Z_NORETURN                   __attribute__(( noreturn ))           
#   if __GNUC_VERSION_ >= 30300
#       define Z_DEPRECATED             __attribute__(( deprecated ))   // Ab gcc 3.3
#   else
#       define Z_DEPRECATED 
#   endif
//# define Z_PURE                       __attribute__(( pure ))         // Echt funktional, liefert bei gleichen Argumenten dasselbe Ergebnis, darf globale Variablen lesen
//# define Z_FUNCTIONAL                 __attribute__(( pure ))         // Echt funktional, liefert bei gleichen Argumenten dasselbe Ergebnis, liest nur Argumente, völlig unabhängig von globalen Variablen
//# define Z_GNU_PRINT( FORMAT, ... )   do { fprintf( stderr, "%s: ", Z_FUNCTION  ); fprintf( stderr, FORMAT, __VA_ARGS__ ); } while(0)
#   define Z_INIT_FIRST                 __attribute__((init_priority(101)));

#   if __GNUC_VERSION__ >= 40500  // gcc 4.5.0
//#       define Z_HAS_MOVE_CONSTRUCTOR
#   endif	

#endif

#if defined linux
#   define Z_LINUX
#endif

#if defined __SVR4 && defined __sun
#   define Z_SOLARIS
#endif

#ifdef __hpux
#   define Z_HPUX

#   ifdef __IA64__
#       define Z_HPUX_IA64
#    else
#       define Z_HPUX_PARISC
#   endif
#endif

#ifdef _AIX
#    define Z_AIX
#endif

#if !defined Z_MICROSOFT_ONLY
#   define Z_MICROSOFT_ONLY( statement )  
#endif

#if !defined Z_WINDOWS_ONLY
#   define Z_WINDOWS_ONLY( statement )  
#endif

#if !defined Z_BORLAND_ONLY
#   define Z_BORLAND_ONLY( statement )  
#endif

#if !defined Z_GNU_ONLY
#   define Z_GNU_ONLY( statement )  
#endif

#if !defined Z_GNU_PRINT
//#   define Z_GNU_PRINT( FORMAT, ... )  
#endif

#if !defined Z_NORETURN
#   define Z_NORETURN 
#endif

#if !defined Z_DEPRECATED
#   define Z_DEPRECATED 
#endif

#if !defined Z_INIT_FIRST
#   define Z_INIT_FIRST
#endif

#if !defined __declspec_uuid
#   define __declspec_uuid(UUID)
#endif

#if defined Z_WINDOWS
#   define Z_HAS_THREADS
//#   define __PRETTY_FUNCTION__  Z_FUNCTION
#endif

#define __pretty_function__ __PRETTY_FUNCTION__

#if defined Z_HPUX || defined Z_AIX
#    define EDEADLOCK EDEADLK
#endif
//-------------------------------------------------------------------------------------Z_DEBUG_ONLY

#if defined _DEBUG
#   define Z_DEBUG
#   define Z_DEBUG_ONLY( statement )  statement
# else
#   define Z_DEBUG_ONLY( statement )
#endif

//-----------------------------------------------------------------------------------Z_NDEBUG_DEBUG

#if defined _DEBUG
#   define Z_NDEBUG_DEBUG( ndebug, debug ) debug
# else
#   define Z_NDEBUG_DEBUG( ndebug, debug ) ndebug
#endif

//-----------------------------------------------------------------------------------Z_WINDOWS_ELSE

#if defined Z_WINDOWS
#   define Z_WINDOWS_ELSE( WINDOWS, NOT_WINDOWS ) WINDOWS
# else
#   define Z_WINDOWS_ELSE( WINDOWS, NOT_WINDOWS ) NOT_WINDOWS
#endif

//----------------------------------------------------------------------------------_vsnprintf etc.

#ifdef Z_WINDOWS

namespace zschimmer 
{
    struct tm* localtime_r( const time_t* timep, struct tm* result );       // in zschimmer.cxx
    struct tm* gmtime_r( const time_t* timep, struct tm* result );          // in zschimmer.cxx
}

#else
#   define _vsnprintf   vsnprintf
#endif

//----------------------------------------------------------------------------------------exception

#if defined __GNUC__ || defined __BORLANDC__
#   include <exception>
#endif

using std::exception;


#if defined __GNUC__
#   if !defined stricmp && !defined strcasecmp
#       include <strings.h>
        inline int stricmp( const char* s1, const char* s2 ) { return strcasecmp( s1, s2 ); }
        inline int strnicmp( const char* s1, const char* s2, size_t n ) { return strncasecmp( s1, s2, n ); }
#   endif
#endif

//----------------------------------------------------------------------------Z_NL Z_PATH_SEPARATOR

#ifdef Z_WINDOWS
#   define Z_NL             "\r\n"
#   define Z_PATH_SEPARATOR ";"
#   define Z_DIR_SEPARATOR  "\\"
# else
#   define Z_NL             "\n"
#   define Z_PATH_SEPARATOR ":"
#   define Z_DIR_SEPARATOR  "/"
#endif

//-----------------------------------------------------------------------------------------__assume

#ifndef Z_WINDOWS
#   define __assume(X)
#endif

//-----------------------------------------------------------------------------------------snprintf
#ifdef Z_WINDOWS

#   define snprintf _snprintf

#endif
//-----------------------------------------------------------------------------------Z_PRINTF_INT64

#if defined __GNUC__
#   define Z_PRINTF_INT64  "lli"
#   define Z_PRINTF_UINT64 "llu"
#else
#   define Z_PRINTF_INT64  "I64i"
#   define Z_PRINTF_UINT64 "I64u"
#endif

//----------------------------------------------------------------------------------------uint etc.

namespace zschimmer {

#ifdef Z_WINDOWS
    typedef unsigned int        Thread_id;
# else
    typedef pthread_t           Thread_id;
#endif

    
#ifdef __GNUC__                 // Gnu: in /usr/include/sys/types.h bereits definiert
    using ::uint;
#else
    typedef unsigned int        uint;
#endif

typedef unsigned char           Byte;



typedef short int               int16;
typedef unsigned short int      uint16;

#ifndef INT16_MAX
#   define INT16_MAX            ((int16)0x7FFF)
#endif

#ifndef UINT16_MAX
#   define UINT16_MAX           ((uint16)0xFFFFu)
#endif


typedef int                     int32;
typedef uint                    uint32;

#ifndef INT32_MAX
#   define INT32_MAX            ((int32)0x7FFFFFFF) 
#endif

#ifndef UINT32_MAX
#   define UINT32_MAX           ((uint32)0xFFFFFFFFu)
#endif


typedef unsigned long           ulong;


#if defined __GNUC__

typedef int32                   long32;                     // Windows long hat 32bit, Unix long hat 64 bit.
typedef uint32                  ulong32;

#   define Z_I64(N)  N##LL
#   define Z_UI64(N) N##uLL
    typedef          long long int int64;
    typedef          long long int __int64;
    typedef unsigned long long int uint64;
#   if !defined INT64_MAX
#       define  INT64_MAX    0x7fffffffffffffffLL
#       define UINT64_MAX    0xffffffffffffffffuLL
#   endif

#else //fined __STEM_MICROSOFT

typedef long                    long32;                     // Windows long hat 32bit, Unix long hat 64 bit.
typedef unsigned long           ulong32;

#   define I64(N)  N##i64
#   define UI64(N) N##ui64
    typedef          __int64  int64;
    typedef unsigned __int64 uint64;
#   define  INT64_MAX     _I64_MAX
#   define UINT64_MAX    _UI64_MAX

#endif

/*
typedef unsigned short int      uint16;

#ifndef INT16_MAX
#   define INT16_MAX            ((int16)0x7FFF)
#endif

#ifndef UINT16_MAX
#   define UINT16_MAX           ((uint16)0xFFFFu)
#endif

typedef int                     int32;
typedef uint                    uint32;

#ifndef INT32_MAX
#   define INT32_MAX            ((int32)0x7FFFFFFF)
#endif

#ifndef UINT32_MAX
#   define UINT32_MAX           ((uint32)0xFFFFFFFFu)
#endif
*/
//-----------------------------------------------------------------------------------string::string
// gcc 3.2 löst bei string( NULL, 0 )  exception aus. Das wollen wir nicht.

#if defined __GNUC__ && __GNUC_VERSION_ < 30202  // Bis gcc 3.2.1:
    inline std::string          make_string                 ( const char* s, int len )              { return std::string( s? s : "", len ); }
#else
    inline std::string          make_string                 ( const char* s, int len )              { return std::string( s, len ); }
#endif

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

//-----------------------------------------------------------------------------InterlockedIncrement
#ifdef Z_UNIX

zschimmer::long32               InterlockedIncrement        ( zschimmer::long32* );      // In z_posix.cxx
zschimmer::long32               InterlockedDecrement        ( zschimmer::long32* );      // In z_posix.cxx

typedef int                     errno_t;

//---------------------------------------------------------------------------------------strerror_s

errno_t                         strerror_s                  ( char* buffer, size_t buffer_size, errno_t );      // In z_posix.cxx

//-------------------------------------------------------------------------------------------------
#ifdef Z_SOLARIS
    extern "C" char**               environ;
#endif


#endif
//-------------------------------------------------------------------------------------------------
#ifdef Z_WINDOWS

namespace zschimmer
{
    const void*                 z_memrchr                   ( const void*, char, size_t );
}

inline const void*              memrchr                     ( const void* s, char c, size_t l )     { return zschimmer::z_memrchr( s, c, l ); }

#endif
//-------------------------------------------------------------------------------------------------

#endif
