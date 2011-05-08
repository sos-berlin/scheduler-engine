// olestd.h                            (C)1997 SOS GmbH Berlin
#ifndef __OLESTD_H
#define __OLESTD_H

/*
 * This #define tells <bookguid.h> to not define GUIDs that will
 * be defined in MKTYPLIB-generated header files, like ibeeper.h.
 */
//#define GUIDS_FROM_TYPELIB

//#define INC_AUTOMATION
//#define INC_CONTROLS


#include "com_simple_standards.h"

#include <vector>
#include <windows.h>
#include <ole2.h>
#include <comdef.h>

#include <oleauto.h>
#include <unknwn.h>     // IUnknown
#include <ocidl.h>      // IConnectionPointContainer etc.


#if !defined _WIN32  &&  defined INITGUIDS
#   include <initguid.h>
#endif

#ifdef WIN32
#   define WIN32ANSI
#   include <tchar.h>
#   ifdef UNICODE
#       include <wchar.h>
#   endif
#endif

#ifndef TEXT
#   define TEXT(a) a
#endif

#ifdef WIN32ANSI
#   define OLETEXT(x)  L ## x
#else
#   define OLETEXT(x)  TEXT(x)
#endif

#ifndef APIENTRY
#   define APIENTRY __export FAR PASCAL
#endif 

#ifndef SETMESSAGEQUEUE
#   define SETMESSAGEQUEUE
#endif

//#ifndef lstrcpyA
//#   define lstrcpyA lstrcpy
//#endif
//
//#ifndef lstrcmpiA
//#   define lstrcmpiA lstrcmpi
//#endif

#include "sosalloc.h"

class  CImpIProvideClassInfo;
//typedef class CImpIProvideClassInfo *PCImpIProvideClassInfo;
//typedef void (*PFNDESTROYED)();

namespace sos
{

struct Sos_ole_object;
struct Ole_factory;
struct Connection_point_container;

//---------------------------------------------------------------------------------------------

struct Ole_server;
struct Dyn_obj;

ostream& operator << ( ostream&, const _com_error& );

//----------------------------------------------------------------------------Sos_ole_type_info
/*
struct Sos_ole_type_info
{
                                Sos_ole_type_info       ( const IID* type_lib_id ) : _zero_(this+1), _type_lib_id ( type_lib_id ) {}
                               ~Sos_ole_type_info       ();


    uint                       _ref_count;
    const IID*                 _type_lib_id;
};
*/

ostream& operator << ( ostream& s, const Sos_ole_object& o ) ;

#define SOS_OLE_MEMBER_BOOL( NAME ) \
    STDMETHODIMP_( HRESULT )    put_##NAME             ( short  o )                 { _##NAME = o != 0; return NOERROR; }   \
    STDMETHODIMP_( HRESULT )    get_##NAME             ( short* o )                 { *o = _##NAME; return NOERROR; }  \
    void                        NAME                   ( bool  o )                  { _##NAME = o; }   \
    Bool                        NAME                   () const                     { return _##NAME;  }  \
    Bool                       _##NAME;

#define SOS_OLE_MEMBER_STRING( NAME ) \
    STDMETHODIMP_( HRESULT )    put_##NAME             ( BSTR  o )                  { _##NAME = o; return NOERROR; }   \
    STDMETHODIMP_( HRESULT )    get_##NAME             ( BSTR* o )                  { *o = SysAllocString( _##NAME ); return NOERROR; }  \
    void                        NAME                   ( const Sos_string& o )      { _##NAME = o; }   \
    const Sos_string&           NAME                   () const                     { return _##NAME;  }  \
    Sos_string                 _##NAME;

#define SOS_OLE_MEMBER_STRING_NULL( NAME ) \
    STDMETHODIMP_( HRESULT )    put_##NAME             ( VARIANT* o )               { _##NAME##_null = V_VT( o ) == VT_NULL; if( _##NAME##_null ) { _##NAME = empty_string; return NOERROR; }  return variant_to_string( *o, &_##NAME ); }   \
    STDMETHODIMP_( HRESULT )    get_##NAME             ( VARIANT* o )               { VariantInit( o ); if( _##NAME##_null ) { V_VT( o ) = VT_NULL; return NOERROR; }  V_VT( o ) = VT_BSTR; V_BSTR( o ) = SysAllocString( _##NAME ); return NOERROR; } \
    void                        NAME                   ( const Sos_string& o )      { _##NAME = o; _##NAME##_null = false; }   \
    const Sos_string&           NAME                   () const                     { return _##NAME; }  \
    void                        NAME##_null            ( Bool o )                   { _##NAME##_null = o; }   \
    Bool                        NAME##_null            () const                     { return _##NAME##_null;  }  \
    Sos_string                 _##NAME; \
    Bool                       _##NAME##_null;


//-------------------------------------------------------------------------DESCRIBE_CLASS_CREATABLE
/*
#define DESCRIBE_CLASS_CREATABLE( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION )  \
    _DESCRIBE_CLASS( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION,                \
                                                                                        \
        HRESULT create_simple( IUnknown** result )                                      \
        {                                                                               \
            *result = static_cast<Sos_ole_object*>( new UNAME() );                      \
            return result? NOERROR : (HRESULT)E_OUTOFMEMORY;                            \
        }                                                                               \
    )
*/
//-------------------------------------------------------------------------------------------------
/*
class CImpIProvideClassInfo : public IProvideClassInfo
{
    public:
        ULONG           m_cRef;     //For debugging

    private:
        LPUNKNOWN       m_pUnkOuter;
        ITypeInfo      *m_pITI;     //To return from GetClassInfo

    public:
        CImpIProvideClassInfo(LPUNKNOWN, ITypeLib *);
        ~CImpIProvideClassInfo(void);

        //IUnknown members that delegate to m_pUnkOuter.
        STDMETHODIMP         QueryInterface( REFIID, void** );
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IProvideClassInfo members
        STDMETHODIMP GetClassInfo(ITypeInfo **);

};
*/

//---------------------------------------------------------------------------------------------

// _bstr_t (für #import)
inline string                   as_string               ( const _bstr_t& bstr )                     { return bstr_as_string( bstr ); }
inline string                   as_string               ( const _variant_t& variant )               { return variant_as_string(variant); }

       bool                     as_bool                 ( const VARIANT& );
inline bool                     as_bool                 ( const _variant_t& v )                     { return as_bool( (const VARIANT&)v ); }
inline bool                     as_bool                 ( const Variant& v )                        { return as_bool( (const VARIANT&)v ); }

       double                   as_double               ( const VARIANT& );
inline double                   as_double               ( const _variant_t& v )                     { return as_double( (const VARIANT&)v ); }
inline double                   as_double               ( const Variant& v )                        { return as_double( (const VARIANT&)v ); }

inline _bstr_t                  as_bstr_t               ( const string& str )                       { return _bstr_t( SysAllocString_string(str), false ); }
inline _bstr_t                  as_bstr_t               ( const char* str )                         { return _bstr_t(str); }
       _bstr_t                  as_bstr_t               ( const Big_int );
       _bstr_t                  as_bstr_t               ( const Ubig_int );
       _bstr_t                  as_bstr_t               ( const int );
       _bstr_t                  as_bstr_t               ( const uint );

       bool                     operator ==             ( const _bstr_t&, const string& );
       bool                     operator ==             ( const _bstr_t&, const char* );
inline bool                     operator !=             ( const _bstr_t& a, const string& b )       { return !( a == b ); }
inline bool                     operator !=             ( const _bstr_t& a, const char* b )         { return !( a == b ); }


       bool                     com_param_given         ( VARIANT* param );


} //namespace sos

#endif
