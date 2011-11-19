// $Id: com.h 12977 2007-09-09 12:23:56Z jz $

#ifndef __ZSCHIMMER_COM_H
#define __ZSCHIMMER_COM_H

#include "com_base.h"

#ifndef _WIN32


//------------------------------------------------------------------------------------------------

#include <wchar.h>      // GNU
#include <string.h>
#include <assert.h>
//#include <alloc.h>

#include "base.h"
#include "olechar.h"

//------------------------------------------------------------------------------------------Windows

struct SYSTEMTIME 
{ 
    WORD wYear; 
    WORD wMonth; 
    WORD wDayOfWeek; 
    WORD wDay; 
    WORD wHour; 
    WORD wMinute; 
    WORD wSecond; 
    WORD wMilliseconds; 
}; 


#ifndef FALSE  //usr/include/sqltypes.h
const BOOL              FALSE = 0;
#endif

#ifndef TRUE   //usr/include/sqltypes.h
const BOOL              TRUE  = 1;
#endif

// DllMain()
const DWORD             DLL_PROCESS_ATTACH = 1;
const DWORD             DLL_PROCESS_DETACH = 2;
const DWORD             DLL_THREAD_ATTACH  = 3;
const DWORD             DLL_THREAD_DETACH  = 4;

const DWORD             Z_DLL_COM_ATTACH = 101;         // Eigene Erweiterung, um Com_context zu übergeben (für SetError() etc.)
const DWORD             Z_DLL_COM_DETACH = 102;         // Eigene Erweiterung 

#define LOCALE_SYSTEM_DEFAULT  0u
#define LANG_ENGLISH           0u
#define SORT_DEFAULT           0u
#define MAKELANGID(x,y)        0u
#define MAKELCID(x,y)          0u
#define SUBLANG_DEFAULT        0u

//----------------------------------------------------------------------------------------------COM

#define DISPATCH_METHOD         0x1
#define DISPATCH_PROPERTYGET    0x2
#define DISPATCH_PROPERTYPUT    0x4
#define DISPATCH_PROPERTYPUTREF 0x8

#define	DISPID_VALUE	        (  0 )
#define	DISPID_UNKNOWN	        ( -1 )
#define	DISPID_PROPERTYPUT      ( -3 )
#define	DISPID_NEWENUM	        ( -4 )
#define	DISPID_EVALUATE	        ( -5 )
#define	DISPID_CONSTRUCTOR      ( -6 )
#define	DISPID_DESTRUCTOR       ( -7 )
#define	DISPID_COLLECT          ( -8 )

const int WC_NO_BEST_FIT_CHARS = 0x00000400;

#define APIENTRY                WINAPI
#define WINOLEAPI               HRESULT
#define STDMETHODCALLTYPE

//-------------------------------------------------------------------------------------------------

void*                           CoTaskMemAlloc          ( ULONG  );
void                            CoTaskMemFree           ( void* );

//------------------------------------------------------------------------------------------VARIANT

typedef short                   VARIANT_BOOL;
const VARIANT_BOOL              VARIANT_FALSE = 0;
const VARIANT_BOOL              VARIANT_TRUE  = -1;

//---------------------------------------------------------------------------------------------BSTR

typedef OLECHAR* BSTR;

BSTR        SysAllocStringLen       ( const OLECHAR*, UINT len );
BSTR        SysAllocString          ( const OLECHAR* );
BSTR        SysAllocString          ( const wchar_t* );                     // Für SysAllocString( L"..." )  (Unix)

void        SysFreeString           ( BSTR );
int         SysReAllocStringLen     ( BSTR*, const OLECHAR*, UINT len );
UINT        SysStringLen            ( BSTR );
inline UINT SysStringByteLen        ( BSTR bstr )                           { return sizeof (OLECHAR) * SysStringLen( bstr ); }

//---------------------------------------------------------------------------------------DISPPARAMS

struct DISPPARAMS
{
    VARIANTARG*                 rgvarg;
    DISPID*                     rgdispidNamedArgs;
    UINT                        cArgs;
    UINT                        cNamedArgs;
};

//-------------------------------------------------------------------------------------IEnumVARIANT

extern const IID IID_IEnumVARIANT;
//DEFINE_GUID( IID_IEnumVARIANT, 0x00020404, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
    
struct IEnumVARIANT : IUnknown
{
    DEFINE_UUIDOF( IEnumVARIANT )

    virtual HRESULT             Next                    ( ULONG, VARIANT*, ULONG* )                 = 0;
    virtual HRESULT             Skip                    ( ULONG )                                   = 0;
    virtual HRESULT             Reset                   ()                                          = 0;
    virtual HRESULT             Clone                   ( IEnumVARIANT** )                          = 0;
};

//------------------------------------------------------------------------------------IClassFactory

extern const IID IID_IClassFactory;
//DEFINE_GUID( IID_IClassFactory, 0x00000001, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );

struct IClassFactory : IUnknown
{
    DEFINE_UUIDOF( IClassFactory )
    
    virtual HRESULT                 CreateInstance      ( IUnknown* outer, const IID&, void** ) = 0;
};

//--------------------------------------------------------------------------------ISupportErrorInfo

extern const IID IID_ISupportErrorInfo;
//DEFINE_GUID( IID_ISupportErrorInfo, 0xDF0B3D60, 0x548F, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );

struct ISupportErrorInfo : IUnknown
{
    DEFINE_UUIDOF( ISupportErrorInfo )
};

//---------------------------------------------------------------------------------------IErrorInfo

extern const IID IID_IErrorInfo;
//DEFINE_GUID( IID_IErrorInfo, 0x1CF2B120, 0x547D, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );

struct IErrorInfo : IUnknown
{
    DEFINE_UUIDOF( IErrorInfo )

    virtual HRESULT             GetDescription              ( BSTR* ) = 0;
    virtual HRESULT             GetGUID                     ( GUID* ) = 0;
    virtual HRESULT             GetHelpContext              ( DWORD* ) = 0;
    virtual HRESULT             GetHelpFile                 ( BSTR* ) = 0;
    virtual HRESULT             GetSource                   ( BSTR* ) = 0;
};

HRESULT                         SetErrorInfo                ( DWORD dwReserved, IErrorInfo* );
HRESULT                         GetErrorInfo                ( DWORD dwReserved, IErrorInfo** );
 
//----------------------------------------------------------------------------------------ITypeInfo

extern const IID IID_ITypeInfo;

struct ITypeInfo : IUnknown
{
    DEFINE_UUIDOF( ITypeInfo )

    virtual HRESULT             GetNames                    ( MEMBERID, BSTR*, unsigned int cMaxNames, unsigned int* pcNames ) = 0;
};

//-----------------------------------------------------------------------------------------ITypeLib

extern const IID IID_ITypeLib;

struct ITypeLib : IUnknown
{
    DEFINE_UUIDOF( ITypeLib )
};

//---------------------------------------------------------------------------------ICreateErrorInfo

extern const IID IID_ICreateErrorInfo;
//DEFINE_GUID( IID_ICreateErrorInfo, 0x22F03340, 0x547D, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );

struct ICreateErrorInfo : IUnknown  // eigentlich IDispatch 
{
    DEFINE_UUIDOF( ICreateErrorInfo )

    virtual HRESULT             SetDescription              ( const OLECHAR* ) = 0;
    virtual HRESULT             SetGUID                     ( const GUID& ) = 0;
    virtual HRESULT             SetHelpContext              ( DWORD ) = 0;
    virtual HRESULT             SetHelpFile                 ( const OLECHAR* ) = 0;
    virtual HRESULT             SetSource                   ( const OLECHAR* ) = 0;
};

HRESULT                         CreateErrorInfo             ( ICreateErrorInfo** );

//--------------------------------------------------------------------------------IProvideClassInfo

extern const IID IID_IProvideClassInfo;
//DEFINE_GUID( IID_IProvideClassInfo, 0xB196B283, 0xBAB4, 0x101A, 0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07);

struct IProvideClassInfo : IUnknown
{
    DEFINE_UUIDOF( IProvideClassInfo )

    virtual HRESULT             GetClassInfo                ( ITypeInfo** )                         = 0;
};

//-----------------------------------------------------------------------------------------CURRENCY

struct CURRENCY
{
    zschimmer::int64 int64;
};

typedef CURRENCY  CY;

//------------------------------------------------------------------------------------------DECIMAL

struct DECIMAL
{
    USHORT wReserved;

    union 
    {
        //struct 
        //{
            char scale;     // The number of decimal places for the
                            // number. Valid values are from 0 to 28. So
                            // 12.345 is represented as 12345 with a 
                            // scale of 3.
            char sign;      // 0 for positive numbers or DECIMAL_NEG for 
                            // negative numbers. So -1 is represented as 
                            // 1 with the DECIMAL_NEG bit set.
        //};

        //USHORT signscale;
    };

    ULONG            Hi32;
    zschimmer::int64 Lo64;
};

//---------------------------------------------------------------------------------------------DATE

typedef double  DATE;

//-------------------------------------------------------------------------------------Variant_type

enum Variant_type
{
    VT_EMPTY            = 0,
    VT_NULL             = 1,
    VT_I2               = 2,
    VT_I4               = 3,
    VT_R4               = 4,
    VT_R8               = 5,
    VT_CY               = 6,
    VT_DATE             = 7,
    VT_BSTR             = 8,
    VT_DISPATCH         = 9,
    VT_ERROR            = 10,
    VT_BOOL             = 11,
    VT_VARIANT          = 12,
    VT_UNKNOWN          = 13,
    VT_DECIMAL          = 14,
    VT_I1               = 16,
    VT_UI1              = 17,
    VT_UI2              = 18,
    VT_UI4              = 19,
    VT_I8               = 20,
    VT_UI8              = 21,
    VT_INT              = 22,
    VT_UINT             = 23,
    VT_VOID             = 24,
    VT_HRESULT          = 25,
    VT_PTR              = 26,
    VT_SAFEARRAY        = 27,
    VT_CARRAY           = 28,
    VT_USERDEFINED      = 29,
    VT_LPSTR            = 30,
    VT_LPWSTR           = 31,
    VT_RECORD           = 36,
    VT_FILETIME         = 64,
    VT_BLOB             = 65,
    VT_STREAM           = 66,
    VT_STORAGE          = 67,
    VT_STREAMED_OBJECT  = 68,
    VT_STORED_OBJECT    = 69,
    VT_BLOB_OBJECT      = 70,
    VT_CF               = 71,
    VT_CLSID            = 72,

    VT_LONG             = VT_I4,        // Erweiterung. Zschimmer 29.11.2002
    VT_ULONG            = VT_UI4,       // Erweiterung. Zschimmer 29.11.2002

    VT_BSTR_BLOB        = 0x0fff,
    VT_VECTOR           = 0x1000,
    VT_ARRAY            = 0x2000,
    VT_BYREF            = 0x4000,
    VT_RESERVED         = 0x8000,
    VT_ILLEGAL          = 0xffff,
    VT_ILLEGALMASKED    = 0x0fff,
    VT_TYPEMASK         = 0x0fff
};


typedef Variant_type            VARENUM;

//------------------------------------------------------------------------------------------VARIANT

struct VARIANT
{
    void*                       operator new                ( size_t, void* p )                     { return p; }
    void*                       operator new                ( size_t size )                         { return CoTaskMemAlloc( size ); }
    void                        operator delete             ( void* p )                             { CoTaskMemFree( p ); }


    union
    {
        struct
        {
            VARTYPE vt;
            WORD    wReserved1;
            WORD    wReserved2;
            WORD    wReserved3;

            union 
            {
                LONG            lVal;
                BYTE            bVal;
                SHORT           iVal;
                FLOAT           fltVal;
                DOUBLE          dblVal;
                VARIANT_BOOL    boolVal;
              //_VARIANT_BOOL   bool;
                SCODE           scode;
                CY              cyVal;
                DATE            date;
                BSTR            bstrVal;
                IUnknown*       punkVal;
                IDispatch*      pdispVal;
                SAFEARRAY*      parray;
                BYTE*           pbVal;
                SHORT*          piVal;
                LONG*           plVal;
                FLOAT*          pfltVal;
                DOUBLE*         pdblVal;
                VARIANT_BOOL*   pboolVal;
              //_VARIANT_BOOL*  pbool;
                SCODE*          pscode;
                CY*             pcyVal;
                DATE*           pdate;
                BSTR*           pbstrVal;
                IUnknown**      ppunkVal;
                IDispatch**     ppdispVal;
                SAFEARRAY**     pparray;
                VARIANT*        pvarVal;
                void*           byref;
                CHAR            cVal;
                USHORT          uiVal;
                ULONG           ulVal;
                INT             intVal;
                UINT            uintVal;
                DECIMAL*        pdecVal;
                CHAR*           pcVal;
                USHORT*         puiVal;
                ULONG*          pulVal;
                INT*            pintVal;
                UINT*           puintVal;
                zschimmer::int64  llVal;          // long long, Joacim
                zschimmer::uint64 ullVal;         // long long, Joacim
                zschimmer::int64*  pllVal;        // long long, Joacim
                zschimmer::uint64* pullVal;       // long long, Joacim
                DATE            dateVal;
                DATE*           pdateVal;

              //struct  __tagBRECORD
              //{
              //    PVOID            pvRecord;
              //    IRecordInfo     *pRecInfo;
              //}    __VARIANT_NAME_4;
            };
        };

        DECIMAL decVal;
    };
};

#define V_VT( V )           ((V)->vt)
#define V_BYREF( V )        ((V)->byref)

#define V_I2( V )           ((V)->iVal)
#define V_I2REF( V )        ((V)->piVal)
#define V_I4( V )           ((V)->lVal)
#define V_I4REF( V )        ((V)->plVal)
#define V_R4( V )           ((V)->fltVal)
#define V_R4REF( V )        ((V)->pfltVal)
#define V_R8( V )           ((V)->dblVal)
#define V_R8REF( V )        ((V)->pdblVal)
#define V_CY( V )           ((V)->cyVal)
#define V_CYREF( V )        ((V)->pcyVal)
#define V_DATE( V )         ((V)->dateVal)
#define V_DATEREF( V )      ((V)->pdateVal)
#define V_BSTR( V )         ((V)->bstrVal)
#define V_BSTRREF( V )      ((V)->pbstrVal)
#define V_DISPATCH( V )     ((V)->pdispVal)

#define V_DISPATCHREF( V )  ((V)->ppdispVal)

#define V_ERROR( V )        ((V)->scode)
#define V_BOOL( V )         ((V)->boolVal)
#define V_BOOLREF( V )      ((V)->pboolVal)
//#define V_VARIANT( V )      ((V)->pvarVal)
#define V_VARIANTREF( V )   ((V)->pvarVal)
#define V_UNKNOWN( V )      ((V)->punkVal)
#define V_UNKNOWNREF( V )   ((V)->ppunkVal)
//#define V_DECIMAL( V )      ((V)->pdevVal)
#define V_I1( V )           ((V)->cVal)
#define V_I1REF( V )        ((V)->pcVal)
#define V_UI1( V )          ((V)->bVal)
#define V_UI1REF( V )       ((V)->pbVal)
#define V_UI2( V )          ((V)->iVal)
#define V_UI2REF( V )       ((V)->piVal)
#define V_UI4( V )          ((V)->ulVal)
#define V_UI4REF( V )       ((V)->pulVal)
#define V_I8( V )           ((V)->llVal)
#define V_I8REF( V )        ((V)->pllVal)
#define V_UI8( V )          ((V)->ullVal)
#define V_UI8REF( V )       ((V)->pullVal)
#define V_INT( V )          ((V)->lVal)
#define V_INTREF( V )       ((V)->plVal)
#define V_UINT( V )         ((V)->ulVal)
#define V_UINTREF( V )      ((V)->pulVal)
//#define V_VOID( V )
//#define V_HRESULT( V )
//#define V_PTR( V )
#define V_SAFEARRAY( V )    ((V)->parray)

#define V_ARRAY( V )        ((V)->parray)
#define V_ARRAYREF( V )     ((V)->pparray)
//#define V_CARRAY( V )
//#define V_USERDEFINED( V )
//#define V_LPSTR( V )
//#define V_LPWSTR( V )
//#define V_RECORD( V )
//#define V_FILETIME( V )
//#define V_BLOB( V )
//#define V_STREAM( V )
//#define V_STORAGE( V )
//#define V_STREAMED_OBJECT( V )
//#define V_STORED_OBJECT( V )
//#define V_BLOB_OBJECT( V )
//#define V_CF( V )
//#define V_CLSID( V )
#define V_ERROR( V )        ((V)->scode)
#define V_ERRORREF( V )     ((V)->pscode)


inline void             VariantInit         ( VARIANT* v )                  { memset( v, 0, sizeof *v ); }
HRESULT                 VariantClear        ( VARIANT* );
HRESULT                 VariantCopy         ( VARIANT* dest, const VARIANT* src );
HRESULT                 VariantChangeTypeEx ( VARIANT* dest, const VARIANT* src, LCID, USHORT wFlags, VARTYPE );
HRESULT                 VariantChangeType   ( VARIANT* dest, const VARIANT* src, USHORT wFlags, VARTYPE );

// Flags for VariantChangeType/VariantChangeTypeEx
#define VARIANT_NOVALUEPROP      0x01
#define VARIANT_ALPHABOOL        0x02 
#define VARIANT_NOUSEROVERRIDE   0x04 
#define VARIANT_CALENDAR_HIJRI   0x08
#define VARIANT_LOCALBOOL        0x10 

//----------------------------------------------------------------------------------------SAFEARRAY 

struct SAFEARRAYBOUND   // 8 Bytes
{
    ULONG                       cElements;
    LONG                        lLbound;
    LONG                        ubound()        { return lLbound + cElements - 1; }
};


struct SAFEARRAY  // 16+8=24 Bytes
{
    void*                       operator new                ( size_t size )                         { return CoTaskMemAlloc( size ); }
    void                        operator delete             ( void* p )                             { CoTaskMemFree( p ); }

    USHORT                      cDims;
    USHORT                      fFeatures;
    ULONG                       cbElements;
    ULONG                       cLocks;
    void*                       pvData;
    SAFEARRAYBOUND              rgsabound[ 1 ];
};


struct SAFEARRAY_with_vartype : SAFEARRAY   // 24+8=32 Bytes
{
    ULONG                      _reserve1;
    VARTYPE                    _vartype;
    USHORT                     _reserve2;
};


//#define FADF_AUTO          0x1 
//#define FADF_STATIC            0x2 
//#define FADF_EMBEDDED          0x4 
#define FADF_FIXEDSIZE        0x10 
//#define FADF_RECORD           0x20 
//#define FADF_HAVEIID          0x40 
#define FADF_HAVEVARTYPE      0x80 
#define FADF_BSTR            0x100 
//#define FADF_UNKNOWN         0x200 
//#define FADF_DISPATCH        0x400 
#define FADF_VARIANT         0x800 
//#define FADF_RESERVED       0xf008 


SAFEARRAY*                      SafeArrayCreateVector       ( VARTYPE, LONG lLbound, unsigned int cElements );
HRESULT                         SafeArrayDestroy            ( SAFEARRAY* );
HRESULT                         SafeArrayGetVartype         ( SAFEARRAY*, VARTYPE* result );
HRESULT                         SafeArrayGetLBound          ( SAFEARRAY*, unsigned int nDim, LONG* plLbound );
HRESULT                         SafeArrayGetUBound          ( SAFEARRAY*, unsigned int nDim, LONG* plLbound ); 
HRESULT                         SafeArrayAccessData         ( SAFEARRAY*, void** );
HRESULT                         SafeArrayUnaccessData       ( SAFEARRAY* );
//HRESULT                         SafeArrayPtrOfIndex         ( SAFEARRAY*, LONG* rgIndices, void** ppvData );
inline HRESULT                  SafeArrayLock               ( SAFEARRAY* safearray )                { safearray->cLocks++; return S_OK; }
inline HRESULT                  SafeArrayUnlock             ( SAFEARRAY* safearray )                { safearray->cLocks--; return S_OK; }
inline int                      SafeArrayGetDim             ( SAFEARRAY* safearray )                { return safearray->cDims; }
HRESULT                         SafeArrayCopy               ( SAFEARRAY*, SAFEARRAY** );

//---------------------------------------------------------------------------------------SYSTEMTIME

INT                             SystemTimeToVariantTime     ( SYSTEMTIME*, DATE* );
INT                             VariantTimeToSystemTime     ( DATE, SYSTEMTIME* );

//----------------------------------------------------------------------------------------EXCEPINFO 

struct EXCEPINFO 
{
   unsigned short       wCode;              // An error code describing the error.
   unsigned short       wReserved;
   BSTR                 bstrSource;         // Source of the exception.
   BSTR                 bstrDescription;    // Textual description of the error.
   BSTR                 bstrHelpFile;       // Help file path.
   ULONG                dwHelpContext;      // Help context ID.
   void*                pvReserved;         
   HRESULT (*pfnDeferredFillIn)(EXCEPINFO*);// Pointer to function that fills in Help and description info.
   HRESULT              scode;              // ?
   //?RETURN VALUE return value;   // A return value describing the error.
};

//-----------------------------------------------------------------------------------------MULTI_QI

struct MULTI_QI 
{
    const IID*          pIID;
    IUnknown *          pItf;
    HRESULT             hr;
};

//---------------------------------------------------------------------------------------_com_error

struct _com_error               // Zur Verträglichkeit mit Microsofts COM. Wird nicht in Unix nicht benutzt.
{
  //zschimmer::com::Bstr        Description                 () const    { return ""; }
    BSTR                        Description                 () const    { return NULL; }
    DWORD                       Error                       () const    { return (DWORD)-1; }
};

//-------------------------------------------------------------------------------------------COINIT

enum COINIT     // Für CoInitializeEx()
{
    COINIT_MULTITHREADED     = 0x0,
    COINIT_APARTMENTTHREADED = 0x2,
    COINIT_DISABLE_OLE1DDE   = 0x4,
    COINIT_SPEED_OVER_MEMORY = 0x8,
};

//------------------------------------------------------------------------------------------------
typedef int COSERVERINFO;

HRESULT                         CoInitializeEx          ( void*, DWORD );       // s. enum COINIT
HRESULT                         CoInitialize            ( void* );
void                            CoUninitialize          ();

HRESULT                         CoCreateInstance        ( const CLSID&, IUnknown* outer, DWORD dwClsContext, const IID&, void** );
HRESULT                         CoCreateInstanceEx      ( const CLSID&, IUnknown* outer, DWORD context, COSERVERINFO*, ULONG count, MULTI_QI* );

HRESULT                         CoGetClassObject        ( const CLSID&, DWORD dwClsContext, COSERVERINFO*, const IID&, void** );

//HRESULT                         DispInvoke              ( IDispatch*, ITypeInfo*, DISPID, WORD wFlags, DISPPARAMS*, VARIANT* results, EXCEPINFO*, UINT* error_arg_nr );

int                             WideCharToMultiByte     ( UINT CodePage, DWORD dwFlags, 
                                                          const OLECHAR* lpWideCharStr, int cchWideChar,
                                                          char* lpMultiByteStr, int cbMultiByte,  
                                                          const char* lpDefaultChar, BOOL* lpUsedDefaultChar );

int                             MultiByteToWideChar     ( UINT CodePage, DWORD dwFlags, 
                                                          const char* lpMultiByteStr, int cbMultiByte, 
                                                          OLECHAR* lpWideCharStr, int cchWideChar );

WINOLEAPI                       StringFromCLSID         ( REFCLSID, LPOLESTR* );
int                             StringFromGUID2         ( const GUID&, OLECHAR*, int size );
WINOLEAPI                       CLSIDFromString         ( const OLECHAR*, CLSID* );
HRESULT                         CLSIDFromProgID         ( const OLECHAR*, CLSID* );

void                            SetLastError            ( int );
int                             GetLastError            ();

//-------------------------------------------------------------------------------------------------

#endif
#endif
