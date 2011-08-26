// $Id: z_com_server.h 13404 2008-02-12 08:09:25Z jz $

/*
    ZU TUN:

    Templates für IDispatch auf IUnknown zurückführen. Die sind fast gleich implementiert.
*/

#ifndef __Z_COM_SERVER_H
#define __Z_COM_SERVER_H

#include "z_com.h"


#ifdef INTERFACE
#   undef INTERFACE             // Aus C:\Programme\Microsoft Visual Studio .NET 2003\Vc7\PlatformSDK\Include\CommDlg.h(920)
#endif


namespace zschimmer {
namespace com {

struct Com_class_descriptor;

//-------------------------------------------------------------------------------------------------

#define Z_DEFINE_IID( CLASS, IID_STRING, IID_A, IID_B, IID_C, IID_D, IID_E, IID_F, IID_G, IID_H, IID_I, IID_J, IID_K ) \
    Z_DEFINE_GUID( IID_##CLASS, 0x##IID_A, 0x##IID_B, 0x##IID_C, 0x##IID_D, 0x##IID_E, 0x##IID_F, 0x##IID_G, 0x##IID_H, 0x##IID_I, 0x##IID_J, 0x##IID_K ); \
    Z_DEFINE_UUID_OF( CLASS, IID_##CLASS, IID_STRING )

//-------------------------------------------------------------------------------------------------

bool                            com_can_unload_now          ();
HRESULT                         Set_excepinfo               ( const exception&, const string& function );
HRESULT                         Set_excepinfo               ( const _com_error&, const string& );
HRESULT                         Set_excepinfo               ( const char* descr, const string& source );

//---------------------------------------------------------------------------------------Com_method
// Com_method brauchen wir eigentlich nur für Unix.

#if !defined _MSC_VER  //||  _MSC_VER >= 1400      // Visual Studio .Net 2003 (7.1) kennt __VA_ARGS__ nicht. Visual Studio 2005 kann mit leerem __VA_ARGS__ nicht umgehen

#   define COM_METHOD_PTR( INTERFACE )  HRESULT (WINAPI INTERFACE::*)()

#   define COM_METHOD_( FLAGS, CLASS, DISPID, NAME, METHOD_PTR, RESULT_TYPE, DEFAULT_ARG_COUNT, ... ) \
        { FLAGS, DISPID, NAME, (Com_method_ptr)(COM_METHOD_PTR(CLASS::Idispatch_implementation))&METHOD_PTR, RESULT_TYPE, { __VA_ARGS__ }, DEFAULT_ARG_COUNT }

#   define COM_METHOD( CLASS, DISPID, NAME, RESULT_TYPE, DEFAULT_ARG_COUNT, ... ) \
        COM_METHOD_( DISPATCH_METHOD     , CLASS, DISPID, #NAME, CLASS::NAME      , RESULT_TYPE, DEFAULT_ARG_COUNT, __VA_ARGS__ )

#   define COM_PROPERTY_PUT( CLASS, DISPID, NAME, DEFAULT_ARG_COUNT, ... ) \
        COM_METHOD_( DISPATCH_PROPERTYPUT, CLASS, DISPID, #NAME, CLASS::put_##NAME, VT_EMPTY   , DEFAULT_ARG_COUNT, __VA_ARGS__ )

#   define COM_PROPERTY_GET( CLASS, DISPID, NAME, RESULT_TYPE, DEFAULT_ARG_COUNT, ... ) \
        COM_METHOD_( DISPATCH_PROPERTYGET, CLASS, DISPID, #NAME, CLASS::get_##NAME, RESULT_TYPE, DEFAULT_ARG_COUNT, __VA_ARGS__ )

#endif


const int max_com_method_params = 10;

typedef HRESULT (WINAPI IDispatch::*Com_method_ptr)();


struct Com_method
{
/*
    struct Type
    {
        Type( VARTYPE vt ) : _vartype( vt ), _default( 0 ) {}
        VARTYPE                _vartype;
        int                    _default;
    };
*/

    int                        _flags;              // Z.B. DISPATCH_METHOD oder DISPATCH_PROPERTYGET
    DISPID                     _dispid;             // irgendwas > 0 (im Allgemeinen),  für jeden Methodennamen eindeutig
    const char*                _name;               // Name der Methode
    Com_method_ptr             _method;             // Methode
    Variant_type               _result_type;
    VARTYPE                    _types[ max_com_method_params ];          // Typen der Parameter
  //Type                       _types[ max_com_method_params ];          // Typen der Parameter
    int                        _default_arg_count;  // Anzahl Default-Parameter von rechts
};

//--------------------------------------------------------------------------------------Typelib_ref
// Nur static!

struct Typelib_ref
{
                                Typelib_ref                 ()                                      {}  // Statisch _class_descriptor_list initialisieren, damit Com_class_descriptor initialisierten Typelib_ref benutzen kann.
    virtual                    ~Typelib_ref                 ()                                      {}

    void                    set_hinstance                   ( HINSTANCE hinstance )                 { _hinstance = hinstance; }

    HRESULT                     Get_class_object            ( const CLSID&, const IID&, void** result );

    HRESULT                     Register_server             ();
    HRESULT                     Unregister_server           ();

    HRESULT                     Load_typelib                ();
    ITypeLib*                   typelib                     () const                                { return _typelib; }


  private:
    friend struct               Com_class_descriptor;


  //Fill_zero                  _zero_;                      
    GUID                       _typelib_id;
    string                     _version_string;
    HINSTANCE                  _hinstance;
    Com_class_descriptor*      _class_descriptor_list;     // Liste aller Klassenbeschreibungen (über _next verkettet), statisch initialisieren!
    ptr<ITypeLib>              _typelib;

#   ifndef Z_COM
        TLIBATTR               _libattr;
#   endif
};

//-----------------------------------------------------------------------------Com_class_descriptor
// Nur für static Objekte!

typedef HRESULT              (* Com_class_factory_creator )( const CLSID&, const IID&, IUnknown** );    // Für perl_scripting_engine.cxx


struct Com_class_descriptor : Non_cloneable
{
    enum Creatable { non_creatable, creatable };

                                Com_class_descriptor        ( Typelib_ref*, const CLSID&, const IID&, const char* name, Creatable = non_creatable, const Com_method* = NULL );
                              //Com_class_descriptor        ( Com_class_factory_creator, const CLSID&, const IID& );         // Für perl_scripting_engine.cxx
    virtual                    ~Com_class_descriptor        ()                                      {}

    void                        Register_class              ();
    HRESULT                     Unregister_class            ();

    HRESULT                     Load_typeinfo               ();
    HRESULT                     GetIDsOfNames               ( const IID&, OLECHAR** rgszNames, UINT cNames, LCID, DISPID* );
    HRESULT                     Invoke                      ( IDispatch*, DISPID, const IID&, LCID, unsigned short wFlags, DISPPARAMS*, VARIANT* pVarResult, EXCEPINFO*, UINT* puArgErr );

  //void                        add_to_typelib              ( Typelib_ref* );

    const CLSID&                clsid                       () const                                { return _clsid; }
    const IID&                  iid                         () const                                { return _iid; }
    const string&               name                        () const                                { return _name; }
    const string&               title                       () const                                { return _title; }
    ITypeInfo*                  typeinfo                    () const                                { return _typeinfo; }
    const Typelib_ref*          typelib_ref                 () const                                { return _typelib_ref; }

    HRESULT                     Create_instance             ( IUnknown* outer, const IID&, void** ) const;
    virtual void                create                      ( void** ) const                        = 0;

    void                        for_linkage                 ();

    friend HRESULT              Get_class_object            ( const CLSID& clsid, const IID& iid, void** result );

  private:
    friend struct               Typelib_ref;

    Fill_zero                  _zero_;
    string const               _name;
    IID const                  _iid;
    CLSID const                _clsid;
    const Com_method* const    _methods;
    const Com_class_descriptor*_next;                       // Verkettung aller Klassenbeschreibungen
    Typelib_ref* const         _typelib_ref;
    string                     _title;
    bool                       _creatable;
    ptr<ITypeInfo>             _typeinfo;
  //Com_class_factory_creator  _creator;                    // Für perl_scripting_engine.cxx
};

//-----------------------------------------------------------------creatable_com_class_descriptor<>

template< class CLASS, class INTERFACE >
struct creatable_com_class_descriptor : Com_class_descriptor
{
    creatable_com_class_descriptor( Typelib_ref* typelib, const char* name, const Com_method* idispatch_methods = NULL ) 
    : 
        Com_class_descriptor( typelib, clsid_of( (CLASS*)NULL ), __uuidof(INTERFACE), name, creatable, idispatch_methods ) 
    {}


    void create( void** result ) const
    { 
        ptr<CLASS> p = Z_NEW( CLASS() );
        *result = p.take();
    }
};

//---------------------------------------------------------------------------com_class_descriptor<>

template< class INTERFACE >
struct com_class_descriptor : Com_class_descriptor
{
    com_class_descriptor( Typelib_ref* typelib, const char* name, const Com_method* idispatch_methods = NULL ) 
    : 
        Com_class_descriptor( typelib, CLSID_NULL, __uuidof(INTERFACE), name, non_creatable, idispatch_methods ) 
    {}


    void create( void** result ) const
    { 
        *result = NULL; 
    }
};

//-----------------------------------------------------------------------iunknown_implementation_<>

template< class CLASS, class INTERFACE, class CLASS_DESCRIPTOR_CLASS >
struct iunknown_implementation_ : simple_iunknown_implementation< INTERFACE >                                  
{
    typedef iunknown_implementation_    Iunknown_implementation_;
    typedef CLASS_DESCRIPTOR_CLASS      Class_descriptor; 


                                iunknown_implementation_    ( Com_class_descriptor* c )             : _class_descriptor( c )  {}

    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )
    {
        Z_IMPLEMENT_QUERY_INTERFACE( this, iid, INTERFACE, result );
        Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IUnknown , result );
        return E_NOINTERFACE;
    }

  private:
    Com_class_descriptor*      _class_descriptor;
};

//------------------------------------------------------------------------iunknown_implementation<>

template< class CLASS, class INTERFACE >
struct iunknown_implementation : iunknown_implementation_< CLASS, INTERFACE, com_class_descriptor<INTERFACE> > 
{
    typedef iunknown_implementation     Iunknown_implementation;

    
    iunknown_implementation( Com_class_descriptor* c = NULL )
    : 
        iunknown_implementation::Iunknown_implementation_( c ) 
    {}
};

//--------------------------------------------------------------creatable_iunknown_implementation<>

template< const CLSID& clsid, class CLASS, class INTERFACE >
struct creatable_iunknown_implementation : iunknown_implementation_< CLASS, INTERFACE, creatable_com_class_descriptor<CLASS,INTERFACE> >
{
    typedef creatable_iunknown_implementation                                       Iunknown_implementation;
    typedef typename creatable_iunknown_implementation::Iunknown_implementation_    Iunknown_implementation_;


    creatable_iunknown_implementation( Com_class_descriptor* c )
    : 
        Iunknown_implementation_( c ) 
    {}


    friend inline const CLSID&  clsid_of                    ( Iunknown_implementation_* )           { return clsid; }
};

//------------------------------------------------------------------idispatch_base_implementation<>

template< class IDISPATCH_INTERFACE >
struct idispatch_base_implementation : simple_iunknown_implementation< IDISPATCH_INTERFACE >,
                                       ISupportErrorInfo
{
    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return simple_iunknown_implementation<IDISPATCH_INTERFACE>::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return simple_iunknown_implementation<IDISPATCH_INTERFACE>::Release(); }
    
    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )
    {
        Z_IMPLEMENT_QUERY_INTERFACE( static_cast<IDISPATCH_INTERFACE*>( this ), iid, IDISPATCH_INTERFACE, result );
        Z_IMPLEMENT_QUERY_INTERFACE( static_cast<IDISPATCH_INTERFACE*>( this ), iid, IDispatch          , result );
        Z_IMPLEMENT_QUERY_INTERFACE( static_cast<IDISPATCH_INTERFACE*>( this ), iid, IUnknown           , result );
        Z_IMPLEMENT_QUERY_INTERFACE( static_cast<ISupportErrorInfo*  >( this ), iid, ISupportErrorInfo  , result );
        return E_NOINTERFACE;
      //return IDISPATCH_INTERFACE::QueryInterface( iid, result );
    }

    STDMETHODIMP                GetTypeInfoCount        ( UINT* result )                            { *result = 0;  return S_OK; }
    STDMETHODIMP                GetTypeInfo             ( UINT, LCID, ITypeInfo** )                 { return E_NOTIMPL; }

    STDMETHODIMP                InterfaceSupportsErrorInfo  ( const IID& )                          { return S_OK; }
};

//--------------------------------------------------------------------Idispatch_base_implementation

typedef idispatch_base_implementation< IDispatch >   Idispatch_base_implementation;

//--------------------------------------------------------------idispatch_standard_implementation<>

template< class BASE_INTERFACE >
struct idispatch_standard_implementation : idispatch_base_implementation< BASE_INTERFACE >
{
    typedef idispatch_standard_implementation   Idispatch_standard_implementation;



    idispatch_standard_implementation( Com_class_descriptor* c, IDispatch* d ) 
    : 
        _com_class_descriptor(c), 
        _idispatch(d) 
    {}


    STDMETHODIMP                GetIDsOfNames               ( const IID& iid, OLECHAR** names, UINT n, LCID lcid, DISPID* result ) 
    { 
        return _com_class_descriptor->GetIDsOfNames( iid, names, n, lcid, result ); 
    }


    STDMETHODIMP                Invoke                      ( DISPID dispid, const IID& iid, LCID lcid, unsigned short flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO* x, UINT* e ) 
    { 
        return _com_class_descriptor->Invoke( _idispatch, dispid, iid, lcid, flags, params, result, x, e ); 
    }

    void                        operator =                  ( const idispatch_standard_implementation& )    {}  // _idispatch und _com_class_descriptor belassen


  private:
                                idispatch_standard_implementation( const idispatch_standard_implementation& );  // Copy-Konstruktor nicht implementiert wegen _idispatch


    Com_class_descriptor*      _com_class_descriptor;
    IDispatch*                 _idispatch;
};

//----------------------------------------------------------------------idispatch_implementation_<>

template< class CLASS, class IDISPATCH_INTERFACE, class CLASS_DESCRIPTOR_CLASS >
struct idispatch_implementation_ : idispatch_standard_implementation< IDISPATCH_INTERFACE >
{
    typedef idispatch_implementation_                                               Idispatch_implementation_;
    typedef typename idispatch_implementation_::Idispatch_standard_implementation   Idispatch_standard_implementation;
    typedef CLASS_DESCRIPTOR_CLASS                                                  Class_descriptor;



                                idispatch_implementation_   ( Com_class_descriptor* c )             : Idispatch_standard_implementation( c, static_cast<IDISPATCH_INTERFACE*>( this ) ) {}


    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_standard_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_standard_implementation::Release(); }

    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )
    {
        //Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDISPATCH_INTERFACE, result );
        return idispatch_standard_implementation< IDISPATCH_INTERFACE >::QueryInterface( iid, result );
    }
    
    STDMETHODIMP                GetTypeInfoCount            ( UINT* pctInfo )                       { return Idispatch_standard_implementation::GetTypeInfoCount( pctInfo ); }
    STDMETHODIMP                GetTypeInfo                 ( UINT i, LCID l, ITypeInfo** result )  { return Idispatch_standard_implementation::GetTypeInfo( i, l, result ); }


    STDMETHODIMP                GetIDsOfNames               ( const IID& iid, OLECHAR** names, UINT names_count, LCID lcid, DISPID* results ) 
    { 
        return Idispatch_standard_implementation::GetIDsOfNames( iid, names, names_count, lcid, results ); 
    }


    STDMETHODIMP                Invoke                      ( DISPID dispid, const IID& iid, LCID lcid, unsigned short flags, DISPPARAMS* dispparams, 
                                                              VARIANT* result, EXCEPINFO* excepinfo, UINT* error_arg_nr )
    {
        return Idispatch_standard_implementation::Invoke( dispid, iid, lcid, flags, dispparams, result, excepinfo, error_arg_nr );
    }
};

//-----------------------------------------------------------------------idispatch_implementation<>

template< class CLASS, class IDISPATCH_INTERFACE >
struct idispatch_implementation : idispatch_implementation_< CLASS, IDISPATCH_INTERFACE, com_class_descriptor<IDISPATCH_INTERFACE> >
{
    typedef idispatch_implementation                                        Idispatch_implementation;
    typedef typename Idispatch_implementation::Idispatch_implementation_    Idispatch_implementation_;


    idispatch_implementation( Com_class_descriptor* c )
    : 
        Idispatch_implementation_( c ) 
    {}
};

//--------------------------------------------------------------creatable_idispatch_implementation<>

template< const CLSID& clsid, class CLASS, class IDISPATCH_INTERFACE >
struct creatable_idispatch_implementation : idispatch_implementation_< CLASS, IDISPATCH_INTERFACE, creatable_com_class_descriptor<CLASS,IDISPATCH_INTERFACE> >
{
    typedef creatable_idispatch_implementation  Creatable_idispatch_implementation;
    typedef typename creatable_idispatch_implementation::Idispatch_implementation_ Idispatch_implementation_;

    
    creatable_idispatch_implementation( Com_class_descriptor* c )
    : 
        Idispatch_implementation_( c ) 
    {
    }


    friend inline const CLSID&  clsid_of                    ( Idispatch_implementation_* )           { return clsid; }
};

//-------------------------------------------------------------------------------------------------

#define Z_COM_IMPLEMENT( STATEMENT )                                                                \
{                                                                                                   \
    HRESULT hr = S_OK;                                                                              \
    try { STATEMENT; }                                                                              \
    catch( const exception&  x )  { hr = zschimmer::com::Set_excepinfo( x, Z_FUNCTION ); }        \
    return hr;                                                                                      \
}

//--------------------------------------------------------------------------------Com_class_factory

struct Com_class_factory : Object,   // iunknown_implementation< CLSID_ClassFactory, Com_class_factory, IClassFactory >
                           IClassFactory
{
                                Com_class_factory           ( const Com_class_descriptor* c )       : _class_descriptor( c ) {}

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }

    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )
    {
        Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IClassFactory, result );
        return Object::QueryInterface( iid, result );
    }

    STDMETHODIMP                CreateInstance              ( IUnknown* outer, const IID&, void** result );
    STDMETHODIMP                LockServer                  ( BOOL lock );

  private:
    const Com_class_descriptor* _class_descriptor;
};

//----------------------------------------------------------------------------------Module_interface

struct Module_interface : creatable_iunknown_implementation< CLSID_Module_interface2, Module_interface, Imodule_interface2 >
{
    static Class_descriptor     class_descriptor;

                                Module_interface            ()                                      : Iunknown_implementation( &class_descriptor ) {}

    STDMETHODIMP                module_interface_version    ()                                      { return 3; }
    STDMETHODIMP         putref_Com_context                 ( const Com_context* );
    STDMETHODIMP            put_Log_categories              ( const BSTR );
    STDMETHODIMP            put_Log_context                 ( Log_context** );
  //STDMETHODIMP                Set_stream_and_system_mutex ( ostream**, System_mutex* );
};

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer

#endif
