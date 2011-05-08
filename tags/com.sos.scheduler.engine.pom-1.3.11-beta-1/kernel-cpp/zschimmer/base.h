// $Id$        © 2000 Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_BASE_H
#define __ZSCHIMMER_BASE_H

//------------------------------------------------------------------------------------------------

#define Z_SOS                               // Für die SOS GmbH

#ifdef Z_SOS
//#   define Z_ERROR_PREFIX "SOS"             // Fehlercode beginnen mit SOS- (statt Z-) 
#   define Z_TEMP_FILE_ID "sos"             // Namensbestandteil von temporären Dateinamen
#else
#   define Z_TEMP_FILE_ID "zsc"
#endif

#define Z_ERROR_PREFIX "Z"

//--------------------------------------------------------------------------------------------WIN32
#ifdef _WIN32
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT  0x0500     // Ab Windows 2000
#   endif

#   ifndef _WIN32_DCOM
#       define _WIN32_DCOM              // Für CoCreateInstanceEx()
#   endif
#endif

class _com_error;

//----------------------------------------------------------------------------------------------GNU
#ifdef __GNUC__

    namespace __gnu_cxx {}

    namespace stdext        
    {
        using namespace __gnu_cxx;  // gcc 3.3.2 kennt stdext nicht (hierin sollte hash_map sein), dafür __gnu_cxx
    }

#endif
//-----------------------------------------------------------------------------------Z_OS_NAMESPACE
//#ifdef Z_WINDOWS
//    #define Z_OS_NAMESPACE ::zschimmer::windows
//#else
//    #define Z_OS_NAMESPACE ::zschimmer::posix
//#endif
//-----------------------------------------------------------------------------------------#include

#include "system.h"

#include <stdlib.h>

//#if defined _DEBUG  &&  defined __GNUC__
//#   include <mcheck.h>           // Memory leak detection
//#endif

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <set>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>         // min, max etc.

#ifdef __GNUC__
#   include <ext/hash_map>
#   include <ext/hash_set>
//#   include <ext/stl_hash_fun.h>   Nicht einziehen!
#else
#   include <hash_map>
#   include <hash_set>
#endif

#ifdef Z_WINDOWS
#   include <winsock2.h>        // timeval
#endif

#if defined __GNUC__ && !defined Z_SOLARIS
#   include <fstream>
//#   include <strstream>
//  typedef std::basic_string< wchar_t, char_traits<wchar_t>, allocator<wchar_t> >   wstring;
# else
#   include <ios>
#endif

//---------------------------------------------------------------------------------hash_map<> etc.
#ifdef __GNUC__

#   define Z_DEFINE_GNU_HASH_VALUE( NAMESPACE, CLASS )                                              \
                                                                                                    \
        namespace __gnu_cxx                                                                         \
        {                                                                                           \
            template<>                                                                              \
            struct hash< NAMESPACE::CLASS >                                                         \
            {                                                                                       \
                size_t operator()( const NAMESPACE::CLASS& object ) const                           \
                {                                                                                   \
                    return NAMESPACE::hash_value( object );                                         \
                }                                                                                   \
            };                                                                                      \
        }


    namespace __gnu_cxx 
    {
        template<>
        struct hash< std::string >
        {
            size_t              operator()                  ( const std::string& str ) const        { return hash<const char*>()( str.c_str() ); }
        };

        template< typename TYPE >
        struct hash< TYPE* >
        {
            size_t              operator()                  ( const TYPE* p ) const                 { return (size_t)p; }
        };
    }

/*
    namespace __gnu_cxx 
    {
        template< class CLASS >
        struct hash
        {
            size_t              operator()                  ( const CLASS& object ) const           { return hash_value( object ); }
        };
    }
*/
#else

#   define Z_DEFINE_GNU_HASH_VALUE( NAMESPACE, CLASS )

#endif
//------------------------------------------------------------------------------------DYNAMIC_CAST

#ifdef __GNUC__                               // gcc 2.3.2 bis 3.3.1: dynamic_cast<> stürzt ab. 
#   define DYNAMIC_CAST_CRASHES
#endif

#ifdef DYNAMIC_CAST_CRASHES
#   define DYNAMIC_CAST( TYPE, POINTER )   ( static_cast< TYPE >( POINTER ) )      // Nur verwenden, wenn sich der Zeiger wirklich casten lässt!
# else
#   define DYNAMIC_CAST( TYPE, POINTER )   ( dynamic_cast< TYPE >( POINTER ) )
#endif

//------------------------------------------------------------------------------------------Z_INIT

#define Z_INIT( NAME )                                                                        \
    static struct Z_init_##NAME                                                               \
    {                                                                                         \
        Z_init_##NAME ();                                                                     \
    }                                                                                         \
    _z_init_##NAME##_;                                                                        \
                                                                                              \
    Z_init_##NAME::Z_init_##NAME()

// Verwendung:
// Z_INIT( beispiel )
// {
//     var = 4711;
// }

//--------------------------------------------------------------------------------------------NO_OF

#if !defined __SOS_H  ||  !defined NO_OF
#   define NO_OF( array )  ( sizeof (array) / sizeof (array)[0] )
#endif

//-----------------------------------------------------------------Z_DEFINE_BITWISE_ENUM_OPERATIONS

#define Z_DEFINE_BITWISE_ENUM_OPERATIONS( ENUM )                                                    \
    inline ENUM  operator|  ( ENUM  a, ENUM b )  { return (ENUM)( (int)a | (int)b ); }              \
    inline ENUM  operator&  ( ENUM  a, ENUM b )  { return (ENUM)( (int)a & (int)b ); }              \
    inline ENUM  operator~  ( ENUM  a         )  { return (ENUM)~(int)a; }                          \
    inline ENUM& operator|= ( ENUM& a, ENUM b )  { return a = a | b; }                              \
    inline ENUM& operator&= ( ENUM& a, ENUM b )  { return a = a & b; }                              

//-----------------------------------------------------------------------------------------SIZE_MAX

#ifndef SIZE_MAX               // Gnu definert's, Microsoft nicht
#   define SIZE_MAX UINT_MAX
#endif

//------------------------------------------------------------------------------namespace zschimmer

namespace zschimmer
{

//---------------------------------------------------------------------------------system_interface
#ifdef Z_WINDOWS
    namespace windows {}
    namespace system_interface = windows;
#else
    namespace posix {}
    namespace system_interface = posix;
#endif
//--------------------------------------------------------------------------------------------using

using std::string;
using std::istream;
using std::ostream;

//-------------------------------------------------------------------------------------------------

namespace io
{
    struct Char_sequence;
    struct Byte_sequence;
}

//------------------------------------------------------------------------------pid, Process_handle
#ifdef Z_WINDOWS
    typedef int                 pid_t;
    typedef HANDLE              Process_handle;
    typedef HANDLE              Process_group_handle;   // Job object
#else
    typedef int                 Process_handle;         // Process group id (Pid des ersten Prozesses)
    typedef int                 Process_group_handle;
#endif
//---------------------------------------------------------------------------------name_of_typeinfo
#ifdef __GNUC__
    std::string                 name_of_type_info_gcc   ( const std::type_info& );
    inline string               name_of_type_info       ( const std::type_info& ti )                     { return name_of_type_info_gcc( ti ); }
#else
    inline std::string          name_of_type_info       ( const std::type_info& ti )                     { return ti.name(); }
#endif

//-------------------------------------------------------------------------------------name_of_type

template< class TYPE >
inline string                   name_of_type            ( const TYPE& object )                           { return name_of_type_info( typeid( object ) ); }

//-------------------------------------------------------------------------------name_of_type_debug

template< class TYPE >
inline string                   name_of_type_debug      ( const TYPE& object )
{ 
#   ifdef __GNUC__
        return "(no type)";     // Gnu C++ kennt bad_typeid nicht.
#   else
        try
        {
            return name_of_type( object ); 
        }
        catch( const bad_typeid& x )
        {
            return string("bad_typeid: ") + x.what();
        }
#   endif
}

//--------------------------------------------------------------------------------------O_NOINHERIT
#ifdef Z_UNIX
#   define O_NOINHERIT  0
#endif
//-------------------------------------------------------------------------------------------last()

template< typename TYPE >
inline TYPE* last( std::vector<TYPE>& container )
{
    return &*( container.end() - 1 );
}


template< typename TYPE >
inline TYPE* last( std::list<TYPE>& container )
{
    typename std::list<TYPE>::iterator it = container.end();
    --it;
    TYPE& result = *it;
    return &result;
}


template< typename TYPE >
inline TYPE* last( std::stack<TYPE>& container )
{
    return &container.top();
}


template< typename TYPE >
inline typename TYPE::mapped_type* get_container_element_or_null( const TYPE& container, const typename TYPE::key_type key )
{
    typename TYPE::iterator it = container.find( key );
    return it == container.end()? NULL : &*it;
}


template< typename TYPE >
inline const typename TYPE::mapped_type& get_container_element( const TYPE& container, const typename TYPE::key_type key, const typename TYPE::mapped_type& deflt )
{
    typename TYPE::iterator it = container.find( key );
    return it == container.end()? deflt :   *it;
}

//---------------------------------------------------------------------------------find_map_entry()

template< typename KEY, typename CONTENT >
inline const CONTENT* find_map_entry( const ::stdext::hash_map<KEY,CONTENT>& map, const KEY& key )
{
    typename stdext::hash_map<KEY,CONTENT>::const_iterator it = map.find( key );
    return it == map.end()? NULL : &it->second;
}


template< typename KEY, typename CONTENT >
inline CONTENT* find_map_entry( ::stdext::hash_map<KEY,CONTENT>& map, const KEY& key )
{
    typename stdext::hash_map<KEY,CONTENT>::iterator it = map.find( key );
    return it == map.end()? NULL : &it->second;
}

//---------------------------------------------------------------------------------------Z_FUNCTION

#ifdef __GNUC__
#   define Z__PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#else
#   define Z__PRETTY_FUNCTION__ __FUNCTION__
#endif

#define Z_FUNCTION  ::zschimmer::z_function( Z__PRETTY_FUNCTION__ )

//---------------------------------------------------------------------------------------Z_FOR_EACH

#define Z_FOR_EACH( TYPE, CONTAINER, ITERATOR )  \
    for( TYPE::iterator ITERATOR = (CONTAINER).begin(); ITERATOR != (CONTAINER).end(); ITERATOR++ )

//---------------------------------------------------------------------------------Z_FOR_EACH_CONST

#define Z_FOR_EACH_CONST( TYPE, CONTAINER, ITERATOR )  \
    for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); ITERATOR != (CONTAINER).end(); ITERATOR++ )

//-------------------------------------------------------------------------------Z_FOR_EACH_REVERSE

#define Z_FOR_EACH_REVERSE( TYPE, CONTAINER, ITERATOR )  \
    for( TYPE::reverse_iterator ITERATOR = (CONTAINER).rbegin(); ITERATOR != (CONTAINER).rend(); ITERATOR++ )

//-------------------------------------------------------------------------Z_FOR_EACH_REVERSE_CONST

#define Z_FOR_EACH_REVERSE_CONST( TYPE, CONTAINER, ITERATOR )  \
    for( TYPE::const_reverse_iterator ITERATOR = (CONTAINER).rbegin(); ITERATOR != (CONTAINER).rend(); ITERATOR++ )

//-------------------------------------------------------------------------------------------Z_EACH

//#define Z_EACH( TYPE, LIST, ITERATOR ) 
//    if( TYPE* __each_array__[] = LIST, true ) 
//        for( TYPE** __iterator__ = __each_array__; __iterator__ < __each_array__ + NO_OF(__each_array__); __iterator__++ ) 
//            if( TYPE* ITERATOR = *__iterator )

//--------------------------------------------------------------------------------------Z_ASSERT_XC

#define Z_ASSERT_XC( STATEMENT, ERROR_CODE )                                                        \
    if( log_category_is_set( "self_test.exception" ) )                                              \
    {                                                                                               \
        try                                                                                         \
        {                                                                                           \
            STATEMENT;                                                                              \
            assert( ( #STATEMENT, !"Exception " ERROR_CODE " expected" ) );                         \
        }                                                                                           \
        catch( ::zschimmer::Xc& x )                                                                 \
        {                                                                                           \
            assert( ( #STATEMENT, x.code() == ERROR_CODE ) );                                       \
        }                                                                                           \
    }

#define Z_ASSERT_ANY_XC( STATEMENT )                                                                \
    if( log_category_is_set( "self_test.exception" ) )                                              \
    {                                                                                               \
        try                                                                                         \
        {                                                                                           \
            STATEMENT;                                                                              \
            assert( ( #STATEMENT, !"Exception expected" ) );                                        \
        }                                                                                           \
        catch( ::zschimmer::Xc& )                                                                   \
        {                                                                                           \
        }                                                                                           \
    }

//----------------------------------------------------------------------------------------Fill_zero

struct Fill_end
{
};

struct Fill_zero
{
    Fill_zero( void* ende )
    {
        memset( this+1, '\0', (uint)( (Byte*)ende - (Byte*)( this+1 ) ) );
    }

    Fill_zero( const Fill_end& ende )
    {
        memset( this + 1, 0, (char*)&ende - (char*)( this+1 ) );
    }
};

//--------------------------------------------------------------------------------------------string

string                      as_string                   ( int64 );
int                         hex_to_digit                ( char );
string                      lcase_hex                   ( const void* p, size_t len );
inline string               lcase_hex_from_string       ( const string& s )                         { return lcase_hex( s.data(), s.length() ); }
string                      string_from_hex             ( const io::Char_sequence& );
string                      printf_string               ( const char* pattern, long );
string                      string_printf               ( const char* pattern, double );
int                         z_snprintf                  ( char* buffer, uint buffer_size, const char* format, ... );
string                      z_strerror                  ( int errn );
inline string               hex_from_int                ( int i )                                   { return printf_string( "%08x", i ); }
inline string               hex_from_int16              ( int16 i )                                 { return printf_string( "%04x", i ); }
inline char                 char_from_wchar             ( wchar_t c )                               { return (char)( c & 0xFF ); }  // Hier könnte eine Prüfung einfügt werden.
//inline char                 char_from_wchar             ( int c )                                   { return char_from_wchar( (wchar_t)c ); }
string                      z_function                  ( const char* pretty_function );

inline const char*          c_str                       ( const string& str )       { return str.c_str(); }
inline size_t               length                      ( const char* str )         { return strlen( str ); }
inline size_t               length                      ( const string& str )       { return str.length(); }

size_t                      length_without_trailing_char( const char* text, size_t len, char c );
inline size_t               length_without_trailing_spaces( const char* text, size_t len )          { return length_without_trailing_char( text, len, ' ' ); }
inline size_t               length_without_trailing_spaces( const char* text )                      { return length_without_trailing_char( text, strlen(text), ' ' ); }
inline size_t               length_without_trailing_spaces( const string& str )                     { return length_without_trailing_char( str.c_str(), str.length(), ' ' ); }

void                        lcase_                      ( string* );
inline string               lcase                       ( const string& str )                       { string s = str; lcase_(&s);  return s; }

void                        ucase_                      ( string* );
inline string               ucase                       ( const string& str )                       { string s = str; ucase_(&s);  return s; }

string                      ltrim                       ( const string& );

string                      rtrim                       ( const char* str, size_t length );
inline string               rtrim                       ( const char* str )                         { return rtrim( str, strlen(str) ); }
inline string               rtrim                       ( const string& str )                       { return str.length() == 0 || !isspace( (unsigned char)*str.rbegin() )? str : rtrim( str.data(), str.length() ); }

inline string               trim                        ( const string& str )                       { return ltrim( rtrim( str ) ); }

string                      quoted_string               ( const char* text, char, char, size_t len );
inline string               quoted_string               ( const char* text, char quote1 = '"', char quote2 = '\\' )  { return quoted_string( text, quote1, quote2, strlen(text) ); }
inline string               quoted_string               ( const string& str, char quote1 = '"', char quote2 = '\\' ) { return quoted_string( str.data(), quote1, quote2, str.length() ); }
//string                      quoted_command_parameter    ( const string& );                              // Nur, wenn ein Blank drin ist
string                      quoted_windows_process_parameter( const string& );                          // Nur, wenn ein Blank drin ist
string                      quoted_unix_command_parameter   ( const string& );                          // Nur, wenn ein Blank drin ist

inline bool                 string_equals_prefix_then_skip( const char** str_ptr, const char* prefix )  { size_t len = strlen(prefix); bool equal = strncmp( *str_ptr, prefix, len ) == 0; if(equal) (*str_ptr) += len;  return equal; }

inline bool                 string_begins_with          ( const char* str, const char* prefix )         { size_t len = strlen(prefix); return strncmp( str, prefix, len ) == 0; }
inline bool                 string_begins_with          ( const string& str, const char* prefix )       { size_t len = strlen(prefix); return strncmp( str.c_str(), prefix, len ) == 0; }
inline bool                 string_begins_with          ( const string& str, const string& prefix )     { return strncmp( str.c_str(), prefix.c_str(), prefix.length() ) == 0; }

//inline bool                 string_ends_with            ( const char* str, const char* suffix )         { size_t len = strlen(suffix); return strlen(str) >= len && strcmp( str, suffix, len ) == 0; }
inline bool                 string_ends_with            ( const string& str, const char* suffix )       { size_t len = strlen(suffix); return str.length() >= len && memcmp( &str.end()[0-len], suffix, len ) == 0; }
inline bool                 string_ends_with            ( const string& str, const string& suffix )     { return str.length() >= suffix.length() && memcmp( &str.end()[0-suffix.length()], suffix.data(), suffix.length() ) == 0; }

string                      remove_password             ( const string& text, const char* replacement = NULL );  // NULL: Jede Spur verwischen, "?" => -password=?
string                      truncate_with_ellipsis      ( const io::Char_sequence&, size_t length, const string& ellipse = "..." );
string                      truncate_to_one_line_with_ellipsis( const io::Char_sequence&, size_t length, const string& ellipse = "..." );

#ifdef Z_DEBUG
    void                    test_truncate_to_ellipsis   ();
#endif

//------------------------------------------------------------------------------------replace_regex
// in regex_class.cxx

string                      replace_regex               ( const string& str, const string& regex, const string& neu, int limit = INT_MAX );
string                      replace_regex_ref           ( const string& str, const string& regex, const string& replacement_with_refs, int limit = INT_MAX );  // (...) -> \1 usw.
string                      replace_regex_ext           ( const string& str, const string& regex, const string& replacement_with_refs, int limit = INT_MAX );  // (...) -> \\1 usw., \\u ist ucase() etc.
string                      find_regex                  ( const string& str, const string& regex );
bool                        has_regex                   ( const string& str, const string& regex );

//-------------------------------------------------------------------------------------vector_split
// in regex_class.cxx

inline std::vector<string>  vector_split                ( const string& regex_str, const string& str, int limit = 0 );
void                        vector_split                ( const string& regex_str, const string& str, int limit, std::vector<string>* result );

inline std::vector<string> vector_split( const string& regex_str, const string& str, int limit )
{
    std::vector<string> result;
    vector_split( regex_str, str, limit, &result );
    return result;
}

//--------------------------------------------------------------------------------------------split
// in regex_class.cxx

inline int split( const string& regex_str, const string& str, string* result1, string* result2 )
{
    std::vector<string> result;

    vector_split( regex_str, str, 2, &result );
    *result1 = result.size() >= 1? result[0] : "";
    *result2 = result.size() >= 2? result[1] : "";

    return (int)result.size();
}


inline int split( const string& regex_str, const string& str, string* result1, string* result2, string* result3 )
{
    std::vector<string> result;

    vector_split( regex_str, str, 3, &result );
    *result1 = result.size() >= 1? result[0] : "";
    *result2 = result.size() >= 2? result[1] : "";
    *result3 = result.size() >= 3? result[2] : "";

    return (int)result.size();
}

//----------------------------------------------------------------------------------------set_split
// in regex_class.cxx

inline std::set<string>     set_split                   ( const string& regex_str, const string& str );
void                        set_split                   ( std::set<string>* result, const string& regex_str, const string& str );

inline std::set<string> set_split( const string& regex_str, const string& str )
{
    std::set<string> result;
    set_split( &result, regex_str, str );
    return result;
}

//---------------------------------------------------------------------------------------------join

template< class CONTAINER_TYPE >
string join( const string& glue, const CONTAINER_TYPE& container )
{
    string result;

    if( !container.empty() )
    {
        int size = ( container.size() - 1 ) * glue.length();

        for( typename CONTAINER_TYPE::const_iterator it = container.begin(); it != container.end(); it++ )  size += it->length();

        result.reserve( size );

        typename CONTAINER_TYPE::const_iterator it = container.begin();
        if( it != container.end() )
        {
            result = *it++;
            while( it != container.end() )  result += glue,  result += *it++;
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------reduce

template< typename FUNCTION_TYPE, typename RESULT_TYPE, class CONTAINER_TYPE >
RESULT_TYPE reduce( FUNCTION_TYPE f, RESULT_TYPE start_value, const CONTAINER_TYPE& container )
{
    RESULT_TYPE x = start_value;
    for( typename CONTAINER_TYPE::const_reverse_iterator it = container.rbegin(); it != container.rend(); it++ )  x = f( *it, x );
    return x;
}

// Beispiel:  int count = reduce( reduce_count_not_null, 0, pointer_list )

//----------------------------------------------------------------------------reduce_count_non_null

inline int reduce_count_non_null( const void* pointer, int c )
{ 
    if( pointer != NULL )  c++;
    return c;
};

//---------------------------------------------------------------------------------------------zmap

/*
template< class CONTAINER_TYPE >
void zmap( void (*f)( CONTAINER_TYPE::value_type* ), CONTAINER_TYPE* container )
{
    for( CONTAINER_TYPE::iterator it = container->begin(); it != container->end(); it++ )
    {
        f( &*it );
    }
}


template< class CONTAINER_TYPE >
void zmap( CONTAINER_TYPE::value_type (*f)( const CONTAINER_TYPE::value_type& ), CONTAINER_TYPE* container )
{
    for( CONTAINER_TYPE::iterator it = container->begin(); it != container->end(); it++ )
    {
        *it = f( *it );
    }

    return result;
}
*/

/*
template< class MAPPED_ELEMENT_TYPE, class ELEMENT_TYPE >
std::vector<MAPPED_ELEMENT_TYPE> vector_map( MAPPED_ELEMENT_TYPE (*f)( const ELEMENT_TYPE& ), const std::vector<ELEMENT_TYPE>& source )
{
    std::vector<MAPPED_ELEMENT_TYPE> result ( source.size() );
    int i = 0;

    for( std::vector<ELEMENT_TYPE>::const_iterator it = source.begin(); it != source.end(); it++ )
    {
        result[i++] = f( *it );
    }

    return result;
}
*/
// Microsoft C++ sagt, das vorstehendes template mehrdeutig sei (string und const string&). Deshalb explizit:

inline
std::vector<string> vector_map( string (*f)( const string& ), const std::vector<string>& source )
{
    std::vector<string> result ( source.size() );
    int i = 0;

    for( std::vector<string>::const_iterator it = source.begin(); it != source.end(); it++ )
    {
        result[i++] = f( *it );
    }

    return result;
}


/*
template< class MAPPED_ELEMENT_TYPE, class ELEMENT_TYPE >
std::set<MAPPED_ELEMENT_TYPE> set_map( MAPPED_ELEMENT_TYPE (*map_function)( const ELEMENT_TYPE& ), const std::set<ELEMENT_TYPE>& container )
{
    std::set<MAPPED_ELEMENT_TYPE> result;

    for( std::set<ELEMENT_TYPE>::const_iterator it = container.begin(); it != container.end(); it++ )
    {
        result.insert( map_function( *it ) );
    }

    return result;
}
*/
// Microsoft C++ sagt, das vorstehendes template mehrdeutig sei (string und const string&). Deshalb explizit:

inline
std::set<string> set_map( string (*f)( const string& ), const std::set<string>& container )
{
    std::set<string> result;

    for( std::set<string>::const_iterator it = container.begin(); it != container.end(); it++ )
    {
        result.insert( f( *it ) );
    }

    return result;
}

//-------------------------------------------------------------------------------------set_includes

template< class T >
inline bool set_includes( const typename std::set<T>& s, const typename std::set<T>::value_type& value )
{
    return s.find( value ) != s.end();
}

//-------------------------------------------------------------------------------------set_includes

template< class T >
inline bool set_includes( const typename stdext::hash_set<T>& s, const typename stdext::hash_set<T>::value_type& value )
{
    return s.find( value ) != s.end();
}

//----------------------------------------------------------------------------------------is_subset

template< class T >
inline bool is_subset( const typename stdext::hash_set<T>& subset, const typename stdext::hash_set<T>& s )
{
    Z_FOR_EACH_CONST( const typename stdext::hash_set<T>, subset, subset_element )  
        if( !set_includes( s, subset_element ) )  return false;

    return true;
}

//--------------------------------------------------------------------------------------as_int etc.

int                         as_int                      ( const char* );
uint                        as_uint                     ( const char* );
int64                       as_int64                    ( const char* );
uint64                      as_uint64                   ( const char* );
inline int32                as_int32                    ( const char* s )           { return as_int(s); }
inline uint32               as_uint32                   ( const char* s )           { return as_uint(s); }
int16                       as_int16                    ( const char* );
uint16                      as_uint16                   ( const char* );
double                      as_double                   ( const char* );
bool                        as_bool                     ( const char* );

inline int                  as_int                      ( const string& str )       { return as_int   ( str.c_str() ); }
inline uint                 as_uint                     ( const string& str )       { return as_uint  ( str.c_str() ); }
inline int64                as_int64                    ( const string& str )       { return as_int64 ( str.c_str() ); }
inline uint64               as_uint64                   ( const string& str )       { return as_uint64( str.c_str() ); }
inline int32                as_int32                    ( const string& str )       { return as_int32 ( str.c_str() ); }
inline uint32               as_uint32                   ( const string& str )       { return as_uint32( str.c_str() ); }
inline int16                as_int16                    ( const string& str )       { return as_int16 ( str.c_str() ); }
inline uint16               as_uint16                   ( const string& str )       { return as_uint16( str.c_str() ); }
inline double               as_double                   ( const string& str )       { return as_double( str.c_str() ); }
inline bool                 as_bool                     ( const string& str )       { return as_bool  ( str.c_str() ); }

//-----------------------------------------------------------------------------------------min, max
#ifdef min
#   undef min
#endif

#ifdef max
#   undef max
#endif
//------------------------------------------------------------------------------------Non_cloneable
// Copy constructor abgeschaltet:

struct Non_cloneable
{
                            Non_cloneable               ()                                          {}

  private:
                            Non_cloneable               ( const Non_cloneable& );                   // Nicht implementiert
                            void operator =             ( const Non_cloneable& );                   // Nicht implementiert
};

//---------------------------------------------------------------------------------------Set_locale

struct Set_locale
{
                            Set_locale                  ( const char* locale )                      { set( LC_ALL, locale ); }
                            Set_locale                  ( int category, const char* locale )        { set( category, locale ); }
                           ~Set_locale                  ()                                          { restore(); }

                            operator bool               () const                                    { return _saved_locale != NULL; }
    void                    set                         ( int category, const char* locale );
    void                    restore                     ();

    int                    _category;
    const char*            _saved_locale;
};

//-----------------------------------------------------------------------------------------Z_LOCALE
#ifdef __GNUC__xxx

#   define Z_LOCALE( CATEGORY, LOCALE )                                                             \
        for( ::zschimmer::Set_locale __set_locale__ ( CATEGORY, LOCALE ) )

#else

#   define Z_LOCALE( CATEGORY, LOCALE )                                                             \
        for( ::zschimmer::Set_locale __set_locale__ ( CATEGORY, LOCALE ); __set_locale__; __set_locale__.close() )

#endif
//-------------------------------------------------------------------------------------Rotating_bar

struct Rotating_bar
{
                            Rotating_bar                ( bool ok = true );
                           ~Rotating_bar                ();

    void                    operator()                  () ;

    const static char       chars [4];
    bool                   _ok;
    int                    _i;
};

//--------------------------------------------------------------------------------------Log_context

struct Log_context  // Dieser struct wird zwischen DLLs ausgetausch. Also kein C++ oder sowas verwenden!
{
    // DIESE TYPEN NIEMALS NIE ÄNDERN, SIE WERDEN ZWISCHEN DLLS AUSGETAUSCHT (hostole.dll, spidermonkey.dll)
    // Und keine C++-Klassen verwenden, nur einfaches altes C.

#   ifdef Z_UNIX
        typedef pthread_mutex_t    Log_mutex;
#    else    
        typedef CRITICAL_SECTION   Log_mutex;                      
#   endif

    typedef int                 Log_write                   ( const char* text, int length );       // return errno;   log_write(NULL,1) ==> flush

    int                        _version;    
    Log_mutex*                 _mutex;                      // _version >= 1
    Log_write*                 _log_write;                  // _version >= 1
    int                        _indent_tls_index;           // _version >= 1
};

//---------------------------------------------------------------------Get_string_by_name_interface

struct Get_string_by_name_interface
{
    virtual                ~Get_string_by_name_interface()                                          {}
    virtual string          get_string_by_name          ( const string& name, bool* name_found ) const = 0;
};

//--------------------------------------------------------------------------------Message_code_text

struct Message_code_text
{
    const char*            _code;
    const char*            _text;
    Message_code_text*     _next;                       // Nur der erste in einem Array Message_code_text[] ist gefüllt
};

//-----------------------------------------------------------------------------------javabridge::Vm

namespace javabridge 
{
    struct Vm;  // Forward
}

//-------------------------------------------------------------------------------------------------

void                        zschimmer_init              ();
void                        zschimmer_terminate         ();
string                      subst_env                   ( const string&, const Get_string_by_name_interface* = NULL );   // Ersetzt $NAME

const static double         double_time_max             = INT_MAX;      // Könnte LONG_MAX sein, wenn time_t 64 Bits hat (wie in Microsoft C++ 2005)
const static time_t         time_max                    = INT_MAX;      

double                      double_from_localtime       ();
double                      double_from_gmtime          ();
time_t                      time_t_from_gmtime          ();
double                      localtime_from_gmtime       ( double );
time_t                      localtime_from_gmtime       ( time_t );
timeval                     timeval_from_seconds        ( double seconds );
time_t                      gmtime_from_localtime       ( time_t localtime );
double                      gmtime_from_localtime       ( double localtime );

string                      string_local_from_time_t    ( time_t );
string                      string_local_from_time_t    ( double );
string                      string_local_strftime       ( const string& format, time_t );
string                      string_local_strftime       ( const string& format, double );
string                      string_gmt_from_time_t      ( time_t );
string                      string_gmt_strftime         ( const string& format, time_t );
string                      string_strftime             ( const string& format, const struct tm& );

struct tm                   tm_from_time_t              ( time_t );
struct tm                   local_tm_from_time_t        ( time_t );

void                        sleep                       ( double seconds );
void                        set_environment_variable    ( const string& name, const string& value );
string                      get_environment_variable    ( const string& name );

string                      complete_computer_name      ();

bool                        log_category_is_set         ( const char* name );                       // log.cxx
bool                        log_category_is_set         ( const string& name );                     // log.cxx
void                    set_log_category                ( const string& name, bool value = true );
void                    set_log_category_default        ( const string& name, bool value = true );
void                    set_log_category_implicit       ( const string& name, bool value );
void                    set_log_category_explicit       ( const string& name );
string                      log_categories_as_string    ();

struct Message_code_text;
void                        add_message_code_texts      ( Message_code_text* );

//-------------------------------------------------------------------------------------------static

extern bool                 unloading_module;

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

//--------------------------------------------------------------------------------------namespace z
// Abkürzung für namespace zschimmer

namespace z 
{ 
    using namespace zschimmer; 
};

//-------------------------------------------------------------------------------------------------

#include "string_stream.h"
#include "message.h"

//-------------------------------------------------------------------------------------------------

#endif
