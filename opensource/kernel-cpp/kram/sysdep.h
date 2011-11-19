#ifndef __SYSDEP_H
#define __SYSDEP_H

// sysdep.h
// Systemabhängige Definitionen
//                                                      (c) 1995 SOS GmbH Berlin
//                                                      Joacim Zschimmer

#include "../zschimmer/system.h"
using zschimmer::make_string;

#include <limits.h>

#if defined _MSC_VER
#   include <stdio.h>            // Wird von afx.h eingezogen. Besser hier damit sysdep.h stdin etc. neutralisieren kann
#   include <stdexcpt.h>         // xmsg bzw. exception
#   pragma include_alias( "strstream.h", "strstrea.h" )
#endif

//-----------------------------------------------------------------------------------SYSTEM_xxx

#if defined __GNUC__
#   define SYSTEM_GNU
#endif


#if defined __SVR4 && defined __sun
#   define SYSTEM_SOLARIS
#endif

#if defined __hpux
#   define SYSTEM_HPUX
#endif

#if defined __BORLANDC__


#   undef SYSTEM_BORLAND
#   if defined _WCHAR_T
#      define SYSTEM_BORLAND 0x500
#      define SYSTEM_BORLAND_5_00
#    else
#      define SYSTEM_BORLAND 0x453
#      define SYSTEM_BORLAND_4_53
#   endif

#endif

#if defined _MSC_VER
#   define SYSTEM_MICROSOFT     // Visual C++
#endif
// _Windows && __DLL__ von BORLAND abhg. !!!


#if defined _Windows  ||  defined _MSC_VER

#    define SYSTEM_WIN

#    if !defined STRICT
#       define STRICT
#    endif

#    if !defined __FLAT__
#       define SYSTEM_I286    // Damit der Emulator von Windows NT auf RM (MIPS) unterstützt wird
#    endif

#    if defined _WIN32
#       define __FLAT__
#       define __WIN32__
#    endif

#    if defined _DLL                    // Nur Multithreaded DLL?
#       define __DLL__                  // ??
#    endif

#    if !defined SYSTEM_WIN32
#        if defined __FLAT__
#            define SYSTEM_WIN32
#         else
#            define SYSTEM_WIN16
#        endif
#    endif

#    if defined(__DLL__)
#        define SYSTEM_WINDLL           // Wird auch von außen gesetzt
#        if defined SYSTEM_WIN16
#            define SYSTEM_WIN16DLL
#        endif
#    endif

#    define SYSTEM_EXCEPTIONS
#    define SYSTEM_NOTALIGNED

#   if defined SYSTEM_WIN16
#       undef  cin
#       define cin    CIN_IN_WINDOWS_GESPERRT
#       undef  cout
#       define cout   COUT_IN_WINDOWS_GESPERRT
#       undef  cerr
#       define cerr   CERR_IN_WINDOWS_GESPERRT
#       undef  clog
#       define clog   CLOG_IN_WINDOWS_GESPERRT
#       undef  stdin
#       define stdin  STDIN_IN_WINDOWS_GESPERRT
#       undef  stdout
#       define stdout STDOUT_IN_WINDOWS_GESPERRT
#       undef  stderr
#       define stderr STDERR_IN_WINDOWS_GESPERRT
#       undef  stdlog
#       define stdlog STDLOG_IN_WINDOWS_GESPERRT
#   endif

#elif defined __BORLANDC__

#    define SYSTEM_DOS
#    define SYSTEM_EXCEPTIONS

#else

#    define SYSTEM_UNIX

#    if ( defined __GNUC__ || defined SYSTEM_GNU ) && !defined SYSTEM_SOLARIS && !defined SYSTEM_HPUX
#        if !defined Z_AIX
#            define SYSTEM_LINUX
#        endif
#        if !defined SYSTEM_GNU
#            define SYSTEM_GNU
#        endif
#        define SYSTEM_EXCEPTIONS
#     else
#        if !defined SYSTEM_SOLARIS && !defined SYSTEM_HPUX
#            define SYSTEM_SOLARIS
#        endif
#        define SYSTEM_EXCEPTIONS
#    endif

#endif

//------------------------------------------------------------------------------warnings on/off
#if defined SYSTEM_BORLAND
#   pragma option -w-cln             // constant is long: Aus
#   pragma option -w-amp             // Superfluous & with function: Aus
#endif

#if defined SYSTEM_MICROSOFT
    //Beispiel: pragma warning( disable : 4507 34; once : 4385; error : 164 )
#   pragma warning( error  :4002 )   // warning C4002: too many actual parameters for macro 'strcmpi'
#   pragma warning( disable:4010 )   // warning C4010: single-line comment contains line-continuation character
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#   pragma warning( disable:4065 )   // warning C4065: switch statement contains 'default' but no 'case' labels
#   pragma warning( disable:4097 )   // warning C4097: typedef-name 'Base_class' used as synonym for class-name 'Field_descr::Sos_self_deleting'
//# pragma warning( disable:4099 )   // warning C4099: 'Sos_factory_agent' : type name first seen using 'class' now seen using 'struct'
#   pragma warning( error  :4150 )   // warning C4150: deletion of pointer to incomplete type 'Soscopy_dialog'; no destructor called
#   pragma warning( error  :4190 )   // warning C4190: 'sos_new_ptr' has C-linkage specified, but returns UDT 'Sos_ptr<struct Hostapi>' which is incompatible with C.  SOS_NEW() liefert gelöschtes Objekt!
#   pragma warning( disable:4237 )   // warning C4237: nonstandard extension used : 'false' keyword is reserved for future use
#   pragma warning( disable:4355 )   // warning C4355: 'this' : used in base member initializer list
#   pragma warning( error  :4307 )   // warning C4307: '*' : integral constant overflow
#   pragma warning( disable:4510 )   // warning C4510: 'Store_msg' : default constructor could not be generated
#   pragma warning( disable:4514 )   // warning C4514: 'logic_error::logic_error' : unreferenced inline function has been removed
//  pragma warning( disable:4610 )   // warning C4610: struct 'Store_msg' can never be instantiated - user defined constructor required
#   pragma warning( disable:4711 )   // warning C4711: function 'ff' selected for automatic inline expansion
#   pragma warning( disable:4786 )   // Bezeichner wurde auf '255' Zeichen in den Debug-Informationen reduziert
#   pragma warning( error  :4800 )   // warning C4800: Variable wird auf booleschen Wert ('True' oder 'False') gesetzt (Auswirkungen auf Leistungsverhalten moeglich)

// Visual Studio 2005:
#   pragma warning( disable:4996 )   // warning C4996: 'open' was declared deprecated
#   define _CRT_SECURE_NO_DEPRECATE  // This function or variable may be unsafe. Consider using _snprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_DEPRECATE. See online help for details.
#endif
//----------------------------------------------------------------------------------SYSTEM_I386
#if defined __WIN32__
#   undef SYSTEM_I286
#endif

#if defined __i386__
#   define SYSTEM_I386
#endif

#if !defined SYSTEM_I286
#   if defined SYSTEM_DOS  ||  defined SYSTEM_WIN
#       define SYSTEM_I386
#   endif
#endif

#if defined SYSTEM_BORLAND
#   if defined SYSTEM_I286
#       pragma option -2
#   elif defined SYSTEM_I386
//#       pragma option -3
#   endif
#endif
//-------------------------------------------------------------------------------SYSTEM_ALIGNED
#if !defined SYSTEM_ALIGNED  &&  !defined SYSTEM_NOTALIGNED
#   if defined SYSTEM_I386
#       define SYSTEM_NOTALIGNED
#    else
#       define SYSTEM_ALIGNED
#   endif
#endif
//-----------------------------------------------------------------------------------------NULL
#if !defined SYSTEM_SOLARIS && !defined NULL
//#   define NULL ((void *)0)
#define NULL 0
#endif
//----------------------------------------------------------------------------------SYSTEM_BOOL
// C++ kennt Datentyp bool?
#if defined SYSTEM_GNU
#   define SYSTEM_BOOL
#endif

#if defined __BORLANDC__
#   if SYSTEM_BORLAND >= 0x500     // BC++ 5.0
#       define SYSTEM_BOOL
#   endif
#endif

#if defined _MSC_VER
#   if _MSC_VER >= 1100  // MS VC++ 5.0?
#       define SYSTEM_BOOL
#	 else
#       define true 1
#       define false 0
#       undef false
#       undef true
#   endif
#endif

//-----------------------------------------------------------------------------------------Bool

#if defined SYSTEM_BOOL
    typedef bool Bool;
#else
    typedef int Bool;
    const Bool false = 0;
    const Bool true  = 1;
#endif

typedef Bool Sos_bool;  // Für Herrn Püschel

//-----------------------------------------------------------------------------------SYSTEM_MFC
#if defined _MSC_VER
//#   define SYSTEM_MFC
#endif

#if defined SYSTEM_MFC
#   include <afx.h>              // MFC muss vor <windows.h> eingezogen sein.
#endif
//--------------------------------------------------------------------SYSTEM_DELETE_WITH_PARAMS
#if defined SYSTEM_MICROSOFT
#   define SYSTEM_DELETE_WITH_PARAMS        /* operator delete() mit gleichen Parametern wie new() deklarieren (neu MSVC++6) */
                                            // Siehe Microsoft warning C4291
#endif
//---------------------------------------------------------------------SYSTEM_INCLUDE_TEMPLATES
#if defined SYSTEM_BORLAND  ||  defined SYSTEM_MICROSOFT ||  defined SYSTEM_GNU
#    define SYSTEM_INCLUDE_TEMPLATES                        // templates in .h einziehen
#endif
//---------------------------------------------------------------------------NON_INLINE_TEMPLATE

#if defined SYSTEM_SOLARIS
#    define NON_INLINE_TEMPLATE inline
# else
#    define NON_INLINE_TEMPLATE
#endif

//----------------------------------------------------------------------------------SYSTEM_RTTI
//#if defined __BORLANDC__
//#   define SYSTEM_RTTI                                     // Run time type information
//#endif
//#if defined SYSTEM_MICROSOFT  &&  defined _CPPRTTI
//#   define SYSTEM_RTTI
//#endif
#define SYSTEM_RTTI
//----------------------------------------------------------------------------------SYSTEM_ODBC
#if defined SYSTEM_WIN || defined SYSTEM_LINUX //|| defined SYSTEM_SOLARIS
#    define SYSTEM_ODBC
#endif
//------------------------------------------------------------------------------SYSTEM_STARVIEW
#if defined SYSTEM_WIN16  &&  !defined SYSTEM_WINDLL  ||  defined SYSTEM_SOLARIS
//#    define SYSTEM_STARVIEW
#endif
//----------------------------------------------------------------------------------SYSTEM_ODBC
#if defined SYSTEM_WIN
#    define SYSTEM_ODBC
#endif
//------------------------------------------------------------------------------------SOS_CLASS
#if defined SYSTEM_WIN && defined SYSTEM_STARVIEW
//Nur für sosole: (führt zu großen export-Listen in der .exe)
//#    define SOS_CLASS _export         // um gemeinsam mit StarView vererbbar zu sein
#   define SOS_CLASS
# else
#    define SOS_CLASS
#endif
//------------------------------------------------------------------------------------SOS_CONST
#if defined SYSTEM_WIN16
#    define SOS_CONST const //__far   // far-Datensegment als CONST deklarieren!
# else
#    define SOS_CONST const
#endif
//-----------------------------------------------------------------------------------NL_IS_CRLF
#if defined( SYSTEM_DOS )  ||  defined( SYSTEM_WIN ) || defined( SYSTEM_WINDLL )
#   define NL_IS_CRLF 1
#   define SYSTEM_NL "\r\n"
# else
#   define NL_IS_CRLF 0
#   define SYSTEM_NL "\n"
#endif
//----------------------------------------------------------MOST_SIGNIFICANT_BYTE_IS_RIGHT/LEFT
#define MOST_SIGNIFICANT_BYTE_IS_RIGHT
//#define MOST_SIGNIFICANT_BYTE_IS_LEFT
//--------------------------------------------------------------------------------near far etc.

#if defined SYSTEM_WIN16
#    define _near  near
#    define _far   far
#    define _huge  huge
# else
#    define _ss
#    define _cdecl
#    define _huge
#    define __huge
#    define _far
#    define __far
#    if !defined SYSTEM_MICROSOFT
#       define FAR
#    endif
#    define _FAR
#    define _near
#    define __near
#    define _export
#endif

//---------------------------------------------------------------------------------------_Cdecl/__cdecl
#if !defined __BORLANDC__ && !defined _Cdecl
#   define _Cdecl
#endif

#if !defined SYSTEM_WIN  &&  !defined __cdecl
#   define __cdecl
#endif
//-----------------------------------------------------------------------------strcmpi strncmpi
#if defined SYSTEM_UNIX
#    define strcmpi(  s1, s2 )     strcasecmp( s1, s2 )
#    define strncmpi( s1, s2, l )  strncasecmp( s1, s2, l )
#elif defined SYSTEM_MICROSOFT
#    define strcmpi(  s1, s2 )     stricmp( s1, s2 )
#    define strncmpi( s1, s2, l )  strnicmp( s1, s2, l )
#endif
//------------------------------------------------------------------------------------_MAX_PATH
#if defined SYSTEM_UNIX
#   define _MAX_PATH    PATH_MAX    // in limits.h    // FILENAME_MAX
#elif defined SYSTEM_MICROSOFT
#   define PATH_MAX _MAX_PATH
#   define MAXPATH  _MAX_PATH
#endif
//------------------------------------------------------------------------------throw try catch
#if defined SYSTEM_EXCEPTIONS

#    define throw_again  throw

# else

    namespace sos
    {
    // Bei throw Programm-Abbruch mit Fehlermeldung (s. sysdep.cxx)

    struct Xc;

    struct Exception_aborts
    {
        Exception_aborts( const char* source_filename, int lineno )
        :
            _filename ( source_filename ),
            _lineno   ( lineno          )
        {
        }

        void operator= ( const void* );
        void operator= ( const Xc*   );

      private:
        const char* _filename;
        const int   _lineno;
    };

    //extern const char* __try_source_filename;   // try-Blöcke müßten gestapelt werden
    //extern const char* __try_lineno;

    } //namespace sos

#    define throw           Exception_aborts( __FILE__, __LINE__) = &
#    define throw_again
#    define try           //if( __try_source_filename = __FILE__, __try_lineno = __LINE__, 1 )
#    define catch( X )      if( 0 )

#endif

//----------------------------------------------------------------------------ZERO_RETURN_VALUE
// Borland 4.5 ruft bei einer Exception den Destruktur für ein Element auf, das durch
// einen Funktionsaufruf erst initialisiert werden soll.
// Dieses Makro setzt das Funktionsergebnis auf binär 0.
// Verwendung:
// TYPE function (....)       // kann auch eine Methode sein
// {
//     ZERO_RETURN_VALUE( TYPE );
//     ...
// }

#if defined __BORLANDC__ && defined SYSTEM_WIN16
#   define ZERO_RETURN_VALUE( TYPE )                \
    {                                               \
        uint size = sizeof (TYPE);                  \
        __asm  push es;                             \
        __asm  push di;                             \
        __asm  push ax;                             \
        __asm  push cx;                             \
        __asm  les di, ss:[bp+6];                   \
        __asm  mov al, 0;                           \
        __asm  mov cx, size;                        \
        __asm  rep stosb;                           \
        __asm  pop cx;                              \
        __asm  pop ax;                              \
        __asm  pop di;                              \
        __asm  pop es;                              \
    }                                               \
    while( 0 )  return *(TYPE*)0;   /*check*/

    void zero_return_value( unsigned int size );
#else
#   define ZERO_RETURN_VALUE( TYPE )
    //inline void zero_return_value( unsigned int size ) {}
#endif

//-------------------------------------------------------------------BORLAND_STATIC_ELSE_INLINE

#if defined __BORLANDC__
#   define BORLAND_STATIC_ELSE_INLINE static
# else
#   define BORLAND_STATIC_ELSE_INLINE inline
#endif

//-----------------------------------------------------------------------------------uint32 etc

namespace sos
{

typedef unsigned int    Uint;
typedef unsigned int    uint;
typedef unsigned char   Uchar;
typedef unsigned char   uchar;
typedef unsigned char   Uint1;
typedef unsigned char   uint1;
typedef   signed char    Int1;
typedef   signed char    int1;
typedef unsigned short  Uint16;
typedef unsigned short  Uint2;      //alt
typedef unsigned short  uint16;
typedef unsigned short  uint2;      //alt
typedef          short   Int16;
typedef          short   Int2;      //alt
typedef          short   int16;
typedef          short   int2;      //alt
//typedef          int     Word;
//typedef          int     word;
typedef unsigned long   ulong;
typedef unsigned long   Uint32;
typedef unsigned long   Uint4;      //alt
typedef unsigned long   uint32;
typedef unsigned long   uint4;      //alt
typedef          long    Int32;
typedef          long    Int4;      //alt
typedef          long    int32;
typedef          long    int4;      //alt
typedef          long   Dword;
typedef          long   dword;
typedef unsigned char    Byte;
typedef unsigned char    byte;
typedef unsigned char    Bit;
typedef unsigned char    bit;

} //namespace sos

//---------------------------------------------------------------------------type_MIN, type_MAX

#define SHORT_MAX  SHRT_MAX
#define USHORT_MAX USHRT_MAX

#if !defined INT32_MAX
#   define INT32_MAX  ((int32)0x7FFFFFFF)
#   define UINT32_MAX ((uint32)0xFFFFFFFFu)
#   define  INT4_MAX   LONG_MAX
#   define UINT4_MAX  ULONG_MAX
#   define INT16_MAX  ((int16)0x7FFF)
#   define UINT16_MAX ((uint16)0xFFFFu)
#   define INT2_MAX   INT16_MAX
#   define UINT2_MAX  USHRT_MAX
#endif

#if defined SYSTEM_MICROSOFT && _MSC_VER < 1500
#   define MAXINT INT_MAX
#endif

//-------------------------------------------------------------------------------------PATH_MAX
#if defined SYSTEM_WIN16
#   define PATH_MAX 260
#endif
//----------------------------------------------------------------------------------------int64
namespace sos
{

#if defined SYSTEM_GNU

#   define SYSTEM_INT64
#   define I64(N) N##LL
#   define UI64(N) N##uLL
    typedef          long long int int64;
    typedef          long long int __int64;
    typedef unsigned long long int uint64;

#   if !defined INT_MAX
#       define  INT_MAX      0x7fffffff
#       define UINT_MAX      0xffffffffu
#   endif

#   if !defined INT64_MAX
#       define  INT64_MAX    0x7fffffffffffffffLL
#       define UINT64_MAX    0xffffffffffffffffuLL
#   endif

#elif defined SYSTEM_SOLARIS

#   define SYSTEM_INT64
#   define I64(N) N##LL
#   define UI64(N) N##uLL
    typedef          long long int int64;
    typedef          long long int __int64;
    typedef unsigned long long int uint64;

#elif defined SYSTEM_MICROSOFT

#   define SYSTEM_INT64
#   define I64(N) N##i64
#   define UI64(N) N##ui64
    typedef          __int64  int64;
    typedef unsigned __int64 uint64;
#   define  INT64_MAX     _I64_MAX
#   define UINT64_MAX    _UI64_MAX

#endif

} //namespace sos
//---------------------------------------------------------------------------------------Big_int
namespace sos
{

#if defined SYSTEM_INT64
    typedef  int64   Big_int;
    typedef uint64  Ubig_int;
#   define SYSTEM_BIG_INT
#   define  BIG_INT_MAX   INT64_MAX
#   define UBIG_INT_MAX UINT64_MAX
# else
    typedef  int32    Big_int;
    typedef uint32   Ubig_int;
#   define BIG_INT_MAX  INT32_MAX
#   define UBIG_INT_MAX UINT32_MAX
#endif

} //namespace sos
//---------------------------------------------------------------------------SYSTEM_LONG_DOUBLE

#if defined __BORLANDC__
#   define SYSTEM_LONG_DOUBLE
#endif

/*
#if 0
// Template funktioniert nicht:
template <class I>
inline Byte* significant_byte_ptr (I *i, unsigned int byte_no) {
    assert (byte_no < sizeof *i);
    #if MOST_SIGNIFICANT_BYTE_IS_LEFT
        return ((Byte*) i + sizeof *i - 1 - byte_no);
     #else
        return ((Byte*) i + byte_no);
    #endif
}
#endif


#if 0           // Und hier wird falscher Code generiert:
inline Byte* significant_byte_ptr (uint2 *i, unsigned int byte_no) {
    assert (byte_no < sizeof *i);
    #if MOST_SIGNIFICANT_BYTE_IS_LEFT
        return ((Byte*) i + sizeof *i - 1 - byte_no);
     #else
        return ((Byte*) i + byte_no);
    #endif
}

inline Byte* significant_byte_ptr (uint4 *i, unsigned int byte_no) {
    assert (byte_no < sizeof *i);
    #if MOST_SIGNIFICANT_BYTE_IS_LEFT
        return ((Byte*) i + sizeof *i - 1 - byte_no);
     #else
        return ((Byte*) i + byte_no);
    #endif
}
#endif
*/
//-------------------------------------------------------------------------significant_byte_ptr

#if MOST_SIGNIFICANT_BYTE_IS_LEFT
#   define significant_byte_ptr(iptr,byte_no) \
                        ((Byte*) iptr + sizeof *iptr - 1 - byte_no)
#else
#   define significant_byte_ptr(iptr,byte_no) ((Byte*) iptr + byte_no)
#endif

//---------------------------------------------------------------------------------------printf
#if defined SYSTEM_UNIX
#   define PRINTF_LONG_LONG "ll"
#else
#   define PRINTF_LONG_LONG "I64"
#endif
//--------------------------------------------------------------------------STRING_TOOLS_HPP ??

#if defined( SYSTEM_SOLARIS )
#   define STRING_TOOLS_HPP
#endif

//-------------------------------------------------------------------------IOS_BINARY, O_BINARY

#if defined SYSTEM_WIN
#   define IOS_BINARY ios::binary
# else
#   define IOS_BINARY ((ios::openmode)0) 
#   define O_BINARY   0
#endif

//--------------------------------------------------------------------S_IREAD_all, S_IWRITE_all

#ifdef SYSTEM_UNIX
#   define S_IREAD_all  ( S_IRUSR | S_IRGRP | S_IROTH )     // umask maskiert wieder aus
#   define S_IWRITE_all ( S_IWUSR | S_IWGRP | S_IWOTH )     // umask maskiert wieder aus
# else
#   define S_IREAD_all  S_IREAD
#   define S_IWRITE_all S_IWRITE
#endif

//-------------------------------------------------------------------------------------iostream
#ifdef SYSTEM_GNU
//?#   define _G_IO_THROW
#   include <typeinfo>
#endif
//-----------------------------------------------------------------------------MS-Windows-Typen

#if defined SYSTEM_WIN16
    #define SOS_DECLARE_MSWIN_HANDLE(name)    struct name##__; \
                                              typedef const struct name##__ _near* name;

    #define SOS_DECLARE_MSWIN_HANDLE32(name)  struct name##__; \
                                              typedef const struct name##__ _far* name;
#endif

#if defined SYSTEM_WIN32
    #define SOS_DECLARE_MSWIN_HANDLE(name)    struct name##__; \
                                              typedef struct name##__* name;
#endif



#ifdef SYSTEM_WIN
#   include <winsock2.h>
#   include <windows.h>         // FILETIME und SYSTEMTIME
#endif


//----------------------------------------------------------Vorwärtsdeklarationen iostream etc.

#include <string>

// In den Quellen keine solchen Includes mehr:
#include <iostream>
#include <iomanip>
#include <fstream>

#if defined SYSTEM_GNU && !defined SYSTEM_SOLARIS      // gcc 3.2
#   include <sstream>
# else
#   include <strstream>
#endif

namespace sos
{
    //using namespace std;  // Das ist (bei Microsoft VSC++ 6) nicht auf namespace sos beschränkt.
    using ::std::string;
    using ::std::iostream;
    using ::std::istream;
    using ::std::ostream;
    using ::std::hex;
    using ::std::dec;
    using ::std::setw;
    using ::std::endl;
    using ::std::fstream;
    using ::std::ifstream;
    using ::std::ofstream;
}


#if defined SYSTEM_MICROSOFT
    std::ostream& operator << ( std::ostream&, const __int64& );
    std::ostream& operator << ( std::ostream&, const unsigned __int64& );
#endif

#if defined __BORLANDC__
#   if !defined(___DEFS_H)
#      include <_defs.h>
#   endif
    class _EXPCLASS xmsg;
    class _EXPCLASS string;
    class _export String;       // StarView
#   if defined SYSTEM_STARVIEW  &&  !defined SYSTEM_SOLARIS
        typedef String Sos_string;
#    else
        typedef string Sos_string;
#   endif

# else

#   include <string>
#   include "sysxcept.h"

#endif

//-------------------------------------------------------------------------------------__assume

#ifndef SYSTEM_WIN
#   define __assume(X)
#endif

//----------------------------------------------------------------------------System-Funktionen

namespace sos
{

void _check_pointer( const void* ptr, uint length = 1, const char* info = "" );
void _check_string0_pointer( const char*, const char* info = "" );


#if defined CHECK_POINTER
    
    inline void check_pointer( const void* ptr, uint length = 1, const char* info = "" )
    {
        _check_pointer( ptr, length, info );
    }

    inline void check_string0_pointer( const char* ptr, const char* info = "" )
    {
        _check_string0_pointer( ptr, info );
    }

    inline void* checked_pointer( void* ptr, const char* info )
    {
        check_pointer( ptr, 1, info );
        return ptr;
    }

    inline const void* checked_pointer( const void* ptr, const char* info )
    {
        check_pointer( ptr, 1, info );
        return ptr;
    }

    inline void* checked_pointer( void* ptr, uint length, const char* info )
    {
        check_pointer( ptr, length, info );
        return ptr;
    }


    inline const void* checked_pointer( const void* ptr, uint length, const char* info )
    {
        check_pointer( ptr, length, info );
        return ptr;
    }

#else

    inline void check_pointer( const void*, uint = 1, const char* = "" )
    {
    }

    inline void check_string0_pointer( const char*, const char* = "" )
    {
    }

    inline void* checked_pointer( void* ptr, const char* )
    {
        return ptr;
    }

    inline const void* checked_pointer( const void* ptr, const char* )
    {
        return ptr;
    }

    inline void* checked_pointer( void* ptr, uint, const char* )
    {
        return ptr;
    }


    inline const void* checked_pointer( const void* ptr, uint, const char* )
    {
        return ptr;
    }
#endif


template<class TYPE>
inline TYPE* checked_pointer( TYPE* ptr, const char* info = "" )
{
#   if defined CHECK_POINTER
        check_pointer( ptr, sizeof *ptr, info );
#   endif

    return ptr;
}





template<class TYPE>
inline TYPE* checked_pointer( TYPE* ptr, uint length, const char* info = "" )
{
#   if defined CHECK_POINTER
        check_pointer( ptr, length, info );
#   endif

    return ptr;
}



//---------------------------------------------------------------------------------------------

//string                          get_temp_path           ();
int                             sos_mkstemp             ( const string& name = "sos" );
double                          get_cpu_time            ();
string                          directory_of_path       ( const string& path );
string                          filename_of_path        ( const string& path );
string                          basename_of_path        ( const string& path );
string                          extension_of_path       ( const string& path );
string                          file_version_info       ( const string& filename );
string                          module_filename         ();
string                          program_filename        ();
string                          make_absolute_filename  ( const string& dir, const string& filename );
void                            print_all_modules       ( ostream* );
inline ostream&                 operator <<             ( ostream& s, const exception& x )          { s << x.what();  return s; }

#ifdef SYSTEM_WIN
    string                      filename_of_hinstance   ( HINSTANCE hinstance );
    string                      version_info_detail     ( void* info, const char* field, bool with_field_name = false );
#endif

//---------------------------------------------------------------------------------------------

} //namespace sos

//---------------------------------------------------------------------------------------------

#endif
