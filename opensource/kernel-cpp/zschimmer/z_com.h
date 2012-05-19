// $Id: z_com.h 13404 2008-02-12 08:09:25Z jz $

// §1719

#ifndef __Z_COM_H
#define __Z_COM_H

#include "com.h"
#include "mutex.h"
#include <jni.h>


#ifdef Copy     // gcc 3.2
#   undef Copy
#endif

//-------------------------------------------------------------------------------------------------

inline bool                     operator <                  ( const CLSID& a, const CLSID& b )          { return memcmp( &a, &b, sizeof a ) < 0; }       // Für map<CLSID>
inline size_t                   hash_value                  ( const CLSID& clsid )                      { return clsid.Data1 ^ clsid.Data3 ^ clsid.Data4[7]; }
Z_DEFINE_GNU_HASH_VALUE( , CLSID )

//-------------------------------------------------------------------------------------------------

namespace zschimmer {

//struct Character_encoding;

namespace com {

//-------------------------------------------------------------------------------------------------

#define STANDARD_LCID  ( MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ) )

#define Z_CLSCTX_SEPARATE_PROCESS ( (CLSCTX)0x1000000 )     // Erweiterung für com_create_instance(). Der COM-Server läuft in einem eigenen, abhängigen Prozess

const int windows_codepage_iso_8859_1 = 28591;              // CP_UTF8 ist definiert, aber nicht diese codepage

//-------------------------------------------------------------------------------------------------

struct Bstr;

//-------------------------------------------------------------------------------------------------

Z_NORETURN void                 throw_com                   ( HRESULT, const string& function, const char* ins1 = NULL, const char* ins2 = NULL );  
Z_NORETURN void                 throw_com                   ( HRESULT, const string& function, const OLECHAR* ins1 );  
Z_NORETURN void                 throw_com                   ( HRESULT, const string& function, const Bstr& ins1 );
Z_NORETURN inline void          throw_com                   ( HRESULT hr, const string& function, const string& ins ) { throw_com( hr, function, ins.c_str() ); }
Z_NORETURN void                 throw_com_excepinfo         ( HRESULT hresult, EXCEPINFO*, const string& function, const char* ins );

// bereits in com_base.h deklariert:
//IUnknown*                       com_create_instance         ( const CLSID& clsid,     const IID&, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL );
//IUnknown*                       com_create_instance         ( const char* class_name, const IID&, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL );
inline IUnknown*                com_create_instance         ( const string& class_name, const IID& iid, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL ) { return com_create_instance( class_name.c_str(), iid, outer, context ); }

IUnknown*                       com_create_instance         ( const CLSID& clsid      , const IID&    , IUnknown* outer       , const std::string& module_filename );

string                          string_from_hresult         ( HRESULT );

ostream&                        operator <<                 ( std::ostream&, const GUID& );
string                          string_from_guid            ( const GUID& );
string                          name_of_guid_or_empty       ( const GUID& );
string                          name_of_clsid_or_empty      ( const CLSID& );
string                          name_of_iid_or_empty        ( const IID& );
string                          name_of_typelib_or_empty    ( const GUID& );

string                          name_of_dispid_or_empty     ( IDispatch*, DISPID );

void                            olechar_from_wchar          ( OLECHAR* o, const wchar_t* w, size_t len );  // (Unix, wchar_t != OLECHAR)

string                          string_from_ole             ( const OLECHAR* );
string                          string_from_ole             ( const OLECHAR*, size_t len );
string                          string_from_bstr            ( const BSTR );

string                          string_from_variant         ( const VARIANT&, LCID = STANDARD_LCID );
string                          string_from_variant_2       ( const VARIANT&, LCID = STANDARD_LCID );     // Für VariantChangeTypeEx()
string                          debug_string_from_variant   ( const VARIANT& );                     // Auf jeden Fall einen String liefern, auch wenn's keinen gibt.
bool                            bool_from_variant           ( const VARIANT& );
int                             int_from_variant            ( const VARIANT& );
int64                           int64_from_variant          ( const VARIANT& );
double                          double_from_variant         ( const VARIANT& );
bool                            variants_are_equal          ( const VARIANT&, const VARIANT& );
bool                            variant_is_lower            ( const VARIANT&, const VARIANT& );
inline ostream&                 operator <<                 ( std::ostream& s, const VARIANT& v )   { s << debug_string_from_variant( v );  return s; }
ostream&                        operator <<                 ( std::ostream& s, BSTR );

string                          vartype_name                ( VARTYPE vt );
inline string                   variant_type_name           ( const VARIANT& v )                    { return vartype_name( V_VT(&v) ); }
inline string                   variant_type_name           ( const VARIANT* v )                    { return vartype_name( V_VT(v) ); }

VARTYPE                         vartype_from_name           ( const string& name );
VARTYPE                         vartype_from_safearray      ( SAFEARRAY* );                         // -1 bei einem Fehler

#ifndef __GNUC__   // gcc 3.2 hat wstring nicht in der Bibliothek
#ifdef Z_OLECHAR_IS_WCHAR
    inline std::wstring         wstring_from_bstr           ( const BSTR bstr )                     { return std::wstring( bstr, SysStringLen(bstr) ); }
    BSTR                        bstr_from_wstring           ( const std::wstring& );
#else
    std::wstring                wstring_from_bstr           ( const BSTR );
    BSTR                        bstr_from_wstring           ( const std::wstring& );
#endif
inline BSTR                     bstr_from_string            ( const std::wstring& str )             { return bstr_from_wstring(str); }
#endif

BSTR                            bstr_from_string            ( const char* single_byte_text, size_t len );
inline BSTR                     bstr_from_string            ( const char* str )                     { return bstr_from_string( str, str? strlen(str) : 0 ); }
inline BSTR                     bstr_from_string            ( const string& str )                   { return bstr_from_string( str.data(), str.length() ); }

string                          string_from_clsid           ( const CLSID& );

int                             compare_olechar_with_char   ( const OLECHAR*, const char* );

bool                            olestring_begins_with       ( const OLECHAR* str, const char* prefix );
inline bool                     olestring_begins_with       ( const OLECHAR* str, const string& prefix ) { return olestring_begins_with( str, prefix.c_str() ); }

CLSID                           clsid_from_name             ( const char*   class_name );
inline CLSID                    clsid_from_name             ( const string& class_name )                { return clsid_from_name( class_name.c_str() ); }

DATE                            com_date_from_seconds_since_1970      ( double );
DATE                            com_date_from_local_seconds_since_1970( double );
DATE                            local_com_date_from_seconds_since_1970( double );

double                          seconds_since_1970_from_com_date      ( DATE date );
double                          seconds_since_1970_from_local_com_date( DATE date );

//-------------------------------------------------------------------------------HRESULT Funktionen

HRESULT                         Call_query_interface        ( IUnknown*, const IID&, void** );

HRESULT                         Com_set_error               ( const exception&  x, const string& method = "" );
HRESULT                         Com_set_error               ( const _com_error& x, const string& method = "" );
HRESULT                         Com_set_error               ( const char* descr  , const string& method = "" );

HRESULT                         String_to_bstr              ( const char*, size_t   , BSTR*      ) throw();
inline HRESULT                  String_to_bstr              ( const string& str     , BSTR* bstr ) throw()  { return String_to_bstr( str.data(), str.length(), bstr ); }
inline HRESULT                  String_to_bstr              ( const char* str       , BSTR* bstr ) throw()  { return String_to_bstr( str, str? strlen( str ) : 0, bstr ); }
//HRESULT                         String_to_bstr              ( const wchar_t*, BSTR* ) throw();
HRESULT                         String_to_bstr              ( const OLECHAR*        , BSTR*      ) throw();
HRESULT                         String_to_bstr              ( const Bstr&           , BSTR*      ) throw();
HRESULT                         String_to_bstr              ( const OLECHAR*, size_t, BSTR*      ) throw();

//HRESULT                         String_to_bstr              ( const Character_encoding&  , const char*, size_t, BSTR*      ) throw();
//inline HRESULT                  String_to_bstr              ( const Character_encoding& e, const string& str  , BSTR* bstr ) throw()  { return String_to_bstr( e, str.data(), str.length(), bstr ); }

HRESULT                         Bstr_to_bstr                ( const BSTR, BSTR* ) throw();

HRESULT                         Bstr_to_string              ( const BSTR, string* result ) throw();
HRESULT                         Olechar_to_string           ( const OLECHAR*r, int len, string* result ) throw();

HRESULT                         Variant_to_string           ( const VARIANT&, string* result, LCID = STANDARD_LCID );   // Keine Änderung bei is_missing()
HRESULT                         Variant_to_bool             ( const VARIANT&, bool* );                                  // Keine Änderung bei is_missing()

HRESULT                         Name_to_clsid               ( const OLECHAR* class_name, CLSID* result );
HRESULT                         Name_to_clsid               ( const char*    class_name, CLSID* result );
inline HRESULT                  Name_to_clsid               ( const string&  class_name, CLSID* result )      { return Name_to_clsid( class_name.c_str(), result ); }

HRESULT                         z_SafeArrayGetVartype       ( SAFEARRAY*, VARTYPE* result );

//--------------------------------------------------------------------------------------Com_context
// Für DllMain(Z_DLL_COM_ATTACH)// Der Aufbau muss stabil bleiben, er wird zwischen Hauptprogramm und nachgeladenen Modulen austauscht!
struct Com_context
{
    int                         version;
    int                         count;                                                              // Anzahl der Funktionen
    int                         (*get_version)          (int);                                      // Noch nicht implementiert
    void*                       (*CoTaskMemAlloc)       ( ULONG size );
    void                        (*CoTaskMemFree)        ( void* p );
    HRESULT                     (*SetErrorInfo)         ( DWORD, IErrorInfo* );
    HRESULT                     (*GetErrorInfo)         ( DWORD, IErrorInfo** );
    void*                       _reserve1;
  //void                        (*set_javavm)           ( JavaVM* );
  //JavaVM*                     (*get_javavm)           ();
    JavaVM*                     (*request_javavm)       ();
    void                        (*release_javavm)       ( JavaVM* );                    // _count >= 8
};

extern Com_context const*       static_com_context_ptr;
extern const Com_context        com_context;

void                            set_com_context         ( const Com_context* );

//-------------------------------------------------------------------------------Imodule_interface2
// Verbindung zwischen nachgeladenen Modulen,
// z.B. um die JavaVM durchzureichen (die dann nicht geschlossen werden darf).

extern const CLSID CLSID_Module_interface2;
extern const IID   IID_Imodule_interface2;

struct Imodule_interface2 : IUnknown
{
    virtual STDMETHODIMP        module_interface_version()                                          = 0;
    virtual STDMETHODIMP putref_Com_context             ( const Com_context* )                      = 0;
    virtual STDMETHODIMP    put_Log_categories          ( const BSTR )                              = 0;
    virtual STDMETHODIMP    put_Log_context             ( Log_context** )                           = 0;
};

#ifdef Z_WINDOWS
    struct __declspec( uuid( "{feee4706-6c1b-11d8-8103-000476ee8afb}" ) )  Imodule_interface2;
#endif

//--------------------------------------------------------------------------------Imodule_interface
// Veraltet seit 2006-03-03
// Verbindung zwischen nachgeladenen Modulen,
// z.B. um die JavaVM durchzureichen (die dann nicht geschlossen werden darf).

extern const CLSID CLSID_Module_interface;
extern const IID   IID_Imodule_interface;

struct Imodule_interface : IUnknown
{
    virtual STDMETHODIMP        module_interface_version()                                          = 0;
    virtual STDMETHODIMP putref_Com_context             ( const Com_context* )                      = 0;
    virtual STDMETHODIMP    put_Log_categories          ( const BSTR )                              = 0;
    //20036-0303  virtual STDMETHODIMP        Set_stream_and_system_mutex( ostream**, System_mutex* )             = 0;
};

#ifdef Z_WINDOWS
    struct __declspec( uuid( "{feee46ea-6c1b-11d8-8103-000476ee8afb}" ) )  Imodule_interface;
#endif

//------------------------------------------------------------------------------------Com_exception

struct Com_exception : Xc
{
                                Com_exception               ( HRESULT, const string& function, const char* ins1 = NULL, const char* ins2 = NULL );

    HRESULT                    _hresult;
};

//----------------------------------------------------------------------Z_IMPLEMENT_QUERY_INTERFACE

template< class Interface >
inline HRESULT Implement_com_query_interface( Interface* iface, void** result )
{
    *result = iface;  
    iface->AddRef();
    return S_OK;
}

#define Z_IMPLEMENT_QUERY_INTERFACE( THIS, IID, INTERFACE, RESULT ) \
    Z_IMPLEMENT_QUERY_INTERFACE2( THIS, IID, __uuidof(INTERFACE), INTERFACE, RESULT )

#define Z_IMPLEMENT_QUERY_INTERFACE2( THIS, IID, INTERFACE_IID, INTERFACE, RESULT ) \
    do { if( IID == INTERFACE_IID )  return ::zschimmer::com::Implement_com_query_interface( static_cast<INTERFACE*>( THIS ), RESULT ); } while(false)

//---------------------------------------------------------------------------------------------Bstr

struct Bstr
{
                                Bstr                        ()                                      : _bstr( NULL ) {}
#ifdef Z_HAS_MOVE_CONSTRUCTOR
                                Bstr                        (Bstr&& o)                              : _bstr(o._bstr) { o._bstr = NULL; }
#endif
                                Bstr                        ( const Bstr& );
                                Bstr                        ( const OLECHAR* s, size_t size )       : _bstr( NULL ) { alloc_string( s, size ); }
                                Bstr                        ( const OLECHAR* s )                    : _bstr( NULL ) { alloc_string( s ); }
                              //Bstr                        ( const BSTR s )                        : _bstr( NULL ) { alloc_string( s, ::SysStringLen(s) ); }   // Vorsicht! typedef wchar_t* BSTR, SysStringLen() stürzt ab!

#ifndef Z_OLECHAR_IS_WCHAR
                                Bstr                        ( const wchar_t* s )                    : _bstr( NULL ) { alloc_string( s ); }
#endif
#ifndef Z_OLECHAR_IS_UINT16
                                Bstr                        ( const uint16* s )                     : _bstr( NULL ) { alloc_string( s ); }
#endif

                                Bstr                        ( const char* s )                       : _bstr( bstr_from_string( s )      ) {}
                                Bstr                        ( const char* s, size_t len )           : _bstr( bstr_from_string( s, len ) ) {}
                                Bstr                        ( const string& s )                     : _bstr( bstr_from_string( s )      ) {}

#ifndef __GNUC__   // gcc 3.2 hat wstring nicht in der Bibliothek
                                Bstr                        ( const std::wstring& s )               : _bstr( bstr_from_wstring( s )     ) {}
    Bstr&                       operator =                  ( const std::wstring& s )               { ::SysFreeString(_bstr); _bstr = bstr_from_wstring(s); return *this; }
#endif

                               ~Bstr                        ();
    
    Bstr&                       operator =                  ( const Bstr& s )                       { alloc_string( s );  return *this; }
    Bstr&                       operator =                  ( const OLECHAR* s )                    { alloc_string( s );  return *this; }
    Bstr&                       operator =                  ( const char* s )                       { assign      ( s       , strlen( s ) );  return *this; }
    Bstr&                       operator =                  ( const string& s )                     { assign      ( s.data(), s.length()  );  return *this; }

    Bstr&                       operator +=                 ( const BSTR s )                        { append( s ); return *this; }
    Bstr&                       operator +=                 ( const Bstr& s )                       { append( s ); return *this; }
    Bstr&                       operator +=                 ( const string& s )                     { append( s ); return *this; }
    Bstr&                       operator +=                 ( const char* s )                       { append( s ); return *this; }

    void                        clear                       ()                                      { ::SysFreeString(_bstr); _bstr = NULL; }

    size_t                      length                      () const                                { return ::SysStringLen(_bstr); }
    
    BSTR                        copy                        () const                                { uint len = ::SysStringLen(_bstr); return len? ::SysAllocStringLen(_bstr,len) : NULL; }
    void                        copy_to                     ( BSTR* );
    HRESULT                     CopyTo                      ( BSTR* bstr )                          { *bstr = ::SysAllocStringLen( _bstr, ::SysStringLen( _bstr ) );  return *bstr? S_OK : E_OUTOFMEMORY; }
    BSTR                        take                        ()                                      { BSTR result = _bstr;  _bstr = NULL;  return result; }

    void                        assign                      ( const char* s, size_t len )           { ::SysFreeString(_bstr); _bstr = bstr_from_string(s,len); }
    void                        assign                      ( const OLECHAR* s, size_t len )        { alloc_string(s,len); }
    
    void                        attach                      ( BSTR s )                              { ::SysFreeString( _bstr );  _bstr = s; }
    BSTR                        detach                      ()                                      { BSTR s = _bstr; _bstr = NULL; return s; }
    
    void                        append                      ( const Bstr& s )                       { append_bstr( s ); }
    void                        append                      ( const OLECHAR* s )                    { append( s, wcslen(s) ); }

#ifndef Z_OLECHAR_IS_WCHAR
    void                        append                      ( const wchar_t* s )                    { append( s, wcslen(s) ); }
    void                        append                      ( const wchar_t*, size_t len );
#endif

    void                        append_bstr                 ( const BSTR s )                        { append( s, ::SysStringLen(s) ); }
    void                        append                      ( const OLECHAR*, size_t len );
    void                        append                      ( const string& s )                     { append( s.data(), s.length() ); }
    void                        append                      ( const char* s )                       { append( s, s? strlen(s) : 0 ); }
    void                        append                      ( const char* s, size_t len );

    void                        to_lower                    ();

    string                      as_string                   () const                                { return string_from_bstr( _bstr ); }
    size_t                      hash_value                  () const;

                                operator BSTR               () const                                { return _bstr; }
    uint16*                     uint16_ptr                  () const                                { return reinterpret_cast<uint16*>( _bstr ); }
#ifndef Z_OLECHAR_IS_UINT16
//                              operator uint16*            () const                                { return reinterpret_cast<uint16*>( _bstr ); }
#endif
  //                            operator string             () const                                { return string_from_bstr( _bstr ); }

    BSTR*                       operator &                  ()                                      { if(_bstr) clear();  return &_bstr; }
    const OLECHAR&              operator []                 ( int index ) const                     { return _bstr[ index ]; }

    bool                        operator !                  () const                                { return _bstr == NULL; }
    bool                        operator <                  ( const Bstr&    s ) const              { return compare( s ) < 0; }
    bool                        operator <                  ( const OLECHAR* s ) const              { return compare( s ) < 0; }
    bool                        operator ==                 ( const Bstr&    s ) const              { return compare( s ) == 0; }
    bool                        operator ==                 ( const OLECHAR* s ) const              { return compare( s ) == 0; }
    bool                        operator ==                 ( const char*    s ) const              { return compare( s ) == 0; }

    template<typename T>
    bool                        operator !=                 ( const T& t ) const                    { return !( *this == t ); }

    int                         compare                     ( const Bstr&    ) const;
    int                         compare                     ( const OLECHAR* ) const;
    int                         compare                     ( const char*    ) const;

    friend ostream&             operator <<                 ( ostream&, const Bstr& );

  protected:
    void                        alloc_string                ( const Bstr& );
    void                        alloc_string                ( const OLECHAR* );
    void                        alloc_string                ( const OLECHAR*, size_t len );
    void                        alloc_string                ( const char*, size_t len );

#ifndef Z_OLECHAR_IS_WCHAR
    void                        alloc_string                ( const wchar_t* );
    void                        alloc_string                ( const wchar_t*, size_t len );
#endif
#ifndef Z_OLECHAR_IS_UINT16
    void                        alloc_string                ( const uint16* );
    void                        alloc_string                ( const uint16*, size_t len );
#endif

  public:
    BSTR                       _bstr;
};

//---------------------------------------------------------------------------------hash_value(Bstr)
// Für stdext::hash_map<>

inline size_t                   hash_value                  ( const Bstr& bstr )                    { return bstr.hash_value(); }

//---------------------------------------------------------------------------------Bstr hash_value

} //namespace com
} //namespace zschimmer

Z_DEFINE_GNU_HASH_VALUE( zschimmer::com, Bstr )

namespace zschimmer {
namespace com {

//---------------------------------------------------------------------------------------bstr_ref<>

template< class STRING_TYPE >
struct bstr_ref : Bstr                // Der allgemeine Fall ist nicht implementiert.
{
                                bstr_ref                    ( const STRING_TYPE& str )              : Bstr( str ) {}
};

//-----------------------------------------------------------------------------------bstr_ref<BSTR>

template<>
struct bstr_ref< BSTR >
{
                                bstr_ref                    ( BSTR bstr )                           { _bstr = bstr; }
                               ~bstr_ref                    ()                                      { _bstr = NULL; }

                                operator BSTR               ()                                      { return _bstr; }

    BSTR                       _bstr;
};

//-----------------------------------------------------------------------------------bstr_ref<BSTR>

template<>
struct bstr_ref< Bstr >
{
                                bstr_ref                    ( const BSTR& bstr )                    { _bstr = bstr; }
                               ~bstr_ref                    ()                                      { _bstr = NULL; }

                                operator BSTR               ()                                      { return _bstr; }

    BSTR                       _bstr;
};

//---------------------------------------------------------------------------------bstr_ref<string>

template<>
struct bstr_ref< const char* > : Bstr
{
                                bstr_ref                    ( const char* str )                          : Bstr( str ) {}
};

//-------------------------------------------------------------------------------------Variant_kind

enum Variant_kind
{
    vk_none,
    vk_empty,
    vk_null,
    vk_integer,
    vk_float,
    vk_date,
    vk_string
};

Variant_kind                    variant_kind            ( VARTYPE );
inline Variant_kind             variant_kind            ( const VARIANT& v )                       { return variant_kind( v.vt ); }

//------------------------------------------------------------------------------------------Variant

struct Variant : VARIANT
{
    enum                        Vt_error                { vt_error };
    enum                        Vt_array                { vt_array };
    enum                        Vt_missing              { vt_missing };                             // Für optionale Parameter: VT_ERROR, DISP_E_PARAMNOTFOUND
    enum                        Vt_empty                { vt_empty };


                                Variant                 ( const Vt_empty = vt_empty )               { init(); }
                                Variant                 ( const VARIANT& v )                        { init(); copy(&v); }
                                Variant                 ( const Variant& v )                        { init(); copy(&v); }
                               ~Variant                 ()                                          { Clear(); }

#ifndef Z_OLECHAR_IS_WCHAR
                                Variant                 ( const wchar_t* s )                        { init();   *this = s; }
#endif

                                Variant                 ( const OLECHAR* s )                        { init();   *this = s; }
                                Variant                 ( const char* s )                           { init();   *this = s; }
                                Variant                 ( const string& s )                         { init();   *this = s; }
                                Variant                 ( const Bstr& s )                           { init();   *this = s; }

#ifndef __GNUC__   // gcc 3.2 hat wstring nicht in der Bibliothek

                                Variant                 ( const std::wstring& s )                   { init();   *this = s; }
    Variant&                    operator =              ( const std::wstring& );
#endif

                                Variant                 ( bool s )                                  { init();  vt = VT_BOOL;    V_BOOL(this) = s? VARIANT_TRUE : VARIANT_FALSE; }
                                Variant                 ( int s )                                   { init();  vt = VT_I4;      V_I4  (this) = s; }
                                Variant                 ( unsigned int s )                          { init();  vt = VT_UI4;     V_UI4 (this) = s; }
                                Variant                 ( Byte s )                                  { init();  vt = VT_UI1;     V_UI1 (this) = s; }
                                Variant                 ( signed char s )                           { init();  vt = VT_I1;      V_I1  (this) = s; }
                                Variant                 ( short s )                                 { init();  vt = VT_I2;      V_I2  (this) = s; }
                                Variant                 ( unsigned short s )                        { init();  vt = VT_UI2;     V_UI2 (this) = s; }
                              //Variant                 ( long s )            Z_DEPRECATED          { init();  vt = VT_I4;      V_I4  (this) = s; }
                              //Variant                 ( unsigned long s )   Z_DEPRECATED          { init();  vt = VT_UI4;     V_UI4 (this) = s; }
                                Variant                 ( int64 s )                                 { init();  vt = VT_I8;      V_I8  (this) = s; }
                                Variant                 ( uint64 s )                                { init();  vt = VT_UI8;     V_UI8 (this) = s; }
                                Variant                 ( float s )                                 { init();  vt = VT_R4;      V_R4  (this) = s; }
                                Variant                 ( double s )                                { init();  vt = VT_R8;      V_R8  (this) = s; }
                                Variant                 ( CY s )                                    { init();  vt = VT_CY;      V_CY  (this) = s; }

                                Variant                 ( IDispatch* s )                            { init();  *this   = s; }
                                Variant                 ( IUnknown* s )                             { init();  vt = VT_UNKNOWN; punkVal = s; if(s) s->AddRef(); }

                                Variant                 ( Vt_error, SCODE scode = 0 )               { init();  vt = VT_ERROR;   this->scode = scode; }
                                Variant                 ( Vt_missing )                              { init();  vt = VT_ERROR;   this->scode = DISP_E_PARAMNOTFOUND; }
                                Variant                 ( Vt_array, int size, Variant_type = VT_VARIANT );


    Variant&                    operator =              ( const Variant& s )                        { copy(&s); return *this; }
    Variant&                    operator =              ( const VARIANT& s )                        { copy(&s); return *this; }

#ifndef Z_OLECHAR_IS_WCHAR
    Variant&                    operator =              ( const wchar_t* );
#endif

    Variant&                    operator =              ( const OLECHAR* );
    Variant&                    operator =              ( const Bstr& );
    Variant&                    operator =              ( const char* );
    Variant&                    operator =              ( const string& );
    
    Variant&                    operator =              ( bool   s )                                { set_vt( VT_BOOL );  V_BOOL(this) = s? VARIANT_TRUE : VARIANT_FALSE;  return *this;}
    Variant&                    operator =              ( int    s )                                { set_vt( VT_I4   );  V_I4  (this) = s; return *this; }
    Variant&                    operator =              ( unsigned int s )                          { set_vt( VT_UI4  );  V_UI4 (this) = s; return *this; }
    Variant&                    operator =              ( Byte   s )                                { set_vt( VT_UI1  );  V_UI1 (this) = s; return *this; }
    Variant&                    operator =              ( signed char s )                           { set_vt( VT_I1   );  V_I1  (this) = s; return *this; }
    Variant&                    operator =              ( short  s )                                { set_vt( VT_I2   );  V_I2  (this) = s; return *this; }
    Variant&                    operator =              ( unsigned short s )                        { set_vt( VT_UI2  );  V_UI2 (this) = s; return *this; }
  //Variant&                    operator =              ( long   s )           Z_DEPRECATED         { set_vt( VT_I4   );  V_I4  (this) = s; return *this; }
  //Variant&                    operator =              ( unsigned long  s )   Z_DEPRECATED         { set_vt( VT_UI4  );  V_UI4 (this) = s; return *this; }
    Variant&                    operator =              ( int64  s )                                { set_vt( VT_I8   );  V_I8  (this) = s; return *this; }
    Variant&                    operator =              ( uint64 s )                                { set_vt( VT_UI8  );  V_UI8 (this) = s; return *this; }
    Variant&                    operator =              ( float  s )                                { set_vt( VT_R4   );  V_R4  (this) = s; return *this; }
    Variant&                    operator =              ( double s )                                { set_vt( VT_R8   );  V_R8  (this) = s; return *this; }
    Variant&                    operator =              ( CY     s )                                { set_vt( VT_CY   );  V_CY  (this) = s; return *this; }
    
    Variant&                    operator =              ( IDispatch* s )                            { clear(); vt = VT_DISPATCH; pdispVal = s; if(s) s->AddRef(); return *this; }
    Variant&                    operator =              ( IUnknown* );
    Variant&                    operator =              ( Vt_missing )                              { set_vt( VT_ERROR );  scode = DISP_E_PARAMNOTFOUND; return *this; }

    void                        init                    ()                                          { vt = VT_EMPTY; plVal = NULL; } //memset( static_cast<VARIANT*>( this ), 0, sizeof (VARIANT) ); }

    void                        assign_bstr             ( const BSTR& );
    void                        attach_bstr             ( BSTR s )                                  { clear(); vt = VT_BSTR; bstrVal = s; }
    void                        assign                  ( const char* s, size_t len )               { clear(); vt = VT_BSTR; bstrVal = bstr_from_string( s, len ); }

    bool                        operator ==             ( const VARIANT& s ) const                  { return variants_are_equal( *this, s ); }
    bool                        operator <              ( const VARIANT& s ) const                  { return variant_is_lower( *this, s ); }
    bool                        operator >              ( const VARIANT& s ) const                  { return variant_is_lower( s, *this ); }

    bool                        operator ==             ( Vt_error ) const                          { return vt == VT_ERROR; }
    bool                        operator ==             ( Vt_missing ) const                        { return vt == VT_ERROR  &&  scode == DISP_E_PARAMNOTFOUND; }

    template<typename T>
    bool                        operator !=             ( const T& t ) const                        { return !( *this == t ); }

    HRESULT                     Clear                   ()                                          { return ::VariantClear(this); }
    HRESULT                     Copy                    ( const VARIANT* s )                        { return ::VariantCopy( this, const_cast<VARIANT*>(s) ); }
    HRESULT                     CopyTo                  ( VARIANT* v ) const                        { return ::VariantCopy( v, const_cast<Variant*>( this ) ); }
    HRESULT                     Attach                  ( VARIANT* );
    HRESULT                     Detach                  ( VARIANT* );
    HRESULT                     ChangeType              ( VARTYPE new_vt, LCID lcid = STANDARD_LCID )                    { return ::VariantChangeTypeEx( this, this, lcid, 0, new_vt ); }
    HRESULT                     ChangeType              ( VARTYPE new_vt, const VARIANT* s, LCID lcid = STANDARD_LCID )  { return ::VariantChangeTypeEx( this, const_cast<VARIANT*>(s), lcid, 0, new_vt ); }

    bool                        is_empty                () const                                    { return vt == VT_EMPTY; }
    bool                        is_null                 () const                                    { return vt == VT_NULL; }
    bool                        is_error                () const                                    { return vt == VT_ERROR; }
    bool                        is_missing              () const                                    { return vt == VT_ERROR  &&  scode == DISP_E_PARAMNOTFOUND; }
    bool                        is_null_or_empty_string () const;

    void                        clear                   ();
    void                        copy                    ( const VARIANT* );
    void                        attach                  ( VARIANT* );
    void                        detach                  ( VARIANT* );
    void                        change_type             ( VARTYPE, LCID = STANDARD_LCID );
    void                        change_type             ( VARTYPE, const VARIANT&, LCID = STANDARD_LCID );
  //VARIANT                     take                    ()                                          { VARIANT result = *this;  init();  return result; }
    void                        move_to                 ( VARIANT* result )                         { *result = *this;  init(); }

    string                      as_string               () const                                    { return string_from_variant( *this ); }
    bool                        as_bool                 () const                                    { return bool_from_variant( *this ); }
    int64                       as_int64                () const                                    { return int64_from_variant( *this ); }
    int                         as_int                  () const                                    { return int_from_variant( *this ); }

  private:
    void                        set_vt                  ( VARTYPE );
};

//-------------------------------------------------------------------------variant_type_of_type_ptr

inline VARENUM                  variant_type_of_type_ptr( const Byte* )                             { return VT_UI1; }
inline VARENUM                  variant_type_of_type_ptr( const BSTR* )                             { return VT_BSTR; }
inline VARENUM                  variant_type_of_type_ptr( const VARIANT* )                          { return VT_VARIANT; }
inline VARENUM                  variant_type_of_type_ptr( const Variant* )                          { return VT_VARIANT; }

//--------------------------------------------------------------------------------------------const

const extern Variant            empty_variant;
const extern Variant            error_variant;
const extern Variant            missing_variant;

//------------------------------------------------------------------------------------Variant_array
/*
struct Variant_array : Variant
{
                                Variant_array               ( int size );
                             //~Variant_array               ();

    void                        lock                        ()                                      { SafeArrayLock( V_ARRAY(this) ); }
    Variant&                    operator []                 ( int index );
};
*/
//----------------------------------------------------------------------------------------Safearray
/*
struct Safearray
{
                                Safearray                   ( int size );
                               ~Safearray                   ();

                                operator SAFEARRAY*         ()                                      { return _safearray; }
                                operator &                  ()                                      { return &_safearray; }

    SAFEARRAY*                 _safearray;
};
*/
//----------------------------------------------------------------------------Locked_any_safearray

struct Locked_any_safearray : Non_cloneable
{
                                Locked_any_safearray        ( SAFEARRAY* safearray );
                                Locked_any_safearray        ( SAFEARRAY* safearray, VARENUM expected_vartype );
                               ~Locked_any_safearray        ();

                                operator SAFEARRAY*         () const                                { return _safearray; }
    SAFEARRAY*                  operator ->                 () const                                { return _safearray; }                               
    VARTYPE                     vartype                     () const;
    const int                   count                       () const                                { return _count; }
    SAFEARRAY*                  safearray_ptr               () const                                { return _safearray; }
    SAFEARRAY*                  take_safearray              ();
    void                        check_index                 ( int index )                           { if( (uint)index >= (uint)count() )  throw_index_error( index ); }

  protected:
                                Locked_any_safearray        ();
    void                        create                      ( int size, VARENUM variant_type );
    void                        set_count                   ();
    Z_NORETURN void             throw_index_error           ( int index );

    SAFEARRAY*                 _safearray;
    void*                      _array;
    int                        _count;
    bool                       _delete;
};

//---------------------------------------------------------------------------------Locked_safearray

template< typename ELEMENT_TYPE >
struct Locked_safearray : Locked_any_safearray
{
                                Locked_safearray            ( SAFEARRAY* safearray )                : Locked_any_safearray( safearray, variant_type_of_type_ptr( (ELEMENT_TYPE*)NULL ) ) {}
                                Locked_safearray            ( int size )                            { create( size, variant_type_of_type_ptr( (ELEMENT_TYPE*)NULL ) ); }

    ELEMENT_TYPE*               data                        ()                                      { return (ELEMENT_TYPE*)_safearray->pvData; }
    
    ELEMENT_TYPE&               operator []                 ( int index )                           // Immer ab 0!
    {
        check_index( index );
        return data()[ index ]; 
    }
};

//----------------------------------------------------------------------------------------Excepinfo

struct Excepinfo : EXCEPINFO
{
                                Excepinfo                   ()                                      { memset( (EXCEPINFO*)this, 0, sizeof (EXCEPINFO) ); }
                               ~Excepinfo                   ();
};

//---------------------------------------------------------------------------------------Dispparams

struct Dispparams : DISPPARAMS
{
                                Dispparams                  ()                                      { memset( (DISPPARAMS*)this, 0, sizeof (DISPPARAMS) ); }
                               ~Dispparams                  ()                                      { delete[] (Variant*)rgvarg; if( rgdispidNamedArgs != (DISPID*)&dispid_propertyput )  delete [] rgdispidNamedArgs; }

    void                    set_arg_count                   ( int count )                           { if(count) rgvarg = new Variant [count];  cArgs = count; }
    int                         arg_count                   () const                                { return cArgs; }
    void                    set_property_put                ()                                      { rgdispidNamedArgs = (DISPID*)&dispid_propertyput;  cNamedArgs = 1; }
    void                    set_named_arg_count             ( int count )                           { cNamedArgs = count; rgdispidNamedArgs = new DISPID[ count ]; }
    void                    set_dispid                      ( int i, DISPID dispid )                { assert((uint)i<(uint)cNamedArgs); rgdispidNamedArgs[i] = dispid; }

    Variant&                    operator []                 ( int i )                               { assert((uint)i<(uint)cArgs); return *(Variant*)&rgvarg[cArgs-1-i]; }

    const static DISPID         dispid_propertyput;
};

//-----------------------------------------------------------------------------------------Multi_qi

struct Multi_qi
{
                                Multi_qi                    ( int size = 0 );
                               ~Multi_qi                    ();

                                operator MULTI_QI*          ()                                      { return _multi_qi; }
    MULTI_QI&                   operator []                 ( int );

    void                        allocate                    ( int size );
    void                        clear                       ();

    void                    set_iid                         ( int index, const IID& );
    void                    set_interface                   ( int index, IUnknown* );

    int                        _size;
    MULTI_QI*                  _multi_qi;
};

//-----------------------------------------------------------------------------------ErrorInfo_impl
#ifndef SYSTEM_HAS_COM

struct ErrorInfo_impl : ICreateErrorInfo,
                        IErrorInfo
{
    void*                       operator new                ( size_t size )                         { return CoTaskMemAlloc( size ); }
    void                        operator delete             ( void* p )                             { CoTaskMemFree( p ); }

                                ErrorInfo_impl              ()                                      : _zero_(this+1) {}
    virtual                    ~ErrorInfo_impl              ()                                      {}

    virtual ULONG               AddRef                      ()                                      { return ++_ref_count; }
    virtual ULONG               Release                     ();
    HRESULT                     QueryInterface              ( const IID&, void** );


    // ICreateErrorInfo
    virtual HRESULT             SetDescription              ( const OLECHAR* d )                    { _description = d;     return S_OK; }
    virtual HRESULT             SetGUID                     ( const GUID& guid )                    { _guid = guid;         return S_OK; }
    virtual HRESULT             SetHelpContext              ( DWORD hc )                            { _help_context = hc;   return S_OK; }
    virtual HRESULT             SetHelpFile                 ( const OLECHAR* s )                    { _help_file = s;       return S_OK; }
    virtual HRESULT             SetSource                   ( const OLECHAR* s )                    { _source = s;          return S_OK; }


    // IErrorInfo
    virtual HRESULT             GetDescription              ( BSTR* d )                             { return _description.CopyTo(d); }
    virtual HRESULT             GetGUID                     ( GUID* guid )                          { *guid = _guid;        return S_OK; }
    virtual HRESULT             GetHelpContext              ( DWORD* hc )                           { *hc = _help_context;  return S_OK; }
    virtual HRESULT             GetHelpFile                 ( BSTR* s )                             { return _help_file.CopyTo(s); }
    virtual HRESULT             GetSource                   ( BSTR* s )                             { return _source.CopyTo(s); }


    Fill_zero                  _zero_;
    int                        _ref_count;
    Bstr                       _description;
    GUID                       _guid;
    DWORD                      _help_context;
    Bstr                       _help_file;
    Bstr                       _source;
};

#endif

//--------------------------------------------------------------------------------Idispatch_dispids
// Merkt sich die DISPIDs von GetIDsOfNames()

struct Idispatch_dispids : Object
{
                                Idispatch_dispids           ( IDispatch*, const Bstr& name = "" );

    void                        add_dispid                  ( const Bstr& name, DISPID dispid )     { _map[ name ] = dispid; }

    DISPID                      get_dispid                  ( const Bstr& name );
    HRESULT                     Get_dispid                  ( const Bstr& name, DISPID* result );


    typedef stdext::hash_map< Bstr, DISPID >  Map;

    Bstr                       _name;
    ptr<IDispatch>             _idispatch;
    Map                        _map;
};

//---------------------------------------------------------------------------------Idispatch_member

struct Idispatch_member
{
                                Idispatch_member            ()                                      : _dispid(DISPID_UNKNOWN) {}

                                operator bool               () const                                { return _dispid != DISPID_UNKNOWN; }   
    bool                        operator !                  () const                                { return _dispid == DISPID_UNKNOWN; }   

    HRESULT                     Invoke                      ( WORD flags, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* arg_nr )
    {
        if( !_idispatch )  return E_POINTER;
        return _idispatch->Invoke( _dispid, IID_NULL, STANDARD_LCID, flags, dispparams, result, excepinfo, arg_nr );
    }

    ptr<IDispatch>             _idispatch;
    DISPID                     _dispid;
};

//-------------------------------------------------------------------------------------------------

bool            variant_is_integer      ( VARTYPE );
inline bool     variant_is_integer      ( const VARIANT& v )                                        { return variant_is_integer( v.vt ); }
bool            variant_is_int64        ( VARTYPE );
inline bool     variant_is_int64        ( const VARIANT& v )                                        { return variant_is_int64( v.vt ); }
bool            variant_is_double       ( VARTYPE );
inline bool     variant_is_double       ( const VARIANT& v )                                        { return variant_is_double ( v.vt ); }
bool            variant_is_numeric      ( VARTYPE );
inline bool     variant_is_numeric      ( const VARIANT& v )                                        { return variant_is_numeric( v.vt ); }
inline bool     variant_is_missing      ( const VARIANT& v )                                        { return v.vt == VT_ERROR  &&  v.scode == DISP_E_PARAMNOTFOUND; }

inline Variant  variant_from_string     ( const string& str )                                       { Variant v; v.vt = VT_BSTR; v.bstrVal = bstr_from_string( str ); return v; }

//inline Bstr     com_bstr_from_string    ( const string& str )                                       { Bstr bstr; bstr.attach( bstr_from_string( str.c_str(), str.length() ) ); return bstr; }

Variant                         com_property_get            ( IDispatch*, const string& property );

//---------------------------------------------------------------------------------variant_is_empty

inline bool variant_is_empty( const Variant& v )
{
    return  v.vt == VT_NULL  
        ||  v.vt == VT_EMPTY  
        ||  v.vt == VT_BSTR && SysStringLen(V_BSTR(&v)) == 0;
}

//----------------------------------------------------------------------------------variant_default

template<class T>
Variant variant_default( const Variant& v, const T& deflt )
{
    if( variant_is_empty( v ) )  return deflt;
                           else  return v;
}

inline
Variant variant_default( const Variant& v, const string& deflt )
{
    if( variant_is_empty( v ) )  return deflt.c_str();
                           else  return v;
}

//-------------------------------------------------------------------------------------------------
/*
HRESULT                         Com_call                    ( VARIANT* result, IDispatch*, const string& method, 
                                                              const Variant& p1 = missing_variant, 
                                                              const Variant& p2 = missing_variant, 
                                                              const Variant& p3 = missing_variant, 
                                                              const Variant& p4 = missing_variant, 
                                                              const Variant& p5 = missing_variant );

inline HRESULT                  Com_call                    ( IDispatch* idispatch, const string& method, 
                                                              const Variant& p1 = missing_variant, 
                                                              const Variant& p2 = missing_variant, 
                                                              const Variant& p3 = missing_variant, 
                                                              const Variant& p4 = missing_variant, 
                                                              const Variant& p5 = missing_variant ) { return Com_call( NULL, idispatch, method, p1, p2, p3, p4, p5 ); }
*/
//-----------------------------------------------------------------------------------Com_initialize

struct Com_initialize
{
                                Com_initialize              ();
                               ~Com_initialize              ();
};

//-------------------------------------------------------------------------------------------------

} //namespace com

//-------------------------------------------------------------------------------------------------

inline void insert_into_message( Message_string* m, int index, const com::Bstr& bstr ) throw()       { return m->insert_string( index, string_from_bstr( bstr ) ); }
inline void insert_into_message( Message_string* m, int index, const com::Variant& v ) throw()       { return m->insert_string( index, debug_string_from_variant( v ) ); }

} //namespace zschimmer

//-------------------------------------------------------------------------------------------------

#endif
