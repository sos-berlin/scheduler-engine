// $Id: spooler_com.cxx,v 1.111 2003/10/02 21:40:00 jz Exp $
/*
    Hier sind implementiert

    Com_log                     COM-Hülle für Task_log
    Com_object_set              COM-Hülle für Object_set
    Com_task                    COM-Hülle für Task
    Com_spooler                 COM-Hülle für Spooler
*/


#include "spooler.h"
//#include "../hostole/hostole.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/z_com_server.h"


using namespace zschimmer::com;


namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

Typelib_descr spooler_typelib ( LIBID_spooler_com, "Spooler", "1.0" );

DESCRIBE_CLASS( &spooler_typelib, Com_error         , error         , CLSID_error         , "Spooler.Error"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable      , variable      , CLSID_Variable      , "Spooler.Variable"      , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable_set  , variable_set  , CLSID_Variable_set  , "Spooler.Variable_set"  , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable_set_enumerator, variable_set_enumerator, CLSID_Variable_set_enumerator, "Spooler.Com_variable_set_enumerator", "1.0" );
DESCRIBE_CLASS( &spooler_typelib, Com_log           , log           , CLSID_log           , "Spooler.Log"           , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job           , job           , CLSID_job           , "Spooler.Job"           , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_task          , task          , CLSID_Task          , "Spooler.Task"          , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_object_set    , object_set    , CLSID_object_set    , "Spooler.Object_set"    , "1.0" )
//DESCRIBE_CLASS( &spooler_typelib, Com_thread        , thread        , CLSID_thread        , "Spooler.Thread"        , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler       , spooler       , CLSID_spooler       , "Spooler.Spooler"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler_context, spooler_context, CLSID_Spooler_context, "Spooler.Context"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job_chain     , job_chain     , CLSID_job_chain     , "Spooler.Job_chain"     , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job_chain_node, job_chain_node, CLSID_job_chain_node, "Spooler.Job_chain_node", "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_order         , order         , CLSID_order         , "Spooler.Order"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_order_queue   , order_queue   , CLSID_order_queue   , "Spooler.Order_queue"   , "1.0" )

//-----------------------------------------------------------------------------IID_Ihostware_dynobj

#ifdef Z_WINDOWS
    extern "C" const GUID IID_Ihostware_dynobj = { 0x9F716A02, 0xD1F0, 0x11CF, { 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
#else
    DEFINE_GUID( IID_Ihostware_dynobj, 0x9F716A02, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
#endif

//--------------------------------------------------------------------------------time_from_variant

Time time_from_variant( const VARIANT& vt )
{
    return time::time_from_string( variant_as_string( vt ) );
}

//----------------------------------------------------------------------order_from_order_or_payload

static ptr<spooler_com::Iorder> order_from_order_or_payload( Spooler* spooler, const VARIANT& order_or_payload )
{
    ptr<spooler_com::Iorder> iorder;

    if( order_or_payload.vt == VT_DISPATCH  ||  order_or_payload.vt == VT_UNKNOWN )
    {
        if( V_UNKNOWN(&order_or_payload) )
        {
            HRESULT hr = V_UNKNOWN(&order_or_payload)->QueryInterface( spooler_com::IID_Iorder, iorder.void_pp() );
            if( FAILED(hr) )  iorder = NULL;
        }
    }

    if( !iorder )  iorder = new Order( spooler, order_or_payload );

    return iorder;
}

//------------------------------------------------------------------------------Com_error::_methods
#ifdef Z_COM

const Com_method Com_error::_methods[] =
{ 
   // _flags              , _name             , _method                                        , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYGET, 1, "java_class_name" , (Com_method_ptr)&Com_error::get_java_class_name, VT_BSTR },
    { DISPATCH_PROPERTYGET, 2, "is_error"        , (Com_method_ptr)&Com_error::get_is_error       , VT_BOOL },
    { DISPATCH_PROPERTYGET, 3, "code"            , (Com_method_ptr)&Com_error::get_code           , VT_BSTR }, 
    { DISPATCH_PROPERTYGET, 4, "text"            , (Com_method_ptr)&Com_error::get_text           , VT_BSTR }, 
    {}
};

#endif
//-----------------------------------------------------------------------------Com_error::Com_error

Com_error::Com_error( const Xc_copy& x )
: 
    Sos_ole_object( error_class_ptr, (Ierror*)this ),
    _xc(x) 
{
}

//------------------------------------------------------------------------Com_error::QueryInterface

STDMETHODIMP Com_error::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//---------------------------------------------------------------------------------Com_error::close

void Com_error::close()
{ 
    THREAD_LOCK( _lock )
    {
        _xc = NULL; 
    }
}

//------------------------------------------------------------------------------Com_error::is_error

STDMETHODIMP Com_error::get_is_error( VARIANT_BOOL* result )
{
    THREAD_LOCK( _lock )
    {
        *result = _xc != NULL;
    }

    return NOERROR;
}

//----------------------------------------------------------------------------------Com_error::code

STDMETHODIMP Com_error::get_code( BSTR* code_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_xc )  *code_bstr = NULL;
              else  *code_bstr = SysAllocStringLen_char( _xc->code(), strlen( _xc->code() ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Error::code" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Error::code" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_error::text

STDMETHODIMP Com_error::get_text( BSTR* text_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_xc )  *text_bstr = NULL;
              else  *text_bstr = SysAllocString_string( _xc->what() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Error::text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Error::text" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_variable::_methods
#ifdef Z_COM

const Com_method Com_variable::_methods[] =
{ 
   // _flags         , dispid, _name                , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT, 0, "value"              , (Com_method_ptr)&Com_variable::put_value          , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 0, "value"              , (Com_method_ptr)&Com_variable::get_value          , VT_VARIANT    },
    { DISPATCH_PROPERTYGET, 1, "name"               , (Com_method_ptr)&Com_variable::get_name           , VT_BSTR       },
    { DISPATCH_METHOD     , 2, "Clone"              , (Com_method_ptr)&Com_variable::Clone              , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 3, "java_class_name"    , (Com_method_ptr)&Com_variable::get_java_class_name, VT_BSTR },
    {}
};

#endif
//-----------------------------------------------------------------------Com_variable::Com_variable

Com_variable::Com_variable( const BSTR name, const VARIANT& value )
:
    Sos_ole_object( variable_class_ptr, (Ivariable*)this )
{
    THREAD_LOCK( _lock )
    {
        _name = name;
        _value = value;
    }
}

//---------------------------------------------------------------------Com_variable::QueryInterface

STDMETHODIMP Com_variable::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------------Com_variable::Clone

STDMETHODIMP Com_variable::Clone( Ivariable** result ) 
{ 
    HRESULT hr = NOERROR; 
    
    THREAD_LOCK(_lock)
    {
        *result = new Com_variable(_name,_value); 
        (*result)->AddRef();
    }

    return hr;
}

//-----------------------------------------------------------------------Com_variable_set::_methods
#ifdef Z_COM

const Com_method Com_variable_set::_methods[] =
{ 
   // _flags         , dispid, _name                , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "set_var"            , (Com_method_ptr)&Com_variable_set::set_var        , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 0, "value"              , (Com_method_ptr)&Com_variable_set::put_var        , VT_EMPTY      , { VT_BYREF|VT_VARIANT, VT_BYREF|VT_VARIANT }, 1 },
    { DISPATCH_PROPERTYGET, 0, "value"              , (Com_method_ptr)&Com_variable_set::get_var        , VT_VARIANT    , { VT_BYREF|VT_VARIANT                      }, 1 },
    { DISPATCH_PROPERTYPUT, 8, "var"                , (Com_method_ptr)&Com_variable_set::put_var        , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 8, "var"                , (Com_method_ptr)&Com_variable_set::get_var        , VT_VARIANT    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 2, "count"              , (Com_method_ptr)&Com_variable_set::get_count      , VT_I4         },
    { DISPATCH_PROPERTYGET, 3, "dom"                , (Com_method_ptr)&Com_variable_set::get_dom        , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 4, "Clone"              , (Com_method_ptr)&Com_variable_set::Clone          , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 5, "merge"              , (Com_method_ptr)&Com_variable_set::merge          , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYGET, DISPID_NEWENUM, "_NewEnum", (Com_method_ptr)&Com_variable_set::get__NewEnum , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUT, 6, "xml"                , (Com_method_ptr)&Com_variable_set::put_xml        , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 6, "xml"                , (Com_method_ptr)&Com_variable_set::get_xml        , VT_BSTR       },
    { DISPATCH_PROPERTYGET, 7, "java_class_name"    , (Com_method_ptr)&Com_variable_set::get_java_class_name, VT_BSTR },
    {}
};

#endif
//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set()
:
    Sos_ole_object( variable_set_class_ptr, (Ivariable_set*)this )
{
}

//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set( const Com_variable_set& o )
:
    Sos_ole_object( variable_set_class_ptr, (Ivariable_set*)this )
{
    THREAD_LOCK( _lock )
    {
        for( Map::const_iterator it = o._map.begin(); it != o._map.end(); it++ )
        {
            Com_variable* v = it->second;
            if( v )
            {
                ptr<Com_variable> clone;

                v->Clone( (Ivariable**)&clone );
                _map[ it->first ] = clone;
            }
        }
    }
}

//-----------------------------------------------------------------Com_variable_set::QueryInterface

STDMETHODIMP Com_variable_set::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------Com_variable_set::set_dom

void Com_variable_set::set_dom( const xml::Element_ptr& params )
{
    HRESULT hr;

    THREAD_LOCK( _lock )
    {
        DOM_FOR_EACH_ELEMENT( params, e )
        {
            if( e.nodeName_is( "param" ) ) 
            {
                Bstr    name  = e.getAttribute( "name" );
                Variant value = e.getAttribute( "value" );

                hr = put_var( name, &value );                       if( FAILED(hr) )  throw_ole( hr, "Ivariable_set::put_var" );
            }
        }
    }
}

//----------------------------------------------------------------------Com_variable_set::put_value

STDMETHODIMP Com_variable_set::put_value( VARIANT* name, VARIANT* value )
{
    if( name->vt == VT_BSTR )
    {
        return put_var( V_BSTR(name), value );
    }
    else
    if( name->vt == VT_ERROR )
    {
        if( value->vt != VT_BSTR )  return DISP_E_TYPEMISMATCH;
        
        return put_xml( V_BSTR(value) );
    }
    else 
        return DISP_E_TYPEMISMATCH;
}

//----------------------------------------------------------------------Com_variable_set::get_value

STDMETHODIMP Com_variable_set::get_value( VARIANT* name, VARIANT* value )
{
    if( name->vt == VT_BSTR )
    {
        return get_var( V_BSTR(name), value );
    }
    else
    if( name->vt == VT_ERROR )
    {
        value->vt      = VT_BSTR;
        value->bstrVal = NULL;

        return get_xml( &V_BSTR(value) );
    }
    else 
        return DISP_E_TYPEMISMATCH;
}

//------------------------------------------------------------------------Com_variable_set::put_var

STDMETHODIMP Com_variable_set::put_var( BSTR name, VARIANT* value )
{
    THREAD_LOCK( _lock )  
    {
        Bstr lname = name;

        bstr_to_lower( &lname );

        Map::iterator it = _map.find( lname );
        if( it != _map.end()  &&  it->second )
        {
            it->second->put_value( value );
        }
        else
        {
            ptr<Com_variable> v = new Com_variable( name, *value );
            _map[lname] = v;
        }
    }

    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_var

STDMETHODIMP Com_variable_set::get_var( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )  
    {
        VariantInit( value );

        Bstr lname = name;
        bstr_to_lower( &lname );

        Map::iterator it = _map.find( lname );
        if( it != _map.end()  &&  it->second )
        {
            hr = it->second->get_value( value );
            if( !FAILED(hr))  hr = S_OK;
        }
    }

    return hr;
}

//----------------------------------------------------------------------Com_variable_set::get_count

STDMETHODIMP Com_variable_set::get_count( int* result )
{
    THREAD_LOCK( _lock )  *result = _map.size();
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_dom

STDMETHODIMP Com_variable_set::get_dom( msxml::IXMLDOMDocument** doc )  
{ 
#   ifdef SPOOLER_HAS_MSXML
        
        HRESULT hr = NOERROR;

        try
        {
            *doc = dom()._ptr; 
            (*doc)->AddRef();
        }
        catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::dom" ); }
        catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::dom" ); }

#    else

        return E_NOTIMPL;

#   endif
}

//------------------------------------------------------------------------Com_variable_set::get_dom

xml::Document_ptr Com_variable_set::dom()
{
    xml::Document_ptr doc;
    
    THREAD_LOCK( _lock )
    {
        //xml::Document_ptr doc; // = msxml::Document_ptr( __uuidof(spooler_com::DOMDocument30), NULL );
        doc.create();
        doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );

        xml::Element_ptr varset = doc.createElement( xml_element_name() );
        doc.appendChild( varset );

        for( Map::iterator it = _map.begin(); it != _map.end(); it++ )
        {
            Com_variable* v = it->second;
            if( v )
            {
                Bstr    name;
                Variant value;

                v->get_name( &name );
                v->get_value( &value );

                xml::Element_ptr var = doc.createElement( "variable" );
                var.setAttribute( "name" , string_from_bstr(name) );

                VARTYPE vt = (VARTYPE)-1;

                if( value.vt == VT_EMPTY
                 || value.vt == VT_NULL
                 || value.vt == VT_I2
                 || value.vt == VT_I4
                 || value.vt == VT_R4       
                 || value.vt == VT_R8       
                 || value.vt == VT_CY       
                 || value.vt == VT_DATE     
              // || value.vt == VT_BSTR          // VT_BSTR müssen wir nicht besonders kennzeichnen. Das ist Default.
              // || value.vt == VT_DISPATCH 
                 || value.vt == VT_ERROR    
                 || value.vt == VT_BOOL     
              // || value.vt == VT_VARIANT  
              // || value.vt == VT_UNKNOWN  
                 || value.vt == VT_DECIMAL  
                 || value.vt == VT_I1       
                 || value.vt == VT_UI1      
                 || value.vt == VT_UI2      
                 || value.vt == VT_UI4      
                 || value.vt == VT_I8       
                 || value.vt == VT_UI8      
                 || value.vt == VT_INT      
                 || value.vt == VT_UINT     
              // || value.vt == VT_VOID     
                 || value.vt == VT_HRESULT )
                {
                    vt = value.vt; 
                }
/*
                else
                if( value.vt == VT_DISPATCH )
                {
                    ptr<Variable_set> v;
                    if( V_DISPATCH(value)
                     && SUCCEEDED( V_DISPATCH(value)->QueryInterface( IID_Ivariable_set, v.pp() ) ) )  vt = value.vt;
                }
*/
                if( vt != (VARTYPE)-1 )  var.setAttribute( "vt", vt );
                                   else  {} // Andere Typen sind nicht rückkonvertierbar. Die werden dann zum String.  

                var.setAttribute( "value", string_from_variant( value ) );

                varset.appendChild( var );
            }
        }
    }

    return doc; //doc.detach();
}

//--------------------------------------------------------------------------Com_variable_set::Clone

STDMETHODIMP Com_variable_set::Clone( Ivariable_set** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        *result = NULL;

        ptr<Com_variable_set> clone = new Com_variable_set( *this );

        *result = clone;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::Clone" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::Clone" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_variable_set::merge

STDMETHODIMP Com_variable_set::merge( Ivariable_set* other )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        Com_variable_set* o = dynamic_cast<Com_variable_set*>( other );
        if( !o )  return E_POINTER;

        for( Map::iterator it = o->_map.begin(); it != o->_map.end(); it++ )
        {
            if( it->second )
            {
                ptr<Com_variable> v;
                hr = it->second->Clone( (Ivariable**)&v );
                _map[ it->first ] = v;
            }
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::merge" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::merge" ); }

    return hr;
}

//-------------------------------------------------------------------Com_variable_set::get__NewEnum

STDMETHODIMP Com_variable_set::get__NewEnum( IUnknown** iunknown )
{
    ptr<Com_variable_set_enumerator> e = new Com_variable_set_enumerator;
    e->initialize( this );

    *iunknown = e;
    (*iunknown)->AddRef();
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::put_xml

STDMETHODIMP Com_variable_set::put_xml( BSTR xml_text )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        xml::Document_ptr doc = dom();
        doc.load_xml( string_from_bstr( xml_text ) );

        DOM_FOR_EACH_ELEMENT( doc.documentElement(), e )
        {
            if( e.nodeName() == "variable" || e.nodeName() == "param" )
            {
                Bstr    name  = e.getAttribute( "name" );
                Variant value = e.getAttribute( "value" );
                VARTYPE vt    = e.int_getAttribute( "vt", VT_BSTR );
                value.change_type( vt );

                hr = put_var( name, &value );
                if( FAILED( hr ) )  break;
            }
            else
                throw_xc( "SPOOLER-182" );
        }

    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }

    return hr;
}

//------------------------------------------------------------------------Com_variable_set::get_xml

STDMETHODIMP Com_variable_set::get_xml( BSTR* xml_doc  )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        hr = string_to_bstr( dom().xml(), xml_doc );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }

    return hr;
}

//------------------------------------------------------------Com_variable_set_enumerator::_methods
#ifdef Z_COM

const Com_method Com_variable_set_enumerator::_methods[] =
{ 
   // _flags         , dispid, _name    , _method                                               , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "Next"   , (Com_method_ptr)&Com_variable_set_enumerator::Next    , VT_LONG       , { VT_LONG, VT_BYREF|VT_VARIANT } },
    { DISPATCH_METHOD     , 2, "Skip"   , (Com_method_ptr)&Com_variable_set_enumerator::Skip    , VT_EMPTY      , { VT_LONG } },
    { DISPATCH_METHOD     , 3, "Reset"  , (Com_method_ptr)&Com_variable_set_enumerator::Reset   },
    { DISPATCH_METHOD     , 4, "Clone"  , (Com_method_ptr)&Com_variable_set_enumerator::Clone   , VT_DISPATCH  },
    {}
};

#endif
//-----------------------------------------Com_variable_set_enumerator::Com_variable_set_enumerator

Com_variable_set_enumerator::Com_variable_set_enumerator()
:
    Sos_ole_object( variable_set_enumerator_class_ptr, this )
{
}

//------------------------------------------------------Com_variable_set_enumerator::QueryInterface

STDMETHODIMP Com_variable_set_enumerator::QueryInterface( REFIID iid, void** obj )
{                                                                    
    if( iid == IID_IEnumVARIANT
     || iid == IID_Ivariable_set_enumerator )
    {
        *obj = this;
        AddRef();
        return NOERROR;
    }

    return Sos_ole_object::QueryInterface( iid, obj );
}                                                                                                                                       

//----------------------------------------------------------Com_variable_set_enumerator::initialize

void Com_variable_set_enumerator::initialize( Com_variable_set* v )
{
    _variable_set = v;
    Reset();
}

//----------------------------------------------------------------Com_variable_set_enumerator::Next

STDMETHODIMP Com_variable_set_enumerator::Next( unsigned long celt, VARIANT* result, unsigned long* pceltFetched )
{
    int i = 0;

    while( i < celt  &&  _iterator != _variable_set->_map.end() )
    {
        VariantInit( result );

        if( _iterator->second )
        {
            Com_variable* v = NULL;
            _iterator->second.copy_to( &v );
            result->vt = VT_DISPATCH;
            result->pdispVal = v;
        }

        _iterator++;
        result++;
        i++;
    }

    if( pceltFetched )  *pceltFetched = i;

    return i < celt? S_FALSE : S_OK;
}

//----------------------------------------------------------------Com_variable_set_enumerator::Skip

STDMETHODIMP Com_variable_set_enumerator::Skip( unsigned long celt )
{
    while( celt &&  _iterator != _variable_set->_map.end() )  _iterator++;

    return celt? S_FALSE : S_OK;
}

//---------------------------------------------------------------Com_variable_set_enumerator::Reset

STDMETHODIMP Com_variable_set_enumerator::Reset()
{
    _iterator = _variable_set->_map.begin();
    return NOERROR;
}

//---------------------------------------------------------------Com_variable_set_enumerator::Clone

STDMETHODIMP Com_variable_set_enumerator::Clone( IEnumVARIANT** ppenum )
{
    return ERROR;
}

//--------------------------------------------------------------------------------Com_log::_methods
#ifdef Z_COM

const Com_method Com_log::_methods[] =
{ 
   // _flags         , dispid, _name                   , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     ,  1, "debug9"               , (Com_method_ptr)&Com_log::debug9                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  2, "debug8"               , (Com_method_ptr)&Com_log::debug8                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  3, "debug7"               , (Com_method_ptr)&Com_log::debug7                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  4, "debug6"               , (Com_method_ptr)&Com_log::debug6                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  5, "debug5"               , (Com_method_ptr)&Com_log::debug5                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  6, "debug4"               , (Com_method_ptr)&Com_log::debug4                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  7, "debug3"               , (Com_method_ptr)&Com_log::debug3                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  8, "debug2"               , (Com_method_ptr)&Com_log::debug2                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  9, "debug1"               , (Com_method_ptr)&Com_log::debug1                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 10, "debug"                , (Com_method_ptr)&Com_log::debug                   , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  0, "info"                 , (Com_method_ptr)&Com_log::info                    , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 11, "msg"                  , (Com_method_ptr)&Com_log::msg                     , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 12, "warn"                 , (Com_method_ptr)&Com_log::warn                    , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 13, "error"                , (Com_method_ptr)&Com_log::error                   , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 14, "log"                  , (Com_method_ptr)&Com_log::log                     , VT_EMPTY      , { VT_I4, VT_BSTR } },
    { DISPATCH_PROPERTYGET, 15, "mail"                 , (Com_method_ptr)&Com_log::get_mail                , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT, 16, "mail_on_error"        , (Com_method_ptr)&Com_log::put_mail_on_error       , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 16, "mail_on_error"        , (Com_method_ptr)&Com_log::get_mail_on_error       , VT_BOOL       },
    { DISPATCH_PROPERTYPUT, 17, "mail_on_success"      , (Com_method_ptr)&Com_log::put_mail_on_success     , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 17, "mail_on_success"      , (Com_method_ptr)&Com_log::get_mail_on_success     , VT_BOOL       },
    { DISPATCH_PROPERTYPUT, 18, "mail_on_process"      , (Com_method_ptr)&Com_log::put_mail_on_process     , VT_EMPTY      , { VT_I4 } },
    { DISPATCH_PROPERTYGET, 18, "mail_on_process"      , (Com_method_ptr)&Com_log::get_mail_on_process     , VT_I4         },
    { DISPATCH_PROPERTYPUT, 19, "level"                , (Com_method_ptr)&Com_log::put_level               , VT_EMPTY      , { VT_I4 } },
    { DISPATCH_PROPERTYGET, 19, "level"                , (Com_method_ptr)&Com_log::get_level               , VT_I4         },
    { DISPATCH_PROPERTYGET, 20, "filename"             , (Com_method_ptr)&Com_log::get_filename            , VT_BSTR       },
    { DISPATCH_PROPERTYPUT, 21, "collect_within"       , (Com_method_ptr)&Com_log::put_collect_within      , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 21, "collect_within"       , (Com_method_ptr)&Com_log::get_collect_within      , VT_R8         },
    { DISPATCH_PROPERTYPUT, 22, "collect_max"          , (Com_method_ptr)&Com_log::put_collect_max         , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 22, "collect_max"          , (Com_method_ptr)&Com_log::get_collect_max         , VT_R8         },
    { DISPATCH_PROPERTYPUT, 23, "mail_it"              , (Com_method_ptr)&Com_log::put_mail_it             , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 24, "java_class_name"      , (Com_method_ptr)&Com_log::get_java_class_name     , VT_BSTR },
    {}
};

#endif
//---------------------------------------------------------------------------------Com_log::Com_log

Com_log::Com_log( Prefix_log* log )
:
    Sos_ole_object( log_class_ptr, (Ilog*)this ),
    _zero_(this+1),
    _log(log)
{ 
}

//--------------------------------------------------------------------------Com_log::QueryInterface

STDMETHODIMP Com_log::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//--------------------------------------------------------------------------------Com_log::set_task

void Com_log::set_log( Prefix_log* log )
{ 
    THREAD_LOCK( _lock )
    {
        _log = log; 
    }
}

//----------------------------------------------------------------------------------------Com_log::
    
STDMETHODIMP Com_log::debug9( BSTR line )                       { return log( spooler_com::log_debug9, line ); }
STDMETHODIMP Com_log::debug8( BSTR line )                       { return log( spooler_com::log_debug8, line ); }
STDMETHODIMP Com_log::debug7( BSTR line )                       { return log( spooler_com::log_debug7, line ); }
STDMETHODIMP Com_log::debug6( BSTR line )                       { return log( spooler_com::log_debug6, line ); }
STDMETHODIMP Com_log::debug5( BSTR line )                       { return log( spooler_com::log_debug5, line ); }
STDMETHODIMP Com_log::debug4( BSTR line )                       { return log( spooler_com::log_debug4, line ); }
STDMETHODIMP Com_log::debug3( BSTR line )                       { return log( spooler_com::log_debug3, line ); }
STDMETHODIMP Com_log::debug2( BSTR line )                       { return log( spooler_com::log_debug2, line ); }
STDMETHODIMP Com_log::debug1( BSTR line )                       { return log( spooler_com::log_debug1, line ); }
STDMETHODIMP Com_log::debug ( BSTR line )                       { return log( spooler_com::log_debug1, line ); }
STDMETHODIMP Com_log::msg   ( BSTR line )                       { return log( spooler_com::log_info  , line ); }
STDMETHODIMP Com_log::info  ( BSTR line )                       { return log( spooler_com::log_info  , line ); }
STDMETHODIMP Com_log::warn  ( BSTR line )                       { return log( spooler_com::log_warn  , line ); }
STDMETHODIMP Com_log::error ( BSTR line )                       { return log( spooler_com::log_error , line ); }

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::log( spooler_com::Log_level level, BSTR line )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->log( (zschimmer::Log_level)level, bstr_as_string( line ) ); 
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log" ); }

    return hr;
}

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::get_mail( Imail** mail )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *mail = _log->imail();
        if( *mail )  (*mail)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_log::put_mail_on_error

STDMETHODIMP Com_log::put_mail_on_error( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_error( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_log::get_mail_on_error

STDMETHODIMP Com_log::get_mail_on_error( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_error();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }

    return hr;
}

    
//----------------------------------------------------------------------Com_log::put_mail_on_success

STDMETHODIMP Com_log::put_mail_on_success( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_success( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }

    return hr;
}

//---------------------------------------------------------------------Com_log::get_mail_on_success

STDMETHODIMP Com_log::get_mail_on_success( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_success();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }

    return hr;
}

//----------------------------------------------------------------------Com_log::put_mail_on_process

STDMETHODIMP Com_log::put_mail_on_process( int level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_process( level == -1? 1 : level );        // True (-1) -> 1
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }

    return hr;
}

//---------------------------------------------------------------------Com_log::get_mail_on_process

STDMETHODIMP Com_log::get_mail_on_process( int* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *result = _log->mail_on_process();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_log::level

STDMETHODIMP Com_log::put_level( int level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_log_level( level );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_log::level

STDMETHODIMP Com_log::get_level( int* level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *level = _log->log_level();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_log::filename

STDMETHODIMP Com_log::get_filename( BSTR* filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *filename_bstr = SysAllocString_string( _log->filename() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::filename" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_log::new_filename

STDMETHODIMP Com_log::put_new_filename( BSTR filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;
        _log->set_new_filename( bstr_as_string( filename_bstr ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_log::new_filename

STDMETHODIMP Com_log::get_new_filename( BSTR* filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *filename_bstr = SysAllocString_string( _log->new_filename() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_log::collect_within

STDMETHODIMP Com_log::put_collect_within( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_collect_within( time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_log::collect_within

STDMETHODIMP Com_log::get_collect_within( double* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *result = _log->collect_within();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_log::collect_max

STDMETHODIMP Com_log::put_collect_max( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_collect_max( time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_log::collect_max

STDMETHODIMP Com_log::get_collect_max( double* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *result = _log->collect_max();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_log::put_mail_it

STDMETHODIMP Com_log::put_mail_it( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_it( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_it" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_it" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_object_set::_methods
#ifdef Z_COM

const Com_method Com_object_set::_methods[] =
{ 
   // _flags         , dispid, _name                   , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "low_level"            , (Com_method_ptr)&Com_object_set::get_low_level    , VT_I4         },
    { DISPATCH_PROPERTYGET,  2, "high_level"           , (Com_method_ptr)&Com_object_set::get_high_level   , VT_I4         },
    {}
};

#endif
//-------------------------------------------------------------------Com_object_set::Com_object_set

Com_object_set::Com_object_set( Object_set* object_set )
:
    Sos_ole_object( object_set_class_ptr, this ),
    _object_set(object_set)
{
}

//--------------------------------------------------------------------Com_object_set::get_low_level

STDMETHODIMP Com_object_set::get_low_level( int* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_object_set )  return E_POINTER;
        //if( current_thread_id() != _object_set->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = _object_set->_object_set_descr->_level_interval._low_level;
    }

    return NOERROR;
}

//-------------------------------------------------------------------Com_object_set::get_high_level

STDMETHODIMP Com_object_set::get_high_level( int* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_object_set )  return E_POINTER;
      //if( current_thread_id() != _object_set->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = _object_set->_object_set_descr->_level_interval._high_level;
    }
    return NOERROR;
}

//--------------------------------------------------------------------------------Com_job::_methods
#ifdef Z_COM

const Com_method Com_job::_methods[] =
{ 
   // _flags         , dispid, _name                            , _method                                               , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     ,  1, "start_when_directory_changed"  , (Com_method_ptr)&Com_job::start_when_directory_changed, VT_EMPTY      , { VT_BSTR, VT_BSTR }, 1 },
    { DISPATCH_METHOD     ,  2, "clear_when_directory_changed"  , (Com_method_ptr)&Com_job::clear_when_directory_changed },
    { DISPATCH_METHOD     ,  3, "start"                         , (Com_method_ptr)&Com_job::start                       , VT_DISPATCH   , { VT_BYREF|VT_VARIANT }, 1 },
  //{ DISPATCH_PROPERTYGET,  4, "thread"                        , (Com_method_ptr)&Com_job::get_thread                  , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET,  5, "include_path"                  , (Com_method_ptr)&Com_job::get_include_path            , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  6, "name"                          , (Com_method_ptr)&Com_job::get_name                    , VT_BSTR       },
    { DISPATCH_METHOD     ,  7, "wake"                          , (Com_method_ptr)&Com_job::wake                        },
    { DISPATCH_PROPERTYPUT,  8, "state_text"                    , (Com_method_ptr)&Com_job::put_state_text              , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  9, "title"                         , (Com_method_ptr)&Com_job::get_title                   , VT_BSTR       },
    { DISPATCH_PROPERTYPUT, 10, "delay_after_error"             , (Com_method_ptr)&Com_job::put_delay_after_error       , VT_EMPTY      , { VT_I4, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 11, "order_queue"                   , (Com_method_ptr)&Com_job::get_order_queue             , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 12, "java_class_name"               , (Com_method_ptr)&Com_job::get_java_class_name         , VT_BSTR },
    { DISPATCH_PROPERTYPUT, 13, "delay_order_after_setback"     , (Com_method_ptr)&Com_job::put_delay_order_after_setback,VT_EMPTY      , { VT_I4, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 14, "max_order_setbacks"            , (Com_method_ptr)&Com_job::put_max_order_setbacks      , VT_EMPTY      , { VT_I4 } },
    {}
};

#endif
//---------------------------------------------------------------------------------Com_job::Com_job

Com_job::Com_job( Job* job )
:
    Sos_ole_object( job_class_ptr, (Ijob*)this ),
    _job(job)
{
}

//--------------------------------------------------------------------------Com_job::QueryInterface

STDMETHODIMP Com_job::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------Com_job::start_when_directory_changed 

STDMETHODIMP Com_job::start_when_directory_changed( BSTR directory_name, BSTR filename_pattern )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;

        _job->start_when_directory_changed( bstr_as_string( directory_name ), bstr_as_string( filename_pattern ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }

    return hr;
}

//------------------------------------------------------------Com_job::clear_when_directory_changed 

STDMETHODIMP Com_job::clear_when_directory_changed()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;
        _job->clear_when_directory_changed();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::clear_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::clear_when_directory_changed" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_job::start

STDMETHODIMP Com_job::start( VARIANT* params, Itask** itask )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;

        Sos_ptr<Task>       task;

        ptr<Ivariable_set>  pars;
        Time                start_at = 0; 

        if( params  &&  params->vt != VT_EMPTY  &&  params->vt != VT_NULL  &&  params->vt != VT_ERROR )
        {
            if( params->vt != VT_DISPATCH && params->vt != VT_UNKNOWN )  return DISP_E_TYPEMISMATCH;
            hr = params->punkVal->QueryInterface( IID_Ivariable_set, pars.void_pp() );
            if( FAILED(hr) )  return hr;
        }

        Variant task_name_vt;
        if( pars )  pars->get_var( Bstr("spooler_task_name"), &task_name_vt );
        hr = task_name_vt.ChangeType( VT_BSTR );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_task_name" );

        Variant start_after_vt;
        if( pars )  pars->get_var( Bstr("spooler_start_after"), &start_after_vt );
        if( start_after_vt.vt != VT_EMPTY )
        {
            hr = start_after_vt.ChangeType( VT_R8 );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_start_after" );
            start_at = Time::now() + start_after_vt.dblVal;
        }

        THREAD_LOCK( _job->_lock )
        {
            string name = bstr_as_string( task_name_vt.bstrVal );
            task = _job->start_without_lock( pars, name, start_at, true );
        }

        *itask = new Com_task( task );
        (*itask)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }

    return hr;
}

//------------------------------------------------------------------------------------Com_job::wake

STDMETHODIMP Com_job::wake()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;
        _job->set_state_cmd( Job::sc_wake );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }

    return hr;
}

//------------------------------------------------------------------------------Com_job::get_thread
/*
STDMETHODIMP Com_job::get_thread( Ithread** thread )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;

        *thread = _job->_thread->_com_thread;
        if( *thread )  (*thread)->AddRef();
    }

    return hr;
}
*/
//------------------------------------------------------------------------Com_job::get_include_path

STDMETHODIMP Com_job::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;
      //if( current_thread_id() != _job->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _job->_spooler->include_path() );
    }

    return NOERROR;
}

//--------------------------------------------------------------------------------Com_job::get_name

STDMETHODIMP Com_job::get_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;
        //if( current_thread_id() != _job->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _job->_name );
    }

    return NOERROR;
}

//--------------------------------------------------------------------------Com_job::put_state_text

STDMETHODIMP Com_job::put_state_text( BSTR text )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );
      //if( current_thread_id() != _job->thread()->thread_id() )  return E_ACCESSDENIED;

        _job->set_state_text( bstr_as_string( text ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_job::get_title

STDMETHODIMP Com_job::get_title( BSTR* title )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        *title = SysAllocString_string( _job->title() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }

    return hr;
}

//-------------------------------------------------------------------Com_job::put_delay_after_error

STDMETHODIMP Com_job::put_delay_after_error( int error_steps, VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        _job->set_delay_after_error( error_steps, time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_job::get_order_queue

STDMETHODIMP Com_job::get_order_queue( Iorder_queue** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        *result = _job->order_queue();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.order_queue" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.order_queue" ); }

    return hr;
}

//-----------------------------------------------------------Com_job::put_delay_order_after_setback

STDMETHODIMP Com_job::put_delay_order_after_setback( int setback_number, VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        _job->set_delay_order_after_setback( setback_number, time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_order_after_setback" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_order_after_setback" ); }

    return hr;
}

//------------------------------------------------------------------Com_job::put_max_order_setbacks

STDMETHODIMP Com_job::put_max_order_setbacks( int count )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        _job->set_max_order_setbacks( count );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.max_order_setbacks" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.max_order_setbacks" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_task::_methods
#ifdef Z_COM

const Com_method Com_task::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "object_set"                , (Com_method_ptr)&Com_task::get_object_set         , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT,  2, "error"                     , (Com_method_ptr)&Com_task::put_error              , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET,  2, "error"                     , (Com_method_ptr)&Com_task::get_error              , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "job"                       , (Com_method_ptr)&Com_task::get_job                , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "params"                    , (Com_method_ptr)&Com_task::get_params             , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT,  5, "result"                    , (Com_method_ptr)&Com_task::put_result             , VT_EMPTY      , { VT_BYREF|VT_DISPATCH } },
    { DISPATCH_PROPERTYGET,  5, "result"                    , (Com_method_ptr)&Com_task::get_result             , VT_DISPATCH  },
    { DISPATCH_METHOD     ,  6, "wait_until_terminated"     , (Com_method_ptr)&Com_task::wait_until_terminated  , VT_BOOL       , { VT_R8 } },
    { DISPATCH_PROPERTYPUT,  7, "repeat"                    , (Com_method_ptr)&Com_task::put_repeat             , VT_EMPTY      , { VT_R8 } },
    { DISPATCH_METHOD     ,  8, "end"                       , (Com_method_ptr)&Com_task::end                    },
    { DISPATCH_PROPERTYPUT,  9, "history_field"             , (Com_method_ptr)&Com_task::put_history_field      , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 10, "id"                        , (Com_method_ptr)&Com_task::get_id                 , VT_I4         },
    { DISPATCH_PROPERTYPUT, 11, "delay_spooler_process"     , (Com_method_ptr)&Com_task::put_delay_spooler_process, VT_EMPTY    , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 12, "close_engine"              , (Com_method_ptr)&Com_task::put_close_engine       , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 13, "order"                     , (Com_method_ptr)&Com_task::get_order              , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 14, "java_class_name"           , (Com_method_ptr)&Com_task::get_java_class_name    , VT_BSTR },
    {}
};

#endif
//-------------------------------------------------------------------------------Com_task::Com_task

Com_task::Com_task( Task* task )
:
    Sos_ole_object( task_class_ptr, (Itask*)this ),
    _task(task)
{
}

//-------------------------------------------------------------------------Com_task::QueryInterface

STDMETHODIMP Com_task::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//-------------------------------------------------------------------------------Com_task::set_task

void Com_task::set_task( Task* task )
{ 
    THREAD_LOCK( _lock )
    {
        _task = task; 
    }
}

//-------------------------------------------------------------------------Com_task::get_object_set

STDMETHODIMP Com_task::get_object_set( Iobject_set** result )
{
    return E_NOTIMPL;
/*
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        if( !_task->_job->object_set_descr() )  return E_ACCESSDENIED;
        THREAD_LOCK( _task->_job->_lock )  *result = (dynamic_cast<Object_set_task*>(+_task))->_com_object_set;
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task::object_set" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task::object_set" ); }

    return hr;
*/
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::put_error( VARIANT* error_par )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        Variant error_vt = *error_par;
        hr = error_vt.ChangeType( VT_BSTR );        if( FAILED(hr) )  return hr;

        string error_text = bstr_as_string( error_vt.bstrVal );
        _task->set_error( Xc( "SPOOLER-120", error_text.c_str() ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::get_error( Ierror** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        THREAD_LOCK( _task->_lock )  *result = new Com_error( _task->error() );
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_task::get_job

STDMETHODIMP Com_task::get_job( Ijob** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        *result = _task->_job->com_job();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_params

STDMETHODIMP Com_task::get_params( Ivariable_set** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        *result = _task->_params;
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }

    return hr;
}

//------------------------------------------------------------------Com_task::wait_until_terminated

STDMETHODIMP Com_task::wait_until_terminated( double wait_time, VARIANT_BOOL* ok )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( _task )  *ok = _task->wait_until_terminated( wait_time );
               else  *ok = true;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.wait_until_terminated" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.wait_until_terminated" ); }

    return hr;
}

//------------------------------------------------------------------------------------Com_task::end

STDMETHODIMP Com_task::end()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( _task )  _task->cmd_end();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.end" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.end" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_result

STDMETHODIMP Com_task::put_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        THREAD_LOCK( _task->_lock )  hr = _task->_result.Copy( value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_result

STDMETHODIMP Com_task::get_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        VariantInit( value ); 
        THREAD_LOCK( _task->_job->_lock )  hr = VariantCopy( value, &_task->_result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_repeat

STDMETHODIMP Com_task::put_repeat( double seconds )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        _task->_job->set_repeat( seconds );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }

    return hr;
}

//----------------------------------------------------------------------Com_task::put_history_field

STDMETHODIMP Com_task::put_history_field( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        _task->set_history_field( bstr_as_string(name), *value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_task::get_id

STDMETHODIMP Com_task::get_id( int* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        *result = _task->id();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.id" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.id" ); }

    return hr;
}

//--------------------------------------------------------------Com_task::put_delay_spooler_process

STDMETHODIMP Com_task::put_delay_spooler_process( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        _task->set_delay_spooler_process( time_from_variant( *time ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.delay_spooler_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.delay_spooler_process" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_task::put_close_engine

STDMETHODIMP Com_task::put_close_engine( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        _task->set_close_engine( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.close_engine" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.close_engine" ); }

    return hr;
}

//------------------------------------------------------------------------------Com_task::get_order

STDMETHODIMP Com_task::get_order( Iorder** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = _task->order();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_thread::_methods
/*
#ifdef Z_COM

const Com_method Com_thread::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_thread::get_log              , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "script"                    , (Com_method_ptr)&Com_thread::get_script           , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "include_path"              , (Com_method_ptr)&Com_thread::get_include_path     , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  4, "name"                      , (Com_method_ptr)&Com_thread::get_name             , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  5, "java_class_name"           , (Com_method_ptr)&Com_thread::get_java_class_name  , VT_BSTR },
    {}
};

#endif
*/
//---------------------------------------------------------------------------Com_thread::Com_thread
/*
Com_thread::Com_thread( Spooler_thread* thread )
:
    Sos_ole_object( thread_class_ptr, (Ithread*)this ),
    _thread(thread)
{
}

//-----------------------------------------------------------------------Com_thread::QueryInterface

STDMETHODIMP Com_thread::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------------Com_thread::get_Log

STDMETHODIMP Com_thread::get_log( Ilog** com_log )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( current_thread_id() != _thread->thread_id() )  return E_ACCESSDENIED;

        *com_log = _thread->_com_log;
        if( *com_log )  (*com_log)->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------------Com_thread::get_script

STDMETHODIMP Com_thread::get_script( IDispatch** script_object )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( current_thread_id() != _thread->thread_id() )  return E_ACCESSDENIED;
        if( !_thread->_module_instance )  return E_ACCESSDENIED;

        *script_object = _thread->_module_instance->dispatch();
        if( *script_object )  (*script_object)->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------Com_thread::get_include_path

STDMETHODIMP Com_thread::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( current_thread_id() != _thread->thread_id() )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _thread->_include_path );
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------Com_thread::get_name

STDMETHODIMP Com_thread::get_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( current_thread_id() != _thread->thread_id() )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _thread->_name );
    }

    return NOERROR;
}
*/
//----------------------------------------------------------------------------Com_spooler::_methods
#ifdef Z_COM

const Com_method Com_spooler::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_spooler::get_log             , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "id"                        , (Com_method_ptr)&Com_spooler::get_id              , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  3, "param"                     , (Com_method_ptr)&Com_spooler::get_param           , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  4, "script"                    , (Com_method_ptr)&Com_spooler::get_script          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  5, "job"                       , (Com_method_ptr)&Com_spooler::get_job             , VT_DISPATCH  , { VT_BSTR } },
    { DISPATCH_METHOD     ,  6, "create_variable_set"       , (Com_method_ptr)&Com_spooler::create_variable_set , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  7, "include_path"              , (Com_method_ptr)&Com_spooler::get_include_path    , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  8, "log_dir"                   , (Com_method_ptr)&Com_spooler::get_log_dir         , VT_BSTR      },
    { DISPATCH_METHOD     ,  9, "let_run_terminate_and_restart", (Com_method_ptr)&Com_spooler::let_run_terminate_and_restart },
    { DISPATCH_PROPERTYGET, 10, "variables"                 , (Com_method_ptr)&Com_spooler::get_variables       , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT, 11, "var"                       , (Com_method_ptr)&Com_spooler::put_var             , VT_EMPTY     , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 11, "var"                       , (Com_method_ptr)&Com_spooler::get_var             , VT_VARIANT   , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 12, "db_name"                   , (Com_method_ptr)&Com_spooler::get_db_name         , VT_BSTR      },
    { DISPATCH_METHOD     , 13, "create_job_chain"          , (Com_method_ptr)&Com_spooler::create_job_chain    , VT_DISPATCH  },
    { DISPATCH_METHOD     , 14, "add_job_chain"             , (Com_method_ptr)&Com_spooler::add_job_chain       , VT_EMPTY     , { VT_DISPATCH } },
    { DISPATCH_PROPERTYGET, 15, "job_chain"                 , (Com_method_ptr)&Com_spooler::get_job_chain       , VT_DISPATCH  , { VT_BSTR } },
    { DISPATCH_METHOD     , 16, "create_order"              , (Com_method_ptr)&Com_spooler::create_order        , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET, 17, "is_service"                , (Com_method_ptr)&Com_spooler::get_is_service      , VT_BOOL      },
    { DISPATCH_PROPERTYGET, 17, "java_class_name"           , (Com_method_ptr)&Com_spooler::get_java_class_name , VT_BSTR },
    {}
};

#endif
//-------------------------------------------------------------------------Com_spooler::Com_spooler

Com_spooler::Com_spooler( Spooler* spooler )
:
    Sos_ole_object( spooler_class_ptr, (Ispooler*)this ),
    _spooler(spooler)
{
}

//----------------------------------------------------------------------Com_spooler::QueryInterface

STDMETHODIMP Com_spooler::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//-----------------------------------------------------------------------------Com_spooler::get_Log

STDMETHODIMP Com_spooler::get_log( Ilog** com_log )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *com_log = _spooler->_com_log;
        if( *com_log )  (*com_log)->AddRef();
    }

    return NOERROR;
}

//----------------------------------------------------------------------------------Com_spooler::id

STDMETHODIMP Com_spooler::get_id( BSTR* id_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *id_bstr = SysAllocString_string( _spooler->id() );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------------Com_spooler::param

STDMETHODIMP Com_spooler::get_param( BSTR* param_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *param_bstr = SysAllocString_string( _spooler->_spooler_param );
    }

    return NOERROR;
}

//---------------------------------------------------------------------------Com_spooler::get_script

STDMETHODIMP Com_spooler::get_script( IDispatch** script_object )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        if( !_spooler->_module_instance )  return E_ACCESSDENIED;

        *script_object = _spooler->_module_instance->dispatch();
        if( *script_object )  (*script_object)->AddRef();
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------Com_spooler::get_job

STDMETHODIMP Com_spooler::get_job( BSTR job_name, Ijob** com_job )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;
        *com_job = _spooler->get_job( bstr_as_string( job_name ) )->com_job();
        (*com_job)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Spooler.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Spooler.job" ); }

    return hr;
}

//-----------------------------------------------------------------Com_spooler::create_variable_set

STDMETHODIMP Com_spooler::create_variable_set( Ivariable_set** result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *result = new Com_variable_set;
        (*result)->AddRef();
    }

    return NOERROR;
}

//--------------------------------------------------------------------Com_spooler::get_include_path

STDMETHODIMP Com_spooler::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_include_path );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------Com_spooler::get_log_dir

STDMETHODIMP Com_spooler::get_log_dir( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_log_directory );
    }

    return NOERROR;
}

//-------------------------------------------------------Com_spooler::let_run_terminate_and_restart

STDMETHODIMP Com_spooler::let_run_terminate_and_restart()
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        _spooler->cmd_let_run_terminate_and_restart();
    }

    return NOERROR;
}

//-----------------------------------------------------------------------Com_spooler::get_variables

STDMETHODIMP Com_spooler::get_variables( Ivariable_set** result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *result = _spooler->_variables;
        if( *result )  (*result)->AddRef();
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------Com_spooler::put_var

STDMETHODIMP Com_spooler::put_var( BSTR name, VARIANT* value )
{
    HRESULT hr;
    ptr<Ivariable_set> variables;

    hr = get_variables( variables.pp() );  if( FAILED(hr) )  return hr;

    return variables->put_var( name, value );
}

//-----------------------------------------------------------------------------Com_spooler::get_var

STDMETHODIMP Com_spooler::get_var( BSTR name, VARIANT* value )
{
    HRESULT hr;
    ptr<Ivariable_set> variables;

    hr = get_variables( variables.pp() );  if( FAILED(hr) )  return hr;

    return variables->get_var( name, value );
}

//-------------------------------------------------------------------------Com_spooler::get_db_name

STDMETHODIMP Com_spooler::get_db_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_db_name );
    }

    return NOERROR;
}

//--------------------------------------------------------------------Com_spooler::create_job_chain

STDMETHODIMP Com_spooler::create_job_chain( spooler_com::Ijob_chain** result )
{
    ptr<Job_chain> job_chain = new Job_chain( _spooler );

    *result = job_chain;
    (*result)->AddRef();

    return S_OK;
}

//-----------------------------------------------------------------------Com_spooler::add_job_chain

STDMETHODIMP Com_spooler::add_job_chain( spooler_com::Ijob_chain* job_chain )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;

        _spooler->add_job_chain( dynamic_cast<Job_chain*>( job_chain ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.add_job_chain" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.add_job_chain" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_spooler::get_job_chain

STDMETHODIMP Com_spooler::get_job_chain( BSTR name, spooler_com::Ijob_chain** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;

        *result = _spooler->job_chain( string_from_bstr(name) ); //->com_job_chain();
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.get_job_chain" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.get_job_chain" ); }

    return hr;
}

//----------------------------------------------------------------------------Spooler::create_order

STDMETHODIMP Com_spooler::create_order( Iorder** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;

        *result = new Order( _spooler );
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.get_job_chain" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.get_job_chain" ); }

    return hr;
}

//--------------------------------------------------------------------------spooler::get_is_service

STDMETHODIMP Com_spooler::get_is_service( VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *result = _spooler->is_service();
    }

    return hr;
}

//----------------------------------------------------------------------------Com_context::_methods
#ifdef Z_COM

const Com_method Com_context::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_context::get_log             , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "spooler"                   , (Com_method_ptr)&Com_context::get_spooler         , VT_DISPATCH  },
  //{ DISPATCH_PROPERTYGET,  3, "thread"                    , (Com_method_ptr)&Com_context::get_thread          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "job"                       , (Com_method_ptr)&Com_context::get_job             , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  5, "task"                      , (Com_method_ptr)&Com_context::get_Task            , VT_DISPATCH  },
    {}
};

#endif
//-------------------------------------------------------------------------Com_context::Com_context

Com_context::Com_context()
: 
    Sos_ole_object( spooler_context_class_ptr, this )
{
}

//-------------------------------------------------------------------------------Com_context::close

void Com_context::close()
{ 
    THREAD_LOCK(_lock)
    {
        _log     = NULL;
        _spooler = NULL; 
      //_thread  = NULL; 
        _job     = NULL; 
      //if( _task )  _task->set_task(NULL), _task = NULL; 
        _task = NULL;
    }
}

//--------------------------------------------------------------------------Com_job_chain::_methods
#ifdef Z_COM

const Com_method Com_job_chain::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT,  1, "name"                      , (Com_method_ptr)&Com_job_chain::put_name          , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  1, "name"                      , (Com_method_ptr)&Com_job_chain::get_name          , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  2, "order_count"               , (Com_method_ptr)&Com_job_chain::get_order_count   , VT_INT        },
    { DISPATCH_METHOD     ,  3, "add_job"                   , (Com_method_ptr)&Com_job_chain::add_job           , VT_EMPTY      , { VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF }, 3 },
    { DISPATCH_METHOD     ,  4, "add_end_state"             , (Com_method_ptr)&Com_job_chain::add_end_state     , VT_EMPTY      , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_METHOD     ,  5, "add_order"                 , (Com_method_ptr)&Com_job_chain::add_order         , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  6, "node"                      , (Com_method_ptr)&Com_job_chain::get_node          , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  7, "order_queue"               , (Com_method_ptr)&Com_job_chain::get_order_queue   , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  8, "java_class_name"           , (Com_method_ptr)&Com_job_chain::get_java_class_name, VT_BSTR },
    {}
};

#endif
//---------------------------------------------------------------------Com_job_chain::Com_job_chain

Com_job_chain::Com_job_chain( Job_chain* job_chain )
:
    Sos_ole_object( job_chain_class_ptr, (Ijob_chain*)this ),
    _job_chain(job_chain)
{
}

//--------------------------------------------------------------------Com_job_chain::QueryInterface

STDMETHODIMP Com_job_chain::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this; 
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------Com_job_chain::get_length
/*
STDMETHODIMP Com_job_chain::get_length( int* result )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        *result = _job_chain->length(); 
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.length" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.length" ); }

    return hr;
}
*/
//--------------------------------------------------------------------------Com_job_chain::put_name

STDMETHODIMP Com_job_chain::put_name( BSTR name_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        _job_chain->set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.name" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.name" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_job_chain::get_name

STDMETHODIMP Com_job_chain::get_name( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        *result = bstr_from_string( _job_chain->name() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.name" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.name" ); }

    return hr;
}

//-------------------------------------------------------------------Com_job_chain::get_order_count

STDMETHODIMP Com_job_chain::get_order_count( int* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        *result = _job_chain->order_count();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_count" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_count" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_job_chain::add_job

STDMETHODIMP Com_job_chain::add_job( VARIANT* job_or_jobname, VARIANT* begin_state, VARIANT* end_state, VARIANT* error_state )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        Job* job = NULL;

        switch( job_or_jobname->vt )
        {
            case VT_BSTR:
            {
                string jobname = string_from_variant(*job_or_jobname);
                
                job = stricmp( jobname.c_str(), "*end" ) == 0? NULL
                                                             : _job_chain->_spooler->get_job( jobname );
                break;
            }

          //case VT_UNKNOWN:
          //case VT_DISPATCH:
          //{
          //    break;
          //}

            default: return DISP_E_TYPEMISMATCH;
        }

        _job_chain->add_job( job, *begin_state, *end_state, *error_state );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_job" ); }

    return hr;
}

//---------------------------------------------------------------------Com_job_chain::add_end_state

STDMETHODIMP Com_job_chain::add_end_state( VARIANT* state )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        VARIANT error; VariantInit( &error );  error.vt = VT_ERROR;
        _job_chain->add_job( NULL, *state, error, error );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_end_state" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_end_state" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_job_chain::finish
/*
STDMETHODIMP Com_job_chain::finish()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;

        _job_chain->finish();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.finish" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.finish" ); }

    return hr;
}
*/
//-------------------------------------------------------------------------Com_job_chain::add_order

STDMETHODIMP Com_job_chain::add_order( VARIANT* order_or_payload, spooler_com::Iorder** result )
{
    HRESULT hr = NOERROR;

    LOGI( "Job_chain.add_order\n" );

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SPOOLER-151" );

        ptr<spooler_com::Iorder> iorder = order_from_order_or_payload( _job_chain->_spooler, *order_or_payload );
        if( !iorder )  return E_POINTER;

        // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
        dynamic_cast<Order*>( &*iorder )->add_to_job_chain( dynamic_cast<Job_chain*>( this ) );  

        *result = iorder;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_order" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_order" ); }

    LOG( "Job_chain.add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//-------------------------------------------------------------------Com_job_chain::get_order_queue

STDMETHODIMP Com_job_chain::get_order_queue( VARIANT* state, Iorder_queue** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SPOOLER-151" );

        *result = _job_chain->node_from_state( *state )->_job->order_queue();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_queue" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_queue" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_job_chain::node

STDMETHODIMP Com_job_chain::get_node( VARIANT* state, Ijob_chain_node** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SPOOLER-151" );

        *result = _job_chain->node_from_state( *state );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }

    return hr;
}

//---------------------------------------------------------------------Com_job_chain_node::_methods
#ifdef Z_COM

const Com_method Com_job_chain_node::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                               , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "state"                     , (Com_method_ptr)&Com_job_chain_node::get_state        , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  2, "next_node"                 , (Com_method_ptr)&Com_job_chain_node::get_next_node    , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "error_node"                , (Com_method_ptr)&Com_job_chain_node::get_error_node   , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "job"                       , (Com_method_ptr)&Com_job_chain_node::get_job          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  5, "next_state"                , (Com_method_ptr)&Com_job_chain_node::get_next_state   , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  6, "error_state"               , (Com_method_ptr)&Com_job_chain_node::get_error_state  , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  7, "java_class_name"           , (Com_method_ptr)&Com_job_chain_node::get_java_class_name, VT_BSTR },
    {}
};

#endif
//-----------------------------------------------------------Com_job_chain_node::Com_job_chain_node

Com_job_chain_node::Com_job_chain_node()
:
    Sos_ole_object( job_chain_node_class_ptr, (Ijob_chain_node*)this )
{
}

//---------------------------------------------------------------Com_job_chain_node::QueryInterface

STDMETHODIMP Com_job_chain_node::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//--------------------------------------------------------------------Com_job_chain_node::get_state

STDMETHODIMP Com_job_chain_node::get_state( VARIANT* result ) 
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_state );
}

//----------------------------------------------------------------Com_job_chain_node::get_next_node

STDMETHODIMP Com_job_chain_node::get_next_node( Ijob_chain_node** result )
{ 
    *result = ((Job_chain_node*)(this))->_next_node;
    if( *result )  (*result)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------Com_job_chain_node::get_error_node

STDMETHODIMP Com_job_chain_node::get_error_node( Ijob_chain_node** result )   
{ 
    *result = ((Job_chain_node*)(this))->_error_node; 
    if( *result )  (*result)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------Com_job_chain_node::get_next_state

STDMETHODIMP Com_job_chain_node::get_next_state( VARIANT* result )
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_next_state );
}

//--------------------------------------------------------------Com_job_chain_node::get_error_state

STDMETHODIMP Com_job_chain_node::get_error_state( VARIANT* result )   
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_error_state );
}

//----------------------------------------------------------------------Com_job_chain_node::get_job

STDMETHODIMP Com_job_chain_node::get_job( Ijob** result )              
{ 
    *result = ((Job_chain_node*)(this))->_job->com_job(); 
    if( *result )  (*result)->AddRef();
    return S_OK;
}

//------------------------------------------------------------------------------Com_order::_methods
#ifdef Z_COM

const Com_method Com_order::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT,  1, "id"                        , (Com_method_ptr)&Com_order::put_id                , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  1, "id"                        , (Com_method_ptr)&Com_order::get_id                , VT_VARIANT    },
    { DISPATCH_PROPERTYPUT,  2, "title"                     , (Com_method_ptr)&Com_order::put_title             , VT_EMPTY      , { VT_BSTR     } },
    { DISPATCH_PROPERTYGET,  2, "title"                     , (Com_method_ptr)&Com_order::get_title             , VT_BSTR       },
    { DISPATCH_PROPERTYPUT,  3, "priority"                  , (Com_method_ptr)&Com_order::put_priority          , VT_EMPTY      , { VT_INT      } },
    { DISPATCH_PROPERTYGET,  3, "priority"                  , (Com_method_ptr)&Com_order::get_priority          , VT_INT        },
    { DISPATCH_PROPERTYGET,  4, "job_chain"                 , (Com_method_ptr)&Com_order::get_job_chain         , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET,  5, "job_chain_node"            , (Com_method_ptr)&Com_order::get_job_chain_node    , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUTREF,6,"job"                       , (Com_method_ptr)&Com_order::putref_job            , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYPUT,  6, "job"                       , (Com_method_ptr)&Com_order::put_job               , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  6, "job"                       , (Com_method_ptr)&Com_order::get_job               , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUT,  7, "state"                     , (Com_method_ptr)&Com_order::put_state             , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  7, "state"                     , (Com_method_ptr)&Com_order::get_state             , VT_VARIANT    },
    { DISPATCH_PROPERTYPUT,  8, "state_text"                , (Com_method_ptr)&Com_order::put_state_text        , VT_EMPTY      , { VT_BSTR     } },
    { DISPATCH_PROPERTYGET,  8, "state_text"                , (Com_method_ptr)&Com_order::get_state_text        , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  9, "error"                     , (Com_method_ptr)&Com_order::get_error             , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUTREF,10,"payload"                  , (Com_method_ptr)&Com_order::putref_payload        , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYPUT, 10, "payload"                   , (Com_method_ptr)&Com_order::put_payload           , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET, 10, "payload"                   , (Com_method_ptr)&Com_order::get_payload           , VT_VARIANT    },
    { DISPATCH_METHOD     , 11, "payload_is_type"           , (Com_method_ptr)&Com_order::payload_is_type       , VT_BOOL       , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 12, "java_class_name"           , (Com_method_ptr)&Com_order::get_java_class_name   , VT_BSTR },
    { DISPATCH_METHOD     , 13, "setback"                   , (Com_method_ptr)&Com_order::setback               , VT_EMPTY      },
    {}
};

#endif
//-----------------------------------------------------------------------------Com_order::Com_order

Com_order::Com_order( Order* order )
:
    Sos_ole_object( order_class_ptr, (Iorder*)this ),
    _zero_(this+1),
    _order(order)
{
}

//------------------------------------------------------------------------Com_order::QueryInterface

STDMETHODIMP Com_order::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//--------------------------------------------------------------------------------Com_order::put_id

STDMETHODIMP Com_order::put_id( VARIANT* id )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_id( *id );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.id" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.id" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_order::get_id

STDMETHODIMP Com_order::get_id( VARIANT* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        return _order->id().CopyTo( result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.id" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.id" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_order::put_title

STDMETHODIMP Com_order::put_title( BSTR title_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_title( string_from_bstr(title_bstr) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.title" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.title" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_order::get_title

STDMETHODIMP Com_order::get_title( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = bstr_from_string( _order->title() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.title" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.title" ); }

    return hr;
}
    
//--------------------------------------------------------------------------Com_order::put_priority

STDMETHODIMP Com_order::put_priority( int priority )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_priority( priority );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.priority" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.priority" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_order::get_priority

STDMETHODIMP Com_order::get_priority( int* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = _order->priority();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.priority" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.priority" ); }

    return hr;
}
    
//-------------------------------------------------------------------------Com_order::get_job_chain

STDMETHODIMP Com_order::get_job_chain( Ijob_chain** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        Job_chain* job_chain = _order->job_chain();
        if( job_chain )  
        {
            ptr<Ijob_chain> ijob_chain = job_chain; //->com_job_chain();
            ijob_chain.CopyTo( result );
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.job_chain" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.job_chain" ); }

    return hr;
}

//--------------------------------------------------------------------Com_order::get_job_chain_node

STDMETHODIMP Com_order::get_job_chain_node( Ijob_chain_node** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_order )  return E_POINTER;

        *result = _order->job_chain_node();
        if( *result )  (*result)->AddRef();
    }

    return hr;
}

//-------------------------------------------------------------------------------Com_order::put_job

STDMETHODIMP Com_order::put_job( VARIANT* job_or_jobname )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        switch( job_or_jobname->vt )
        {
            case VT_BSTR:       
                _order->set_job_by_name( string_from_bstr( V_BSTR(job_or_jobname) ) ); 
                break;
/*
            case VT_DISPATCH:
            case VT_UNKNOWN:    
            {
                ptr<Ijob> ijob;
                hr = V_UNKNOWN(job_or_jobname)->QueryInterface( IID_Ijob, ijob.void_pp() );
                if( FAILED(hr) )  return hr;

                Job* job = ijob? dynamic_cast<Com_job*>( &*ijob )->_job : NULL;
                _order->set_job( job );
                break;
            }
*/
            default:            
                return DISP_E_TYPEMISMATCH;
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.job" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_order::get_job

STDMETHODIMP Com_order::get_job( Ijob** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = _order->com_job();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.job" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_order::put_state

STDMETHODIMP Com_order::put_state( VARIANT* state )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_state(*state);
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.state" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.state" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_order::get_state

STDMETHODIMP Com_order::get_state( VARIANT* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        return _order->state().CopyTo( result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.state" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.state" ); }

    return hr;
}

//------------------------------------------------------------------------Com_order::put_state_text

STDMETHODIMP Com_order::put_state_text( BSTR state_text_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_state_text( string_from_bstr(state_text_bstr) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.state_text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.state_text" ); }

    return hr;
}

//------------------------------------------------------------------------Com_order::get_state_text

STDMETHODIMP Com_order::get_state_text( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = bstr_from_string( _order->state_text() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.state_text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.state_text" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_order::get_error

STDMETHODIMP Com_order::get_error( Ierror** )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        return E_FAIL;//...;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.error" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_order::put_payload

STDMETHODIMP Com_order::put_payload( VARIANT* payload )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->set_payload( *payload );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }

    return hr;
}

//------------------------------------------------------------------------Com_order::putref_payload

STDMETHODIMP Com_order::putref_payload( IUnknown* payload )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        Variant payload_vt = payload;

        if( payload )
        {
            ptr<IDispatch> idispatch;
            hr = payload->QueryInterface( IID_IDispatch, idispatch.void_pp() );
            if( SUCCEEDED(hr) )  payload_vt = idispatch;
        }

        _order->set_payload( payload_vt );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_order::get_payload

STDMETHODIMP Com_order::get_payload( VARIANT* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        hr = _order->payload().CopyTo( result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.payload" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_order::payload_is_type

STDMETHODIMP Com_order::payload_is_type( BSTR typname_bstr, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    *result = false;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        string typname = lcase( string_from_bstr( typname_bstr ) );

        Variant payload = _order->payload();
        
        switch( payload.vt )
        {
            case VT_UNKNOWN:
            case VT_DISPATCH:
            {
                ptr<IUnknown> iunknown;

                if( typname == "spooler.variable_set" )
                {
                    hr = V_UNKNOWN(&payload)->QueryInterface( IID_Ivariable_set, iunknown.void_pp() );
                    if( SUCCEEDED(hr)  )  { *result = true;  return hr; }
                    iunknown = NULL;
                }

                if( typname == "hostware.dyn_obj" 
                 || typname == "hostware.record" )
                {
                    hr = V_UNKNOWN(&payload)->QueryInterface( IID_Ihostware_dynobj, iunknown.void_pp() );
                    if( SUCCEEDED(hr) )  { *result = true;  return hr; }
                }

                hr = S_FALSE;
                break;
            }
            
            default: ;
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.payload_is_type" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.payload_is_type" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_order::setback

STDMETHODIMP Com_order::setback()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->setback_();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.setback" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.setback" ); }

    return hr;
}

//----------------------------------------------------------------------Com_order::add_to_job_chain
/*
STDMETHODIMP Com_order::add_to_job_chain( Ijob_chain* ijob_chain )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;
        if( !ijob_chain )  return E_POINTER;

        _order->add_to_job_chain( dynamic_cast<Com_job_chain*>( &*ijob_chain )->_job_chain );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order.add_to_job_chain" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order.add_to_job_chain" ); }

    return hr;
}
*/
//-----------------------------------------------------------------------Com_order_queue::_methods
#ifdef Z_COM

const Com_method Com_order_queue::_methods[] =
{ 
   // _flags          , dispid, _name                       , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "length"                    , (Com_method_ptr)&Com_order_queue::get_length      , VT_INT        },
    { DISPATCH_METHOD     ,  2, "add_order"                 , (Com_method_ptr)&Com_order_queue::add_order       , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  3, "java_class_name"           , (Com_method_ptr)&Com_order_queue::get_java_class_name, VT_BSTR },
    {}
};

#endif
//-----------------------------------------------------------------Com_order_queue::Com_order_queue

Com_order_queue::Com_order_queue()
:
    Sos_ole_object( order_queue_class_ptr, (Iorder_queue*)this ),
    _zero_(this+1)
{
}

//------------------------------------------------------------------Com_order_queue::QueryInterface

STDMETHODIMP Com_order_queue::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

    return Sos_ole_object::QueryInterface( iid, result );
}

//----------------------------------------------------------------------Com_order_queue::get_length

STDMETHODIMP Com_order_queue::get_length( int* result )
{
    THREAD_LOCK( _lock )
    {
        *result = dynamic_cast<Order_queue*>(this)->length();
    }

    return S_OK;
}

//-----------------------------------------------------------------------Com_order_queue::add_order

STDMETHODIMP Com_order_queue::add_order( VARIANT* order_or_payload, Iorder** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        ptr<spooler_com::Iorder> iorder = order_from_order_or_payload( dynamic_cast<Order_queue*>(this)->_spooler, *order_or_payload );
        if( !iorder )  return E_POINTER;

        // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
        dynamic_cast<Order*>( &*iorder )->add_to_order_queue( dynamic_cast<Order_queue*>( this ) );

        *result = iorder;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Order_queue.add_order" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Order_queue.add_order" ); }

    LOG( "Com_order_queue::add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
