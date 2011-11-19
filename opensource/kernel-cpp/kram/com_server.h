// $Id: com_server.h 13404 2008-02-12 08:09:25Z jz $

#ifndef __SOS_COM_SERVER
#define __SOS_COM_SERVER


#include <string>
#include "com.h"
#include "sos.h"
#include "sosalloc.h"
#include "sosappl.h"

#include "../zschimmer/com_server.h"


namespace sos {

struct Sos_ole_object;

extern zschimmer::long32        com_object_count;
extern zschimmer::long32        com_lock;
extern bool                     hostole_log_all;
//extern Xc*                      com_server_last_xc;

struct Ole_factory;
struct Ole_server;
struct Ole_class_descr;
struct Dyn_obj;

//--------------------------------------------------------------------Hostole_ISupportErrorInfo

struct Hostole_ISupportErrorInfo : ISupportErrorInfo
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostole_ISupportErrorInfo" ); }
    void                        operator delete         ( void* ptr, size_t )                     { sos_free( ptr ); }

                                Hostole_ISupportErrorInfo( Sos_ole_object* );
    virtual                    ~Hostole_ISupportErrorInfo();

    //IUnknown members that delegate to m_pUnkOuter.
    STDMETHODIMP                QueryInterface          ( const IID&, void** );
    STDMETHODIMP_( ULONG )      AddRef                  ();
    STDMETHODIMP_( ULONG )      Release                 ();

    //ISupportErrorInfo members
    STDMETHODIMP                InterfaceSupportsErrorInfo( const IID& );

  private:
    zschimmer::long32          _ref_count;
    Sos_ole_object*            _object;
    //? IUnknown*                   m_pUnkOuter;
};

//--------------------------------------------------------------------------------Typelib_descr

struct Typelib_descr //: Sos_self_deleting
{
                                Typelib_descr           ( const IID& typelib_id, const char* name, const char* version );
    virtual                    ~Typelib_descr           ();

    void                        init                    ();

#ifdef SYSTEM_HAS_COM
    void                        register_typelib        ();
    void                        unregister_typelib      ();

    HRESULT                     register_server         ();
    HRESULT                     unregister_server       ();

    void                        load_typelib            ();
#endif

    Fill_zero                  _zero_;
    bool                       _initialized;
    IID                        _typelib_id;
    string                     _name;
    string                     _version;
    string                     _typelib_filename;

#ifdef SYSTEM_HAS_COM
    ITypeLib*                  _typelib;
#endif
};

//-------------------------------------------------------------------------------Sos_ole_object

struct Sos_ole_object //: Sos_self_deleting  -- Sind IUnknown und Sos_self_deleting zu vereinigen, Mehrfachvererbung oder Sos_self_deleting : IUnknown ?
{
/*
    struct Interface_ptr
    {
        IID                    _iid;
        IUnknown*              _iunknown;
    };
*/

                                Sos_ole_object          ( Ole_class_descr*, IUnknown* this_, IUnknown* outer = NULL );
    virtual                    ~Sos_ole_object          ();

    void*                       operator new            ( size_t, void* ptr )                   { return ptr; }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }

    void*                       operator new            ( size_t size, const char* info = "Sos_ole_object" )  { return sos_alloc( size, info ); }
    void                        operator delete         ( void* ptr, const char*                           )  { sos_free( ptr ); }

    virtual Sos_ole_object&     operator =              ( const Sos_ole_object& );

  //void                        com_add_interface       ( const IID&, IUnknown* );

    virtual void               _obj_print               ( ostream* s ) const;

    //Non-delegating object IUnknown:
    virtual STDMETHODIMP_( ULONG )  AddRef              ();
    virtual STDMETHODIMP_( ULONG )  Release             ();
    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    STDMETHODIMP                GetTypeInfoCount        ( UINT* pctInfo );
    STDMETHODIMP                GetTypeInfo             ( UINT itinfo, LCID, ITypeInfo** );


    STDMETHODIMP                GetIDsOfNames           ( const IID&, OLECHAR** rgszNames, UINT cNames,
                                                          LCID, DISPID* );
    void                        log_GetIDsOfNames       ( const IID& riid, OLECHAR** rgszNames, UINT cNames,
                                                          LCID /*lcid*/ );
    STDMETHODIMP                Invoke                  ( DISPID, const IID&, LCID,
                                                          unsigned short wFlags, DISPPARAMS*,
                                                          VARIANT* pVarResult, EXCEPINFO*,
                                                          UINT* puArgErr );
    void                        log_invoke              ( DISPID, const IID&, LCID,
                                                          unsigned short wFlags, DISPPARAMS* );

    IUnknown*                   iunknown                ()  { return _iunknown; }

    void                        fill_excepinfo          ( const Xc& x, EXCEPINFO* );
    void                        fill_excepinfo          ( const exception& x, EXCEPINFO* );
    void                        fill_excepinfo          ( const _com_error& x, EXCEPINFO* );
    HRESULT                    _set_excepinfo           ( const char* source, const char* descr, const string& method );
    HRESULT                    _set_excepinfo           ( const Xc&, const string& method );
    HRESULT                    _set_excepinfo           ( const exception&, const string& method );
    HRESULT                    _set_excepinfo           ( const _com_error&, const string& method );


    Fill_zero                  _zero_;
    zschimmer::long32          _ref_count;
    IUnknown*                  _iunknown;               // Zeiger auf (IUnknown*)this, wegen Mehrfachvererbung != (Sos_ole_object*)this


    Hostole_ISupportErrorInfo  _supporterrorinfo;

  //typedef list<Interface_ptr> Interfaces;
  //Interfaces                 _additional_interfaces;
    Ole_class_descr*           _ole_class_descr;
    IUnknown*                  _pUnkOuter;              //Controlling unknown

  //CImpIProvideClassInfo*     _pImpIProvideCI;
    string                     _class_name;
    IID                        _last_queried_interface; // Zur Optimierung des Logs

  //IConnectionPointContainer* _connection_point_container;

  private:
                                Sos_ole_object          ( const Sos_ole_object& );   // Unterklasse muss ihren eigenen Copy-Kontruktur schreiben! (s. Hostware_dynobj)
};

ostream& operator << ( ostream& s, const Sos_ole_object& o ) ;

//----------------------------------------------------------------USE_SOS_OLE_OBJECT_QUERYINTERFACE

#define USE_SOS_OLE_OBJECT_QUERYINTERFACE \
                                                                                                                                            \
    STDMETHODIMP QueryInterface( const IID& refiid , void** ppObj )                                                                         \
    {                                                                                                                                       \
        return Sos_ole_object::QueryInterface( refiid, ppObj );                                                                             \
    }                                                                                                                                       

//----------------------------------------------------------------USE_SOS_OLE_OBJECT_ADDREF_RELEASE

#define USE_SOS_OLE_OBJECT_ADDREF_RELEASE \
                                                                                                                                            \
    STDMETHODIMP_( ULONG ) AddRef()                                                                                                         \
    {                                                                                                                                       \
        return Sos_ole_object::AddRef();                                                                                                    \
    }                                                                                                                                       \
                                                                                                                                            \
    STDMETHODIMP_( ULONG ) Release()                                                                                                        \
    {                                                                                                                                       \
        ULONG ref_count = Sos_ole_object::Release();                                                                                        \
        if( ref_count == 0 )  delete this;                                                                                                  \
        return ref_count;                                                                                                                   \
    }

//-------------------------------------------------------------------USE_SOS_OLE_OBJECT_GETTYPEINFO

#define USE_SOS_OLE_OBJECT_GETTYPEINFO \
                                                                                                                                            \
    STDMETHODIMP GetTypeInfoCount( UINT* pctInfo )                                                                                          \
    {                                                                                                                                       \
        return Sos_ole_object::GetTypeInfoCount( pctInfo );                                                                                 \
    }                                                                                                                                       \
                                                                                                                                            \
    STDMETHODIMP GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** itypeinfo )                                                               \
    {                                                                                                                                       \
        return Sos_ole_object::GetTypeInfo( itinfo, lcid, itypeinfo );                                                                      \
    }                                                                                                                                       

//------------------------------------------------------------------------USE_SOS_OLE_OBJECT_INVOKE

#ifdef SYSTEM_HAS_COM

# define USE_SOS_OLE_OBJECT_INVOKE \
                                                                                                                                            \
    STDMETHODIMP GetIDsOfNames( const IID& refiid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* dispid )                            \
    {                                                                                                                                       \
        return Sos_ole_object::GetIDsOfNames( refiid, rgszNames, cNames, lcid, dispid );                                                    \
    }                                                                                                                                       \
                                                                                                                                            \
    STDMETHODIMP Invoke( DISPID dispid, const IID& refiid, LCID lcid, unsigned short wFlags,                                                \
                         DISPPARAMS* dispparams, VARIANT* pVarResult, EXCEPINFO* excepinfo, UINT* puArgErr )                                \
    {                                                                                                                                       \
        return Sos_ole_object::Invoke( dispid, refiid, lcid, wFlags, dispparams, pVarResult, excepinfo, puArgErr );                         \
    }

#else

/*
# define USE_SOS_OLE_OBJECT_INVOKE \
    STDMETHODIMP GetIDsOfNames( const IID& refiid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* dispid );                           \
    STDMETHODIMP Invoke( DISPID dispid, const IID& refiid, LCID lcid, unsigned short wFlags,                                                \
                         DISPPARAMS* dispparams, VARIANT* pVarResult, EXCEPINFO* excepinfo, UINT* puArgErr );                                
*/

# define USE_SOS_OLE_OBJECT_INVOKE Z_DEFINE_GETIDSOFNAMES_AND_INVOKE

#endif

//--------------------------------------------------------------------USE_SOS_OLE_OBJECT_WITHOUT_QI

#define USE_SOS_OLE_OBJECT_WITHOUT_QI   \
    USE_SOS_OLE_OBJECT_ADDREF_RELEASE   \
    USE_SOS_OLE_OBJECT_GETTYPEINFO      \
    USE_SOS_OLE_OBJECT_INVOKE           

//-------------------------------------------------------------------------------USE_SOS_OLE_OBJECT

#define USE_SOS_OLE_OBJECT              \
    USE_SOS_OLE_OBJECT_WITHOUT_QI       \
    USE_SOS_OLE_OBJECT_QUERYINTERFACE   

//-------------------------------------------------------------------------------SOS_OLE_MEMBER_xxx

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

//------------------------------------------------------------------------------Ole_class_descr

struct Ole_class_descr 
{
                                Ole_class_descr         ( Typelib_descr*, const CLSID& clsid, const IID&, const char* name, const char* version );
    virtual                    ~Ole_class_descr         ();

#ifdef SYSTEM_HAS_COM
    void                        register_class          ();
    HRESULT                     unregister_class        ();

    void                        register_class_object   ( IUnknown* );
    HRESULT                     unregister_class_object ();

    HRESULT                     load_typeinfo           ();
#endif



    virtual HRESULT             create_instance         ( IUnknown* pUnkOuter, const IID&, IUnknown** );
    virtual HRESULT             create_simple           ( IUnknown** );
    void                        dummy_call              ();

    static Ole_class_descr*     head;                   // Liste aller Klassenbeschreibungen (über _next verkettet)

    Fill_zero                  _zero_;
    Ole_class_descr*           _next;                   // Verkettung aller Klassenbeschreibungen
    Typelib_descr*             _typelib_descr;
    Bool                       _creatable;
    CLSID                      _clsid;
    IID                        _iid;                    // Nur ein Interface
    Sos_string                 _name;
    Sos_string                 _version;



#ifdef SYSTEM_HAS_COM
    ITypeInfo*                 _typeinfo;

  //IProvideClassInfo*         _provide_class_info;
  //IConnectionPointContainer* _connection_point_container;
#endif



    Ole_factory*               _class_factory;          // Eine IClassFactory für .exe (CoRegisterClassObject)

  private:
                                Ole_class_descr         ( const Ole_class_descr& );             // icke implementerad
    Ole_class_descr&            operator =              ( const Ole_class_descr& );             // icke implementerad
};

inline ostream& operator << ( ostream& s, const Ole_class_descr& cls ) { s << "Ole_class_descr(" << cls._name << ')'; return s; }


//-----------------------------------------------------------------------------------DESCRIBE_CLASS

#define _DESCRIBE_CLASS( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION, EXTRA )        \
struct UNAME##_class : Ole_class_descr                                              \
{                                                                                   \
    UNAME##_class()                                                                 \
    :                                                                               \
        Ole_class_descr( TYPELIB_PTR, CLSID, IID_I##LNAME, REGNAME, VERSION )       \
    {                                                                               \
        _creatable = true;                                                          \
    }                                                                               \
                                                                                    \
    EXTRA                                                                           \
                                                                                    \
}                                                                                   \
LNAME##_class;                                                                      \
Ole_class_descr* LNAME##_class_ptr = &LNAME##_class;

//-----------------------------------------------------------------------------------DESCRIBE_CLASS

#define DESCRIBE_CLASS( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION )        \
    _DESCRIBE_CLASS( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION, enum _dc_dummy_{}; )

//-------------------------------------------------------------------------DESCRIBE_CLASS_CREATABLE

#define DESCRIBE_CLASS_CREATABLE( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION )  \
    _DESCRIBE_CLASS( TYPELIB_PTR, UNAME, LNAME, CLSID, REGNAME, VERSION,                \
                                                                                        \
        HRESULT create_simple( IUnknown** result )                                      \
        {                                                                               \
            *result = ( new UNAME() )->_iunknown;                                       \
            return S_OK;                                                                \
        }                                                                               \
    )

//---------------------------------------------------------------------------------------Ole_server

struct Ole_server
{
                                Ole_server              ( Typelib_descr* );
    virtual                    ~Ole_server              ();

    void                        init                    ();
    void                        exit                    ();
    void                        message_loop            ();

    Fill_zero                  _zero_;
    Sos_appl                   _appl;                   // Sos_static
    bool                       _initialized;
    Typelib_descr*             _typelib; 
};

//--------------------------------------------------------------------------------------Ole_factory

struct Ole_factory : IClassFactory,
                     Sos_ole_object
{
                                Ole_factory             ();
    virtual                    ~Ole_factory             ();

    USE_SOS_OLE_OBJECT_ADDREF_RELEASE

    void                       _obj_print               ( ostream* s ) const                { *s << "Ole_factory"; }

    // IUnknown:
    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    // IClassFactory:
    STDMETHODIMP                CreateInstance          ( LPUNKNOWN, const IID&, void** );
    STDMETHODIMP                LockServer              ( BOOL );

    STDMETHODIMP                set_clsid               ( const CLSID& );
    void                        register_factory        ();
    HRESULT                     unregister_factory      ();

  protected:
  //Ole_server*                _ole_server;
    Ole_class_descr*           _class;
    DWORD                      _register_id;            // Kennung von CoRegisterClassObject (bei .exe)
};

//-----------------------------------------------------------------------------------------Ole_appl

struct Ole_appl : Ole_server
{
                                Ole_appl                ( Typelib_descr* );
                               ~Ole_appl                ();

    void                        init                    ();
    void                        exit                    ();

  protected:
    Fill_zero                  _zero_;
    Bool                       _initialized;
};

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
