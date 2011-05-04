// $Id$

// §1719

#ifndef __ZSCHIMMER_COM_BASE_H
#define __ZSCHIMMER_COM_BASE_H


// Am Ende sind einige Voraus-Deklarationen für zschimmer::com (aus z_com.h) zum Gebrauch für zschimmer.h ptr<>



#ifdef _WIN32

#   define SYSTEM_HAS_COM
#   define SYSTEM_HAS_IDISPATCH
#   define Z_OLECHAR_IS_WCHAR       // sizeof (OLECHAR) == sizeof (wchar_t)

#   define _WIN32_DCOM              // Für CoCreateInstanceEx()

#   define COM_LINKAGE              // Nur für Unix

#   include <winsock2.h>            // Damit nicht winsock.h eingezogen wird.
#   include <windows.h>
#   include <comdef.h>
#   include <unknwn.h>              // IUnknown

#   ifndef V_I8
#       define V_I8(VARIANT)      ((VARIANT)->cyVal.int64)
#       define V_I8REF(VARIANT)   ((int64*)(VARIANT)->piVal)
#       define V_UI8(VARIANT)     (*(uint64*)&(VARIANT)->cyVal.int64)
#       define V_UI8REF(VARIANT)  ((uint64*)&(VARIANT)->piVal)
#   endif

    typedef VARTYPE Variant_type;   // unsigned short. Bei eigener Implementierung für Unix ein typsicherer enum

#   define Z_DEFINE_GUID(NAME,A,B,C,D,E,F,G,H,I,J,K)  const GUID NAME = { A,B,C,D,E,F,G,H,I,J,K }
#   define DEFINE_UUIDOF( INTERFACE )
#   define Z_DEFINE_UUID_OF( INTERFACE, IID, IID_STRING )  struct __declspec( uuid( IID_STRING ) )  INTERFACE;

#else

//-------------------------------------------------------------------------------------------------

//#define Z_OLECHAR_IS_WCHAR    // sizeof (OLECHAR) == sizeof (wchar_t)

#define Z_COM

#ifdef Z_LINK_STATIC
#   define COM_LINKAGE
#else
#   define COM_LINKAGE  __attribute__((weak))
#endif

//------------------------------------------------------------------------------------Windows-Namen

#include "olechar.h"

#define WINAPI
#define CP_ACP          0           // default to ANSI code page
#define CP_UTF8         65001

#ifndef __SQLTYPES_H   // unixodbc
struct _Hinstance {};  typedef _Hinstance* HINSTANCE;
#endif

struct _Handle    {};  typedef _Handle*    HANDLE;

typedef unsigned char       BYTE;
typedef unsigned short int  WORD;
typedef unsigned int        DWORD;      // 32bit

typedef DWORD*              LPDWORD;

typedef char                CHAR;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef int                 INT;
typedef unsigned int        UINT;

typedef int                 LONG;       // Unter Windows hat long immer 32 Bits
typedef unsigned int        ULONG;      // Unter Windows hat long immer 32 Bits
typedef int                 BOOL;
typedef float               FLOAT;
typedef double              DOUBLE;

typedef char*               LPSTR;
typedef const char*         LPCSTR;

typedef unsigned int        LCID;


#define STDMETHODIMP_( TYPE )   WINAPI TYPE
#define STDMETHODIMP            STDMETHODIMP_(HRESULT)
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method

#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method

//#define __uuidof(CLASS)         CLASS::__uuid


#define __uuidof(CLASS)         ( __uuidof_( (CLASS*)NULL ) )

//-----------------------------------------------------------------------------------------forwards

struct                  SAFEARRAY;
struct                  VARIANT;
struct                  DISPPARAMS;
struct                  EXCEPINFO;
struct                  ITypeInfo;

//------------------------------------------------------------------------------------------------

typedef LONG            DISPID;
typedef DISPID          MEMBERID;
typedef unsigned short  VARTYPE;
typedef VARIANT         VARIANTARG;

//-------------------------------------------------------------------------------------------------

const int               CLSCTX_ALL          = 0; //(CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)

//------------------------------------------------------------------------------------------HRESULT

typedef LONG     HRESULT;
typedef HRESULT  SCODE;

inline bool                     FAILED                      ( HRESULT hr )                          { return ( hr & 0x80000000 ) != 0; }
inline bool                     SUCCEEDED                   ( HRESULT hr )                          { return !FAILED(hr); }
HRESULT                         hresult_from_win32          ( LONG );
inline HRESULT                  HRESULT_FROM_WIN32          ( LONG x )                              { return hresult_from_win32( x ); }


const ULONG   ERROR_INVALID_PARAMETER       = 87;
const ULONG   ERROR_INSUFFICIENT_BUFFER     = 122;

const HRESULT NOERROR                       = 0;
const HRESULT S_OK                          = 0;
const HRESULT S_FALSE                       = 1;

const HRESULT ERROR                         = 0x80000000;
const HRESULT FACILITY_WIN32                = 7;

const HRESULT E_NOTIMPL                     = 0x80004001;
const HRESULT E_NOINTERFACE                 = 0x80004002;
const HRESULT E_POINTER                     = 0x80004003;
const HRESULT E_ABORT                       = 0x80004004;
const HRESULT E_FAIL                        = 0x80004005;
const HRESULT E_UNEXPECTED                  = 0x8000FFFF;

const HRESULT DISP_E_UNKNOWNINTERFACE       = 0x80020001;
const HRESULT DISP_E_MEMBERNOTFOUND         = 0x80020003;
const HRESULT DISP_E_PARAMNOTFOUND          = 0x80020004;
const HRESULT DISP_E_TYPEMISMATCH           = 0x80020005;
const HRESULT DISP_E_UNKNOWNNAME            = 0x80020006;
const HRESULT DISP_E_NONAMEDARGS            = 0x80020007;
const HRESULT DISP_E_BADVARTYPE             = 0x80020008;
const HRESULT DISP_E_EXCEPTION              = 0x80020009;
const HRESULT DISP_E_OVERFLOW               = 0x8002000A;

const HRESULT DISP_E_BADINDEX               = 0x8002000B;

//#define DISP_E_UNKNOWNLCID               _HRESULT_TYPEDEF_(0x8002000CL)
//#define DISP_E_ARRAYISLOCKED             _HRESULT_TYPEDEF_(0x8002000DL)
const HRESULT DISP_E_BADPARAMCOUNT          = 0x8002000E;
//#define DISP_E_PARAMNOTOPTIONAL          _HRESULT_TYPEDEF_(0x8002000FL)
//#define DISP_E_BADCALLEE                 _HRESULT_TYPEDEF_(0x80020010L)
//#define DISP_E_NOTACOLLECTION            _HRESULT_TYPEDEF_(0x80020011L)
//#define DISP_E_DIVBYZERO                 _HRESULT_TYPEDEF_(0x80020012L)

const HRESULT TYPE_E_ELEMENTNOTFOUND        = 0x8002802B;

const HRESULT CLASS_E_NOTLICENSED           = 0x80040112;
const HRESULT CLASS_E_CLASSNOTAVAILABLE     = 0x80040111;
const HRESULT CLASS_E_NOAGGREGATION         = 0x80040110;

const HRESULT REGDB_E_WRITEREGDB            = 0x80040151;
const HRESULT REGDB_E_CLASSNOTREG           = 0x80040154;

const HRESULT CO_E_NOTINITIALIZED           = 0x800401F0;
const HRESULT CO_E_DLLNOTFOUND              = 0x800401F8;
const HRESULT CO_E_ERRORINDLL               = 0x800401F9;

const HRESULT E_ACCESSDENIED                = 0x80070005;
const HRESULT E_HANDLE                      = 0x80070006;
const HRESULT E_OUTOFMEMORY                 = 0x8007000E;
const HRESULT E_INVALIDARG                  = 0x80070057;

const HRESULT CO_S_NOTALLINTERFACES         = 0x00080012;

//---------------------------------------------------------------------------------------------GUID

struct GUID
{
/*
    GUID() { memset( &Data1, 0, 16 ); }

    GUID( DWORD a, WORD b, WORD c, BYTE d, BYTE e, BYTE f, BYTE g, BYTE h, BYTE i, BYTE j, BYTE k )
    {
        Data1    = a;
        Data2    = b;
        Data3    = c;
        Data4[0] = d;
        Data4[1] = e;
        Data4[2] = f;
        Data4[3] = g;
        Data4[4] = h;
        Data4[5] = i;
        Data4[6] = j;
        Data4[7] = k;
    }
*/

    bool operator == ( const GUID& b ) const   { return memcmp( &Data1, &b.Data1, 16 ) == 0; }
    bool operator != ( const GUID& b ) const   { return memcmp( &Data1, &b.Data1, 16 ) != 0; }


    DWORD  Data1;
    WORD   Data2;
    WORD   Data3;
    BYTE   Data4[ 8 ];
};

typedef const GUID&             REFGUID;

#define Z_DEFINE_GUID(NAME,A,B,C,D,E,F,G,H,I,J,K)  const GUID NAME = { A, B, C, {D,E,F,G,H,I,J,K} }
#define   DEFINE_GUID(NAME,A,B,C,D,E,F,G,H,I,J,K)  Z_DEFINE_GUID( NAME,A,B,C,D,E,F,G,H,I,J,K )

//#define DEFINE_GUID_(TYPE,NAME,A,B,C,D,E,F,G,H,I,J,K)  const TYPE NAME (A,B,C,D,E,F,G,H,I,J,K)
//#define DEFINE_GUID(      NAME,A,B,C,D,E,F,G,H,I,J,K)  DEFINE_GUID_(GUID ,NAME,A,B,C,D,E,F,G,H,I,J,K)
//#define DEFINE_IID(       NAME,A,B,C,D,E,F,G,H,I,J,K)  DEFINE_GUID_(IID  ,NAME,A,B,C,D,E,F,G,H,I,J,K)
//#define DEFINE_CLSID(     NAME,A,B,C,D,E,F,G,H,I,J,K)  DEFINE_GUID_(CLSID,NAME,A,B,C,D,E,F,G,H,I,J,K)

extern const GUID GUID_NULL;
//DEFINE_GUID( GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

#define DEFINE_UUIDOF( INTERFACE )                      friend inline const GUID& __uuidof_( const INTERFACE* ) { return IID_##INTERFACE; }

#define Z_DEFINE_UUID_OF( INTERFACE, IID, IID_STRING ) \
    struct INTERFACE; \
    inline const GUID& __uuidof_( const INTERFACE* ) { return IID; }

//----------------------------------------------------------------------------------------------IID

typedef GUID                    IID;
typedef const IID&              REFIID;
extern const GUID               IID_NULL;
//DEFINE_GUID( IID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

//--------------------------------------------------------------------------------------------CLSID

typedef GUID                    CLSID;
typedef const CLSID&            REFCLSID;
extern const GUID               CLSID_NULL;
//DEFINE_GUID( CLSID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

//-----------------------------------------------------------------------------------------IUnknown

extern const IID IID_IUnknown;
//DEFINE_GUID( IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );

struct IUnknown
{
    DEFINE_UUIDOF( IUnknown )

    virtual STDMETHODIMP            QueryInterface      ( const IID&, void** ) = 0;
    virtual STDMETHODIMP_( ULONG )  AddRef              () = 0;
    virtual STDMETHODIMP_( ULONG )  Release             () = 0;
};

typedef IUnknown* LPUNKNOWN;

//----------------------------------------------------------------------------------------IDispatch

extern const IID IID_IDispatch;
//DEFINE_GUID( IID_IDispatch, 0x00020400, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );

struct IDispatch : IUnknown
{
    DEFINE_UUIDOF( IDispatch )

    virtual HRESULT             GetTypeInfoCount        ( UINT* )                                   = 0;
    virtual HRESULT             GetTypeInfo             ( UINT, LCID, ITypeInfo** )                 = 0;
    virtual HRESULT             GetIDsOfNames           ( REFIID, LPOLESTR*, UINT, LCID, DISPID* )  = 0;
    virtual HRESULT             Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* ) = 0;
};

typedef IDispatch* LPDISPATCH;

//-------------------------------------------------------------------------------------------------

#endif



//-------------------------------------------------------------------------namespace zschimmer::com
// Einige Voraus-Deklarationen (z_com.cxx)

namespace zschimmer {
namespace com {


HRESULT                         Name_to_clsid               ( const OLECHAR* class_name, CLSID* result );
HRESULT                         Name_to_clsid               ( const char*    class_name, CLSID* result );

IUnknown*                       com_create_instance         ( const CLSID& clsid    , const IID&, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL );
IUnknown*                       com_create_instance         ( const char* class_name, const IID&, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL );

IUnknown*                       com_query_interface         ( IUnknown*, const IID& );
IUnknown*                       com_query_interface_or_null ( IUnknown*, const IID& );

HRESULT                         Call_query_interface        ( IUnknown*, const IID&, void** );

//------------------------------------------------------------------------------Com_query_interface

template< class RESULT_INTERFACE >
HRESULT Com_query_interface( IUnknown* iunknown, RESULT_INTERFACE** result, const IID& iid = __uuidof( RESULT_INTERFACE ) )
{
    void* void_ptr = NULL;

    HRESULT hr = Call_query_interface( iunknown, iid, &void_ptr );

    *result = FAILED(hr)? NULL 
                        : static_cast<RESULT_INTERFACE*>( void_ptr );

    return hr;
}

//------------------------------------------------------------------------------com_query_interface

template< class RESULT_INTERFACE >
inline void com_query_interface( IUnknown* object, RESULT_INTERFACE** result )
{ 
    *result = static_cast< RESULT_INTERFACE* >( com_query_interface( object, __uuidof( RESULT_INTERFACE ) ) ); 
}

//----------------------------------------------------------------------com_query_interface_or_null

template< class RESULT_INTERFACE >
inline bool com_query_interface_or_null( IUnknown* object, RESULT_INTERFACE** result )
{ 
    *result = static_cast< RESULT_INTERFACE* >( com_query_interface_or_null( object, __uuidof( RESULT_INTERFACE ) ) );  
    return *result != NULL; 
}

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer

//-------------------------------------------------------------------------------------------------

#endif
