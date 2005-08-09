// $Id$
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
#include "../zschimmer/com_server.h"


#ifdef Z_WINDOWS
#   include <process.h>                    // getpid()
#endif

using namespace zschimmer::com;


namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//-------------------------------------------------------------------------Typbibliothek (hostware)

Typelib_descr spooler_typelib ( LIBID_spooler_com, "Spooler", "1.0" );

DESCRIBE_CLASS( &spooler_typelib, Com_error          , error          , CLSID_Error          , "Spooler.Error"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable       , variable       , CLSID_Variable       , "Spooler.Variable"      , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable_set   , variable_set   , CLSID_Variable_set   , "Spooler.Variable_set"  , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable_set_enumerator, variable_set_enumerator, CLSID_Variable_set_enumerator, "Spooler.Com_variable_set_enumerator", "1.0" );
DESCRIBE_CLASS( &spooler_typelib, Com_log            , log            , CLSID_Log            , "Spooler.Log"           , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job            , job            , CLSID_Job            , "Spooler.Job"           , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_task           , task           , CLSID_Task           , "Spooler.Task"          , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_object_set     , object_set     , CLSID_Object_set     , "Spooler.Object_set"    , "1.0" )
//DESCRIBE_CLASS( &spooler_typelib, Com_thread        , thread        , CLSID_thread         , "Spooler.Thread"        , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler        , spooler        , CLSID_Spooler        , "Spooler.Spooler"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler_context, spooler_context, CLSID_Spooler_context, "Spooler.Context"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job_chain      , job_chain      , CLSID_Job_chain      , "Spooler.Job_chain"     , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job_chain_node , job_chain_node , CLSID_Job_chain_node , "Spooler.Job_chain_node", "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_order          , order          , CLSID_Order          , "Spooler.Order"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_order_queue    , order_queue    , CLSID_Order_queue    , "Spooler.Order_queue"   , "1.0" )

//------------------------------------------------------------------------------------Typbibliothek

Typelib_ref                         typelib;  

Com_task_proxy::Class_descriptor    Com_task_proxy::class_descriptor ( &typelib, "Spooler.Task_proxy", Com_task_proxy::_methods );

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
    { DISPATCH_PROPERTYGET, 1, "Java_class_name" , (Com_method_ptr)&Com_error::get_Java_class_name, VT_BSTR },
    { DISPATCH_PROPERTYGET, 2, "Is_error"        , (Com_method_ptr)&Com_error::get_Is_error       , VT_BOOL },
    { DISPATCH_PROPERTYGET, 3, "Code"            , (Com_method_ptr)&Com_error::get_Code           , VT_BSTR }, 
    { DISPATCH_PROPERTYGET, 4, "Text"            , (Com_method_ptr)&Com_error::get_Text           , VT_BSTR }, 
    {}
};

#endif
//-----------------------------------------------------------------------------Com_error::Com_error

Com_error::Com_error( const Xc_copy& x )
: 
    Sos_ole_object( error_class_ptr, (Ierror*)this ),
    _lock("lock"),
    _xc(x) 
{
}

//------------------------------------------------------------------------Com_error::QueryInterface

STDMETHODIMP Com_error::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

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

STDMETHODIMP Com_error::get_Is_error( VARIANT_BOOL* result )
{
    THREAD_LOCK( _lock )
    {
        *result = _xc != NULL;
    }

    return NOERROR;
}

//----------------------------------------------------------------------------------Com_error::code

STDMETHODIMP Com_error::get_Code( BSTR* code_bstr )
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

STDMETHODIMP Com_error::get_Text( BSTR* text_bstr )
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
    { DISPATCH_PROPERTYPUT, 0, "value"              , (Com_method_ptr)&Com_variable::put_Value          , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 0, "value"              , (Com_method_ptr)&Com_variable::get_Value          , VT_VARIANT    },
    { DISPATCH_PROPERTYGET, 1, "name"               , (Com_method_ptr)&Com_variable::get_Name           , VT_BSTR       },
    { DISPATCH_METHOD     , 2, "Clone"              , (Com_method_ptr)&Com_variable::Clone              , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 3, "java_class_name"    , (Com_method_ptr)&Com_variable::get_Java_class_name, VT_BSTR },
    {}
};

#endif
//-----------------------------------------------------------------------Com_variable::Com_variable

Com_variable::Com_variable( const BSTR name, const VARIANT& value )
:
    Sos_ole_object( variable_class_ptr, (Ivariable*)this ),
    _lock("Com_variable")
{
    if( SysStringLen( name ) == 0 )  throw_xc( "SCHEDULER-198" );

    THREAD_LOCK( _lock )
    {
        _name = name;
        _value = value;
    }
}

//---------------------------------------------------------------------Com_variable::QueryInterface

STDMETHODIMP Com_variable::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------------Com_variable::Clone

STDMETHODIMP Com_variable::Clone( Ivariable** result ) 
{ 
    HRESULT hr = NOERROR; 
    
    THREAD_LOCK( _lock )
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
    { DISPATCH_METHOD     , 1, "set_var"            , (Com_method_ptr)&Com_variable_set::Set_var        , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 0, "value"              , (Com_method_ptr)&Com_variable_set::put_Value      , VT_EMPTY      , { VT_BYREF|VT_VARIANT, VT_BYREF|VT_VARIANT }, 1 },
    { DISPATCH_PROPERTYGET, 0, "value"              , (Com_method_ptr)&Com_variable_set::get_Value      , VT_VARIANT    , { VT_BYREF|VT_VARIANT                      }, 1 },
    { DISPATCH_PROPERTYPUT, 8, "var"                , (Com_method_ptr)&Com_variable_set::put_Var        , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 8, "var"                , (Com_method_ptr)&Com_variable_set::get_Var        , VT_VARIANT    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 2, "count"              , (Com_method_ptr)&Com_variable_set::get_Count      , VT_I4         },
    { DISPATCH_PROPERTYGET, 3, "dom"                , (Com_method_ptr)&Com_variable_set::get_Dom        , VT_DISPATCH   },
    { DISPATCH_METHOD     , 4, "Clone"              , (Com_method_ptr)&Com_variable_set::Clone          , VT_DISPATCH   },
    { DISPATCH_METHOD     , 5, "merge"              , (Com_method_ptr)&Com_variable_set::Merge          , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYGET, DISPID_NEWENUM, "_NewEnum", (Com_method_ptr)&Com_variable_set::get__NewEnum , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUT, 6, "xml"                , (Com_method_ptr)&Com_variable_set::put_Xml        , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 6, "xml"                , (Com_method_ptr)&Com_variable_set::get_Xml        , VT_BSTR       },
    { DISPATCH_PROPERTYGET, 7, "java_class_name"    , (Com_method_ptr)&Com_variable_set::get_Java_class_name, VT_BSTR },
    {}
};

#endif
//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set()
:
    Sos_ole_object( variable_set_class_ptr, (Ivariable_set*)this ),
    _lock("Com_variable_set"),
    _ignore_case(true)
{
}

//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set( const xml::Element_ptr& element, const string& variable_element_name )
:
    Sos_ole_object( variable_set_class_ptr, (Ivariable_set*)this ),
    _lock("Com_variable_set"),
    _ignore_case(true)
{
    set_dom( element, variable_element_name );
}

//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set( const Com_variable_set& o )
:
    Sos_ole_object( variable_set_class_ptr, (Ivariable_set*)this ),
    _ignore_case(o._ignore_case)
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------Com_variable_set::set_dom

void Com_variable_set::set_dom( const xml::Element_ptr& params, const string& variable_element_name )
{
    HRESULT hr;

    THREAD_LOCK( _lock )
    {
        DOM_FOR_EACH_ELEMENT( params, e )
        {
            if( e.nodeName_is( variable_element_name ) ) 
            {
                Bstr    name  = e.getAttribute( "name" );
                Variant value = e.getAttribute( "value" );

                hr = put_Var( name, &value );                       if( FAILED(hr) )  throw_ole( hr, "Ivariable_set::put_var" );
            }
        }
    }
}

//----------------------------------------------------------------------Com_variable_set::put_value

STDMETHODIMP Com_variable_set::put_Value( VARIANT* name, VARIANT* value )
{
    if( name->vt == VT_BSTR )
    {
        return put_Var( V_BSTR(name), value );
    }
    else
    if( name->vt == VT_ERROR )
    {
        if( value->vt != VT_BSTR )  return DISP_E_TYPEMISMATCH;
        
        return put_Xml( V_BSTR(value) );
    }
    else 
        return DISP_E_TYPEMISMATCH;
}

//----------------------------------------------------------------------Com_variable_set::get_value

STDMETHODIMP Com_variable_set::get_Value( VARIANT* name, VARIANT* value )
{
    if( name->vt == VT_BSTR )
    {
        return get_Var( V_BSTR(name), value );
    }
    else
    if( name->vt == VT_ERROR )
    {
        value->vt      = VT_BSTR;
        value->bstrVal = NULL;

        HRESULT hr = get_Xml( &V_BSTR(value) );
        if( !FAILED(hr) )  LOG( "Com_variable_set::get_value => " << string_from_bstr(V_BSTR(value)) << "\n" );
        return hr;
    }
    else 
        return DISP_E_TYPEMISMATCH;
}

//------------------------------------------------------------------------Com_variable_set::set_var

void Com_variable_set::set_var( const string& name, const Variant& value )
{
    HRESULT hr = put_Var( Bstr( name ), (VARIANT*)&value );
    if( FAILED(hr) )  throw_ole( hr, "Ivariable_set::put_var" );
}

//---------------------------------------------------------------------Com_variable_set::get_string

string Com_variable_set::get_string( const string& name )
{
    Variant result;
    
    HRESULT hr = get_Var( Bstr(name), &result );
    if( FAILED(hr) )  throw_com( hr, "Ivariable_set::get_Var" );

    return string_from_variant( result );
}


//-------------------------------------------------------------Com_variable_set::get_string_by_name

string Com_variable_set::get_string_by_name( const string& name, bool* name_found ) const
{
    Variant result;
    
    get_var( Bstr(name), &result );

    *name_found = !result.is_empty();

    return string_from_variant( result );
}

//--------------------------------------------------------------------------Com_variable_set::merge

void Com_variable_set::merge( const Com_variable_set* other )
{
    HRESULT hr = Merge( const_cast<Com_variable_set*>( other ) );
    if( FAILED(hr) )  throw_com( hr, "Com_variable_set::merge" );
}

//------------------------------------------------------------------------Com_variable_set::put_var

STDMETHODIMP Com_variable_set::put_Var( BSTR name, VARIANT* value )
{
    // Vorsicht mit _map.erase(): Ein Iterator auf das gelöschte Element wird ungültig. 
    // Com_variable_set_enumerator müsste dann ungültig werden. Aber wir benutzen erase() nicht.

    HRESULT hr = NOERROR;

    try
    {
        THREAD_LOCK( _lock )  
        {
            Bstr lname = name;

            if( _ignore_case )  bstr_to_lower( &lname._bstr );

            Map::iterator it = _map.find( lname );
            if( it != _map.end()  &&  it->second )
            {
                it->second->put_Value( value );
            }
            else
            {
                ptr<Com_variable> v = new Com_variable( name, *value );
                _map[lname] = v;
            }
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::var" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::var" ); }

    return hr;
}

//------------------------------------------------------------------------Com_variable_set::get_var

void Com_variable_set::get_var( BSTR name, VARIANT* value ) const
{
    THREAD_LOCK( _lock )  
    {
        VariantInit( value );

        Bstr lname = name;
        if( _ignore_case )  bstr_to_lower( &lname._bstr );

        Map::const_iterator it = _map.find( lname );
        if( it != _map.end()  &&  it->second )
        {
            HRESULT hr = it->second->get_Value( value );
            if( !FAILED(hr))  hr = S_OK;
        }
    }
}

//------------------------------------------------------------------------Com_variable_set::get_var

STDMETHODIMP Com_variable_set::get_Var( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    try
    {
        get_var( name, value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::var" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::var" ); }

    return hr;
}

//----------------------------------------------------------------------Com_variable_set::get_count

STDMETHODIMP Com_variable_set::get_Count( int* result )
{
    THREAD_LOCK( _lock )  *result = _map.size();
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_dom

STDMETHODIMP Com_variable_set::get_Dom( IXMLDOMDocument** doc )  
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

        __assume( doc );
        return E_NOTIMPL;

#   endif
}

//----------------------------------------------------------------------------Com_variable_set::dom

xml::Document_ptr Com_variable_set::dom()
{
    xml::Document_ptr doc;
    
    doc.create();
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    doc.appendChild( dom_element( doc, xml_element_name(), "variable" ) );

    return doc;
}

//--------------------------------------------------------------------Com_variable_set::dom_element

xml::Element_ptr Com_variable_set::dom_element( const xml::Document_ptr& doc, const string& element_name, const string& subelement_name )
{
    xml::Element_ptr varset = doc.createElement( element_name );

    THREAD_LOCK( _lock )
    {
        for( Map::iterator it = _map.begin(); it != _map.end(); it++ )
        {
            Com_variable* v = it->second;
            if( v )
            {
                Bstr    name;
                Variant value;

                v->get_Name( &name._bstr );
                v->get_Value( &value );

                xml::Element_ptr var = doc.createElement( subelement_name );
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

    return varset;
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

//--------------------------------------------------------------------------Com_variable_set::Merge

STDMETHODIMP Com_variable_set::Merge( Ivariable_set* other )
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

                Bstr name = v->_name;
                if( _ignore_case )  bstr_to_lower( &name );
                _map[ name ] = v;
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

//------------------------------------------------------------------------Com_variable_set::set_xml

void Com_variable_set::set_xml( const string& xml_text )  
{ 
    HRESULT hr = put_Xml( Bstr( xml_text ) );
    if( FAILED(hr) )  throw_ole( hr, "Variable_set::xml" );
}

//------------------------------------------------------------------------Com_variable_set::put_Xml

STDMETHODIMP Com_variable_set::put_Xml( BSTR xml_text )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        xml::Document_ptr doc = dom();
        doc.load_xml( string_from_bstr( xml_text ) );

        DOM_FOR_EACH_ELEMENT( doc.documentElement(), e )
        {
            if( e.nodeName_is( "variable" ) || e.nodeName_is( "param" ) )
            {
                Bstr    name  = e.getAttribute( "name" );
                Variant value = e.getAttribute( "value" );
                VARTYPE vt    = e.int_getAttribute( "vt", VT_BSTR );
                value.change_type( vt );

                hr = put_Var( name, &value );
                if( FAILED( hr ) )  break;
            }
            else
                throw_xc( "SCHEDULER-182" );
        }

    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }

    return hr;
}

//------------------------------------------------------------------------Com_variable_set::get_xml

STDMETHODIMP Com_variable_set::get_Xml( BSTR* xml_doc  )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        hr = String_to_bstr( dom().xml(), xml_doc );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::xml" ); }

    return hr;
}

//------------------------------------------------------------Com_variable_set_enumerator::_methods
#ifdef Z_COM
/*
const Com_method Com_variable_set_enumerator::_methods[] =
{ 
   // _flags         , dispid, _name    , _method                                               , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "Next"   , (Com_method_ptr)&Com_variable_set_enumerator::Next    , VT_LONG       , { VT_LONG, VT_BYREF|VT_VARIANT } },
    { DISPATCH_METHOD     , 2, "Skip"   , (Com_method_ptr)&Com_variable_set_enumerator::Skip    , VT_EMPTY      , { VT_LONG } },
    { DISPATCH_METHOD     , 3, "Reset"  , (Com_method_ptr)&Com_variable_set_enumerator::Reset   },
    { DISPATCH_METHOD     , 4, "Clone"  , (Com_method_ptr)&Com_variable_set_enumerator::Clone   , VT_DISPATCH  },
    {}
};
*/
#endif
//-----------------------------------------Com_variable_set_enumerator::Com_variable_set_enumerator

Com_variable_set_enumerator::Com_variable_set_enumerator()
:
    Sos_ole_object( variable_set_enumerator_class_ptr, this )
{
}

//------------------------------------------------------Com_variable_set_enumerator::QueryInterface

STDMETHODIMP Com_variable_set_enumerator::QueryInterface( REFIID iid, void** result )
{                                                                    
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IEnumVARIANT            , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ivariable_set_enumerator, result );

/*
    if( iid == IID_IEnumVARIANT )
    {
        *result = (IEnumVARIANT*)this;
        (*result)->AddRef();
        return S_OK;
    }

    if( iid == IID_Ivariable_set_enumerator )
    {
        *result = (Ivariable_set_enumerator)this;
        (*result)->AddRef();
        return S_OK;
    }
*/
    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//----------------------------------------------------------Com_variable_set_enumerator::initialize

void Com_variable_set_enumerator::initialize( Com_variable_set* v )
{
    _variable_set = v;
    Reset();
}

//----------------------------------------------------------------Com_variable_set_enumerator::Next

STDMETHODIMP Com_variable_set_enumerator::Next( ulong32 celt, VARIANT* result, ulong32* pceltFetched )
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

STDMETHODIMP Com_variable_set_enumerator::Skip( ulong32 celt )
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

STDMETHODIMP Com_variable_set_enumerator::Clone( IEnumVARIANT** )
{
    return ERROR;
}

//--------------------------------------------------------------------------------Com_log::_methods
#ifdef Z_COM

const Com_method Com_log::_methods[] =
{ 
   // _flags         , dispid, _name                   , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     ,  1, "debug9"               , (Com_method_ptr)&Com_log::Debug9                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  2, "debug8"               , (Com_method_ptr)&Com_log::Debug8                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  3, "debug7"               , (Com_method_ptr)&Com_log::Debug7                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  4, "debug6"               , (Com_method_ptr)&Com_log::Debug6                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  5, "debug5"               , (Com_method_ptr)&Com_log::Debug5                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  6, "debug4"               , (Com_method_ptr)&Com_log::Debug4                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  7, "debug3"               , (Com_method_ptr)&Com_log::Debug3                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  8, "debug2"               , (Com_method_ptr)&Com_log::Debug2                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  9, "debug1"               , (Com_method_ptr)&Com_log::Debug1                  , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 10, "debug"                , (Com_method_ptr)&Com_log::Debug                   , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     ,  0, "info"                 , (Com_method_ptr)&Com_log::Info                    , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 11, "msg"                  , (Com_method_ptr)&Com_log::Msg                     , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 12, "warn"                 , (Com_method_ptr)&Com_log::Warn                    , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 13, "error"                , (Com_method_ptr)&Com_log::Error                   , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_METHOD     , 14, "log"                  , (Com_method_ptr)&Com_log::Log                     , VT_EMPTY      , { VT_I4, VT_BSTR } },
    { DISPATCH_PROPERTYGET, 15, "mail"                 , (Com_method_ptr)&Com_log::get_Mail                , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT, 16, "mail_on_error"        , (Com_method_ptr)&Com_log::put_Mail_on_error       , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 16, "mail_on_error"        , (Com_method_ptr)&Com_log::get_Mail_on_error       , VT_BOOL       },
    { DISPATCH_PROPERTYPUT, 17, "mail_on_success"      , (Com_method_ptr)&Com_log::put_Mail_on_success     , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 17, "mail_on_success"      , (Com_method_ptr)&Com_log::get_Mail_on_success     , VT_BOOL       },
    { DISPATCH_PROPERTYPUT, 18, "mail_on_process"      , (Com_method_ptr)&Com_log::put_Mail_on_process     , VT_EMPTY      , { VT_I4 } },
    { DISPATCH_PROPERTYGET, 18, "mail_on_process"      , (Com_method_ptr)&Com_log::get_Mail_on_process     , VT_I4         },
    { DISPATCH_PROPERTYPUT, 19, "level"                , (Com_method_ptr)&Com_log::put_Level               , VT_EMPTY      , { VT_I4 } },
    { DISPATCH_PROPERTYGET, 19, "level"                , (Com_method_ptr)&Com_log::get_Level               , VT_I4         },
    { DISPATCH_PROPERTYGET, 20, "filename"             , (Com_method_ptr)&Com_log::get_Filename            , VT_BSTR       },
    { DISPATCH_PROPERTYPUT, 21, "collect_within"       , (Com_method_ptr)&Com_log::put_Collect_within      , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 21, "collect_within"       , (Com_method_ptr)&Com_log::get_Collect_within      , VT_R8         },
    { DISPATCH_PROPERTYPUT, 22, "collect_max"          , (Com_method_ptr)&Com_log::put_Collect_max         , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 22, "collect_max"          , (Com_method_ptr)&Com_log::get_Collect_max         , VT_R8         },
    { DISPATCH_PROPERTYPUT, 23, "mail_it"              , (Com_method_ptr)&Com_log::put_Mail_it             , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 24, "java_class_name"      , (Com_method_ptr)&Com_log::get_Java_class_name     , VT_BSTR },
    { DISPATCH_PROPERTYGET, 25, "last_error_line"      , (Com_method_ptr)&Com_log::get_Last_error_line     , VT_BSTR },
    { DISPATCH_PROPERTYGET, 26, "last"                 , (Com_method_ptr)&Com_log::get_Last                , VT_BSTR       , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 27, "mail_on_warning"      , (Com_method_ptr)&Com_log::put_Mail_on_warning     , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 27, "mail_on_warning"      , (Com_method_ptr)&Com_log::get_Mail_on_warning     , VT_BOOL       },
    { DISPATCH_METHOD     , 28, "Start_new_file"       , (Com_method_ptr)&Com_log::Start_new_file          , VT_EMPTY      },
    { DISPATCH_METHOD     , 29, "Log_file"             , (Com_method_ptr)&Com_log::Log_file                , VT_EMPTY      , { VT_BSTR } },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name          , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_reference_with_properties, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//-----------------------------------------------------------Com_log::get_reference_with_properties

ptr<object_server::Reference_with_properties> Com_log::get_reference_with_properties()   //( const IID& iid )
{
    ptr<object_server::Reference_with_properties> result;

    //if( iid != IID_Ilog )  return E_NOINTERFACE;

    
    THREAD_LOCK( _lock )
    {
        if( !_log )  throw_com( E_POINTER, "Com_log::get_reference_with_properties" );

        //*result = _log->get_reference_with_properties();
        result = Z_NEW( object_server::Reference_with_properties( CLSID_Com_log_proxy, (Ilog*)this ) );  // IDispatch* wird von Com_log eingesetzt.

        result->set_property( "level", _log->log_level() );
    }

    return result;
}

//---------------------------------------------------------------------------------Com_log::set_log

void Com_log::set_log( Prefix_log* log )
{ 
    THREAD_LOCK( _lock )
    {
        _log = log; 
    }
}

//----------------------------------------------------------------------------------------Com_log::
    
STDMETHODIMP Com_log::Debug9( BSTR line )                       { return Log( spooler_com::log_debug9, line ); }
STDMETHODIMP Com_log::Debug8( BSTR line )                       { return Log( spooler_com::log_debug8, line ); }
STDMETHODIMP Com_log::Debug7( BSTR line )                       { return Log( spooler_com::log_debug7, line ); }
STDMETHODIMP Com_log::Debug6( BSTR line )                       { return Log( spooler_com::log_debug6, line ); }
STDMETHODIMP Com_log::Debug5( BSTR line )                       { return Log( spooler_com::log_debug5, line ); }
STDMETHODIMP Com_log::Debug4( BSTR line )                       { return Log( spooler_com::log_debug4, line ); }
STDMETHODIMP Com_log::Debug3( BSTR line )                       { return Log( spooler_com::log_debug3, line ); }
STDMETHODIMP Com_log::Debug2( BSTR line )                       { return Log( spooler_com::log_debug2, line ); }
STDMETHODIMP Com_log::Debug1( BSTR line )                       { return Log( spooler_com::log_debug1, line ); }
STDMETHODIMP Com_log::Debug ( BSTR line )                       { return Log( spooler_com::log_debug1, line ); }
STDMETHODIMP Com_log::Msg   ( BSTR line )                       { return Log( spooler_com::log_info  , line ); }
STDMETHODIMP Com_log::Info  ( BSTR line )                       { return Log( spooler_com::log_info  , line ); }
STDMETHODIMP Com_log::Warn  ( BSTR line )                       { return Log( spooler_com::log_warn  , line ); }
STDMETHODIMP Com_log::Error ( BSTR line )                       { return Log( spooler_com::log_error , line ); }

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::Log( spooler_com::Log_level level, BSTR line )
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

//--------------------------------------------------------------------------------Com_log::Log_file

STDMETHODIMP Com_log::Log_file( BSTR path )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->log_file( bstr_as_string( path ) ); 
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_log::get_mail

STDMETHODIMP Com_log::get_Mail( Imail** mail )
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

STDMETHODIMP Com_log::put_Mail_on_error( VARIANT_BOOL b )
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

STDMETHODIMP Com_log::get_Mail_on_error( VARIANT_BOOL* b )
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

//---------------------------------------------------------------------Com_log::put_Mail_on_warning

STDMETHODIMP Com_log::put_Mail_on_warning( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_warning( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Com_log::get_Mail_on_warning

STDMETHODIMP Com_log::get_Mail_on_warning( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_warning();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}
    
//----------------------------------------------------------------------Com_log::put_mail_on_success

STDMETHODIMP Com_log::put_Mail_on_success( VARIANT_BOOL b )
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

STDMETHODIMP Com_log::get_Mail_on_success( VARIANT_BOOL* b )
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

STDMETHODIMP Com_log::put_Mail_on_process( int level )
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

STDMETHODIMP Com_log::get_Mail_on_process( int* result )
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

STDMETHODIMP Com_log::put_Level( int level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_log_level( (zschimmer::Log_level)level );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_log::level

STDMETHODIMP Com_log::get_Level( int* level )
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

STDMETHODIMP Com_log::get_Filename( BSTR* filename_bstr )
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

STDMETHODIMP Com_log::put_New_filename( BSTR filename_bstr )
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

STDMETHODIMP Com_log::get_New_filename( BSTR* filename_bstr )
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

STDMETHODIMP Com_log::put_Collect_within( VARIANT* time )
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

STDMETHODIMP Com_log::get_Collect_within( double* result )
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

STDMETHODIMP Com_log::put_Collect_max( VARIANT* time )
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

STDMETHODIMP Com_log::get_Collect_max( double* result )
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

STDMETHODIMP Com_log::put_Mail_it( VARIANT_BOOL b )
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

//---------------------------------------------------------------------Com_log::get_last_error_line

STDMETHODIMP Com_log::get_Last_error_line( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        hr = String_to_bstr( _log->last_error_line(), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::last_error_line" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::last_error_line" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_log::get_Last

STDMETHODIMP Com_log::get_Last( VARIANT* level, BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        hr = String_to_bstr( _log->last( make_log_level( string_from_variant( *level ) ) ), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::last_error_line" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::last_error_line" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_log::Start_new_file

STDMETHODIMP Com_log::Start_new_file()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->start_new_file();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------Com_log_proxy::Create_instance

HRESULT Com_log_proxy::Create_instance( const IID& iid, ptr<IUnknown>* result )
{
    if( iid == object_server::IID_Iproxy )
    {
        ptr<Com_log_proxy> instance = Z_NEW( Com_log_proxy );
        *result = static_cast<IDispatch*>( +instance );
        return S_OK;
    }

    return E_NOINTERFACE;
}

//----------------------------------------------------------------------Com_log_proxy::set_property

void Com_log_proxy::set_property( const string& name, const Variant& value )
{
    if( name == "level" )  _level = value.as_int();
    else  
        throw_xc( "Com_log_proxy::set_property", name );
}

//---------------------------------------------------------------------Com_log_proxy::GetIDsOfNames
/*
STDMETHODIMP Com_log_proxy::GetIDsOfNames( const IID& iid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* dispid )
{
    HRESULT hr;
    
    hr = com_get_dispid( _methods, iid, rszNames, cNames, lcid, dispid );       // Erst lokal versuchen

    if( hr == DISP_E_UNKNOWNNAME )
    {
        hr = Proxy::GetIDsOfNames( iid, rgszNames, cNames, lcid, dispid );      // Server aufrufen
    }

    return hr;
}
*/
//----------------------------------------------------------------------------Com_log_proxy::Invoke

STDMETHODIMP Com_log_proxy::Invoke( DISPID dispid, const IID& iid, LCID lcid, unsigned short flags, DISPPARAMS* dispparams, 
                                    VARIANT* result, EXCEPINFO* excepinfo, UINT* arg_nr )
{
    const Bstr& name = dispid == 0? "info" : name_from_dispid( dispid );

    if( name == "debug9" )  { if( _level > spooler_com::log_debug9 )  return S_FALSE; }
    else
    if( name == "debug8" )  { if( _level > spooler_com::log_debug8 )  return S_FALSE; }
    else
    if( name == "debug7" )  { if( _level > spooler_com::log_debug7 )  return S_FALSE; }
    else
    if( name == "debug6" )  { if( _level > spooler_com::log_debug6 )  return S_FALSE; }
    else
    if( name == "debug5" )  { if( _level > spooler_com::log_debug5 )  return S_FALSE; }
    else
    if( name == "debug4" )  { if( _level > spooler_com::log_debug4 )  return S_FALSE; }
    else
    if( name == "debug3" )  { if( _level > spooler_com::log_debug3 )  return S_FALSE; }
    else
    if( name == "debug2" )  { if( _level > spooler_com::log_debug2 )  return S_FALSE; }
    else
    if( name == "debug1" )  { if( _level > spooler_com::log_debug1 )  return S_FALSE; }
    else
    if( name == "debug"  )  { if( _level > spooler_com::log_debug  )  return S_FALSE; }
    else                                                                              
    if( name == "info"   )  { if( _level > spooler_com::log_info   )  return S_FALSE; }
    else
    if( name == "log"   )
    {
        if( dispparams->cArgs != 2 )  return DISP_E_BADPARAMCOUNT;
        if( int_from_variant( dispparams->rgvarg[ dispparams->cArgs - 1 ] ) < _level )  return S_FALSE;
    }
    else
    if( name == "level" )
    {
        if( flags & DISPATCH_PROPERTYPUT )
        {
            if( dispparams->cNamedArgs != 1 )  return DISP_E_BADPARAMCOUNT;
            if( dispparams->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT )   return DISP_E_BADPARAMCOUNT;
            _level = int_from_variant( dispparams->rgvarg[0] );
        }
        else 
        {
            if( dispparams->cNamedArgs != 0 )  return DISP_E_BADPARAMCOUNT;

            if( result )  result->vt = VT_I4, V_I4(result) = _level;

            return S_OK;
        }
    }

    return Proxy::Invoke( dispid, iid, lcid, flags, dispparams, result, excepinfo, arg_nr );      // Server aufrufen
}

//-----------------------------------------------------------------------------Com_log_proxy::level
/*
STDMETHODIMP Com_log_proxy::get_level( int* level )
{
    HRESULT hr = NOERROR;

    *level = _level;

    return hr;
}
*/
//-----------------------------------------------------------------------------Com_log_proxy::level
/*
STDMETHODIMP Com_log_proxy::put_level( int level )
{
    HRESULT hr = NOERROR;

    _level = level;

    return hr;
}
*/
//-------------------------------------------------------------------------------Com_log_proxy::log
/*
STDMETHODIMP Com_log_proxy::log( spooler_com::Log_level level, BSTR line )
{ 
    HRESULT hr = NOERROR;

    if( level < _level )  return hr;

    if( !_dispid_valid )
    {
        _object->GetIDsOfName()
        _disp_id_valid = true;
    }

    hr = _object->Invoke();
}
*/
//-------------------------------------------------------------------------Com_object_set::_methods
#ifdef Z_COM

const Com_method Com_object_set::_methods[] =
{ 
   // _flags         , dispid, _name                   , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "low_level"            , (Com_method_ptr)&Com_object_set::get_Low_level    , VT_I4         },
    { DISPATCH_PROPERTYGET,  2, "high_level"           , (Com_method_ptr)&Com_object_set::get_High_level   , VT_I4         },
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

STDMETHODIMP Com_object_set::get_Low_level( int* result )
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

STDMETHODIMP Com_object_set::get_High_level( int* result )
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
    { DISPATCH_METHOD     ,  1, "start_when_directory_changed"  , (Com_method_ptr)&Com_job::Start_when_directory_changed, VT_EMPTY      , { VT_BSTR, VT_BSTR }, 1 },
    { DISPATCH_METHOD     ,  2, "clear_when_directory_changed"  , (Com_method_ptr)&Com_job::Clear_when_directory_changed },
    { DISPATCH_METHOD     ,  3, "start"                         , (Com_method_ptr)&Com_job::Start                       , VT_DISPATCH   , { VT_BYREF|VT_VARIANT }, 1 },
  //{ DISPATCH_PROPERTYGET,  4, "thread"                        , (Com_method_ptr)&Com_job::get_Thread                  , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET,  5, "include_path"                  , (Com_method_ptr)&Com_job::get_Include_path            , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  6, "name"                          , (Com_method_ptr)&Com_job::get_Name                    , VT_BSTR       },
    { DISPATCH_METHOD     ,  7, "wake"                          , (Com_method_ptr)&Com_job::Wake                        },
    { DISPATCH_PROPERTYPUT,  8, "state_text"                    , (Com_method_ptr)&Com_job::put_State_text              , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  9, "title"                         , (Com_method_ptr)&Com_job::get_Title                   , VT_BSTR       },
    { DISPATCH_PROPERTYPUT, 10, "delay_after_error"             , (Com_method_ptr)&Com_job::put_Delay_after_error       , VT_EMPTY      , { VT_I4, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 11, "order_queue"                   , (Com_method_ptr)&Com_job::get_Order_queue             , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 12, "java_class_name"               , (Com_method_ptr)&Com_job::get_Java_class_name         , VT_BSTR },
    { DISPATCH_PROPERTYPUT, 13, "delay_order_after_setback"     , (Com_method_ptr)&Com_job::put_Delay_order_after_setback,VT_EMPTY      , { VT_I4, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 14, "max_order_setbacks"            , (Com_method_ptr)&Com_job::put_Max_order_setbacks      , VT_EMPTY      , { VT_I4 } },
    { DISPATCH_METHOD     , 15, "clear_delay_after_error"       , (Com_method_ptr)&Com_job::Clear_delay_after_error     , VT_EMPTY      },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//------------------------------------------------------------Com_job::start_when_directory_changed 

STDMETHODIMP Com_job::Start_when_directory_changed( BSTR directory_name, BSTR filename_pattern )
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

STDMETHODIMP Com_job::Clear_when_directory_changed()
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

STDMETHODIMP Com_job::Start( VARIANT* params, Itask** itask )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;

        ptr<Task>           task;

        ptr<Ivariable_set>  pars;
        Time                start_at = 0; 

        if( params  &&  params->vt != VT_EMPTY  &&  params->vt != VT_NULL  &&  params->vt != VT_ERROR )
        {
            if( params->vt != VT_DISPATCH && params->vt != VT_UNKNOWN )  return DISP_E_TYPEMISMATCH;
            hr = params->punkVal->QueryInterface( IID_Ivariable_set, pars.void_pp() );
            if( FAILED(hr) )  return hr;
        }

        Variant task_name_vt;
        if( pars )  pars->get_Var( Bstr("spooler_task_name"), &task_name_vt );
        hr = task_name_vt.ChangeType( VT_BSTR );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_task_name" );

        Variant start_after_vt;
        if( pars )  pars->get_Var( Bstr("spooler_start_after"), &start_after_vt );
        if( start_after_vt.vt != VT_EMPTY )
        {
            hr = start_after_vt.ChangeType( VT_R8 );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_start_after" );
            start_at = Time::now() + start_after_vt.dblVal;
        }

        //THREAD_LOCK( _job->_lock )
        {
            string name = bstr_as_string( task_name_vt.bstrVal );
            task = _job->start( pars, name, start_at, true );
        }

        *itask = new Com_task( task );
        (*itask)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }

    return hr;
}

//------------------------------------------------------------------------------------Com_job::wake

STDMETHODIMP Com_job::Wake()
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
STDMETHODIMP Com_job::get_Thread( Ithread** thread )
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

STDMETHODIMP Com_job::get_Include_path( BSTR* result )
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

STDMETHODIMP Com_job::get_Name( BSTR* result )
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

STDMETHODIMP Com_job::put_State_text( BSTR text )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );
      //if( current_thread_id() != _job->thread()->thread_id() )  return E_ACCESSDENIED;

        _job->set_state_text( bstr_as_string( text ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_job::get_title

STDMETHODIMP Com_job::get_Title( BSTR* title )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

        *title = SysAllocString_string( _job->title() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }

    return hr;
}

//-------------------------------------------------------------------Com_job::put_delay_after_error

STDMETHODIMP Com_job::put_Delay_after_error( int error_steps, VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

        if( time->vt == VT_BSTR  &&  stricmp( string_from_variant( *time ).c_str(), "stop" ) == 0 )
        {
            _job->set_stop_after_error( error_steps );
        }
        else
        {
            _job->set_delay_after_error( error_steps, time_from_variant(*time) );
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }

    return hr;
}

//-----------------------------------------------------------------Com_job::clear_delay_after_error

STDMETHODIMP Com_job::Clear_delay_after_error()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

        _job->clear_delay_after_error();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.clear_delay_after_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.clear_delay_after_error" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_job::get_order_queue

STDMETHODIMP Com_job::get_Order_queue( Iorder_queue** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

        *result = _job->order_queue();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.order_queue" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.order_queue" ); }

    return hr;
}

//-----------------------------------------------------------Com_job::put_delay_order_after_setback

STDMETHODIMP Com_job::put_Delay_order_after_setback( int setback_number, VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

        _job->set_delay_order_after_setback( setback_number, time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_order_after_setback" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_order_after_setback" ); }

    return hr;
}

//------------------------------------------------------------------Com_job::put_max_order_setbacks

STDMETHODIMP Com_job::put_Max_order_setbacks( int count )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SCHEDULER-122" );

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
    { DISPATCH_PROPERTYGET,  1, "object_set"                , (Com_method_ptr)&Com_task::get_Object_set         , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT,  2, "error"                     , (Com_method_ptr)&Com_task::put_Error              , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET,  2, "error"                     , (Com_method_ptr)&Com_task::get_Error              , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "job"                       , (Com_method_ptr)&Com_task::get_Job                , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "params"                    , (Com_method_ptr)&Com_task::get_Params             , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT,  5, "result"                    , (Com_method_ptr)&Com_task::put_Result             , VT_EMPTY      , { VT_BYREF|VT_DISPATCH } },
    { DISPATCH_PROPERTYGET,  5, "result"                    , (Com_method_ptr)&Com_task::get_Result             , VT_DISPATCH  },
    { DISPATCH_METHOD     ,  6, "wait_until_terminated"     , (Com_method_ptr)&Com_task::Wait_until_terminated  , VT_BOOL       , { VT_R8 } },
    { DISPATCH_PROPERTYPUT,  7, "repeat"                    , (Com_method_ptr)&Com_task::put_Repeat             , VT_EMPTY      , { VT_R8 } },
    { DISPATCH_METHOD     ,  8, "end"                       , (Com_method_ptr)&Com_task::End                    },
    { DISPATCH_PROPERTYPUT,  9, "history_field"             , (Com_method_ptr)&Com_task::put_History_field      , VT_EMPTY      , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 10, "id"                        , (Com_method_ptr)&Com_task::get_Id                 , VT_I4         },
    { DISPATCH_PROPERTYPUT, 11, "delay_spooler_process"     , (Com_method_ptr)&Com_task::put_Delay_spooler_process, VT_EMPTY    , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYPUT, 12, "close_engine"              , (Com_method_ptr)&Com_task::put_Close_engine       , VT_EMPTY      , { VT_BOOL } },
    { DISPATCH_PROPERTYGET, 13, "order"                     , (Com_method_ptr)&Com_task::get_Order              , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET, 14, "java_class_name"           , (Com_method_ptr)&Com_task::get_Java_class_name    , VT_BSTR       },
    { DISPATCH_PROPERTYGET, 15, "changed_directories"       , (Com_method_ptr)&Com_task::get_Changed_directories, VT_BSTR       },
    { DISPATCH_METHOD     , 16, "add_pid"                   , (Com_method_ptr)&Com_task::Add_pid                , VT_EMPTY      , { VT_INT, VT_BYREF|VT_VARIANT }, 1 },
    { DISPATCH_METHOD     , 17, "remove_pid"                , (Com_method_ptr)&Com_task::Remove_pid             , VT_EMPTY      , { VT_INT } },
    { DISPATCH_PROPERTYGET, 18, "stderr_text"               , (Com_method_ptr)&Com_task::get_Stderr_text        , VT_BSTR       },
    { DISPATCH_PROPERTYGET, 19, "stdout_text"               , (Com_method_ptr)&Com_task::get_Stdout_text        , VT_BSTR       },
    { DISPATCH_METHOD     , 20, "Create_subprocess"         , (Com_method_ptr)&Com_task::Create_subprocess      , VT_DISPATCH   , { VT_BYREF|VT_VARIANT }, 1 },
    { DISPATCH_METHOD     , 21, "Add_subprocess"            , (Com_method_ptr)&Com_task::Add_subprocess         , VT_EMPTY      , { VT_INT, VT_R8, VT_BOOL, VT_BOOL, VT_BSTR }, 1 },
    { DISPATCH_PROPERTYPUT, 22, "Priority_class"            , (Com_method_ptr)&Com_task::put_Priority_class     , VT_EMPTY      , { VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 22, "Priority_class"            , (Com_method_ptr)&Com_task::get_Priority_class     , VT_BSTR       },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name          , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_reference_with_properties, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//----------------------------------------------------------Com_task::get_reference_with_properties

ptr<object_server::Reference_with_properties> Com_task::get_reference_with_properties()
{
    ptr<object_server::Reference_with_properties> result;

    THREAD_LOCK( _lock )
    {
        if( !_task )  throw_com( E_POINTER, "Com_task::get_reference_with_properties" );

        result = Z_NEW( object_server::Reference_with_properties( CLSID_Task_proxy, static_cast<Itask*>( this ) ) );
    }

    return result;
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

STDMETHODIMP Com_task::get_Object_set( Iobject_set** )
{
    return E_NOTIMPL;
/*
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
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

STDMETHODIMP Com_task::put_Error( VARIANT* error_par )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        Variant error_vt = *error_par;
        hr = error_vt.ChangeType( VT_BSTR );        if( FAILED(hr) )  return hr;

        string error_text = bstr_as_string( error_vt.bstrVal );
        _task->set_error( Xc( "SCHEDULER-120", error_text.c_str() ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::get_Error( Ierror** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        THREAD_LOCK( _task->_lock )  *result = new Com_error( _task->error() );
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_task::get_job

STDMETHODIMP Com_task::get_Job( Ijob** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        *result = _task->_job->com_job();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_params

STDMETHODIMP Com_task::get_Params( Ivariable_set** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        *result = _task->_params;
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }

    return hr;
}

//------------------------------------------------------------------Com_task::wait_until_terminated

STDMETHODIMP Com_task::Wait_until_terminated( double wait_time, VARIANT_BOOL* ok )
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

STDMETHODIMP Com_task::End()
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

STDMETHODIMP Com_task::put_Result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SCHEDULER-138" );

        THREAD_LOCK( _task->_lock )  hr = _task->_result.Copy( value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_result

STDMETHODIMP Com_task::get_Result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        VariantInit( value ); 
        THREAD_LOCK( _task->_job->_lock )  hr = VariantCopy( value, &_task->_result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_repeat

STDMETHODIMP Com_task::put_Repeat( double seconds )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SCHEDULER-138" );

        _task->_job->set_repeat( seconds );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }

    return hr;
}

//----------------------------------------------------------------------Com_task::put_history_field

STDMETHODIMP Com_task::put_History_field( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
      //if( !_task->_job->its_current_task(_task) )  throw_xc( "SCHEDULER-138" );

        _task->set_history_field( bstr_as_string(name), *value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_task::get_id

STDMETHODIMP Com_task::get_Id( int* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        *result = _task->id();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.id" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.id" ); }

    return hr;
}

//--------------------------------------------------------------Com_task::put_delay_spooler_process

STDMETHODIMP Com_task::put_Delay_spooler_process( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        _task->set_delay_spooler_process( time_from_variant( *time ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.delay_spooler_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.delay_spooler_process" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_task::put_close_engine

STDMETHODIMP Com_task::put_Close_engine( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        _task->set_close_engine( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.close_engine" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.close_engine" ); }

    return hr;
}

//------------------------------------------------------------------------------Com_task::get_order

STDMETHODIMP Com_task::get_Order( Iorder** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        *result = _task->order();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//----------------------------------------------------------------Com_task::get_changed_directories

STDMETHODIMP Com_task::get_Changed_directories( BSTR* result )
{
    HRESULT hr = S_OK;

    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        hr = String_to_bstr( _task->_changed_directories, result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_task::add_pid

STDMETHODIMP Com_task::Add_pid( int pid, VARIANT* timeout )
{
    Z_LOG( __PRETTY_FUNCTION__ << "(" << pid << "," << debug_string_from_variant( *timeout ) << ")\n" );
    
    HRESULT hr = S_OK;
    
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        Time t  = timeout->vt == VT_EMPTY || com::variant_is_missing( *timeout )? latter_day 
                                                                                : time_from_variant( *timeout );
        _task->add_pid( pid, t );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//-----------------------------------------------------------------------------Com_task::remove_pid

STDMETHODIMP Com_task::Remove_pid( int pid )
{
    Z_LOG( __PRETTY_FUNCTION__ << "(" << pid << ")\n" );
    
    HRESULT hr = S_OK;
    
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;

        _task->remove_pid( pid );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//--------------------------------------------------------------Com_task::get_Stderr_or_stdout_text

STDMETHODIMP Com_task::get_Stderr_or_stdout_text( BSTR* result, bool get_stderr )
{
    HRESULT hr = S_OK;
    
    try
    {
        *result = NULL;

        if( !_task )  throw_xc( "SCHEDULER-122" );
        if( !_task->thread()  ||  current_thread_id() != _task->thread()->thread_id() )  return E_ACCESSDENIED;
        if( !_task->_module_instance )  return S_FALSE;

        string filename = get_stderr? _task->_module_instance->stderr_filename() 
                                    : _task->_module_instance->stdout_filename();
        if( filename == "" )  return S_FALSE;

        return String_to_bstr( string_from_file( filename ), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//------------------------------------------------------------------------Com_task::get_Stderr_text

STDMETHODIMP Com_task::get_Stderr_text( BSTR* result )
{
    Z_LOG( __PRETTY_FUNCTION__ << "()\n" );

    return get_Stderr_or_stdout_text( result, true );
}

//------------------------------------------------------------------------Com_task::get_Stdout_text

STDMETHODIMP Com_task::get_Stdout_text( BSTR* result )
{
    Z_LOG( __PRETTY_FUNCTION__ << "()\n" );
    
    return get_Stderr_or_stdout_text( result, false );
}

//----------------------------------------------------------------------Com_task::Create_subprocess

STDMETHODIMP Com_task::Create_subprocess( VARIANT* program_and_parameters, Isubprocess** result )
{
    Z_LOG( __PRETTY_FUNCTION__ << "(" << debug_string_from_variant( *program_and_parameters ) << "\n" );
    if( !_task )  return E_POINTER;

    return _task->_subprocess_register.Create_subprocess( program_and_parameters, result, this );
}

//--------------------------------------------------------------------Com_task::Register_subprocess
// Wird aufgerufen von Com_task_proxy

STDMETHODIMP Com_task::Add_subprocess( int pid, double timeout, VARIANT_BOOL ignore_error, VARIANT_BOOL ignore_signal, BSTR title  )
{
    Z_LOG( __PRETTY_FUNCTION__ << "(" << pid << ',' << timeout << ',' << ignore_error << ',' << ignore_signal << ',' << Bstr(title) << ")\n" );
    HRESULT hr = S_OK;
    
    try
    {
        if( !_task )  throw_xc( "SCHEDULER-122" );

        _task->add_subprocess( pid, 
                               timeout, 
                               ignore_error? true : false, 
                               ignore_signal? true : false, 
                               string_from_bstr( title ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//---------------------------------------------------------------------Com_task::get_Priority_class

STDMETHODIMP Com_task::get_Priority_class( BSTR* result )
{
    Z_COM_IMPLEMENT( hr = zschimmer::Process( getpid() ).get_Priority_class( result ) );
}

//-------------------------------------------------------------------------Com_task_proxy::_methods
// Dispid wie bei Com_task!

const Com_method Com_task_proxy::_methods[] =
{ 
#if defined COM_METHOD
    COM_METHOD      ( Com_task_proxy, 20, Create_subprocess     , VT_DISPATCH , 1, VT_BYREF|VT_VARIANT ),
    COM_PROPERTY_PUT( Com_task_proxy, 22, Priority_class        ,               0, VT_BYREF|VT_VARIANT ),
    COM_PROPERTY_GET( Com_task_proxy, 22, Priority_class        , VT_BSTR     , 0 ),
#endif
    {}
};

//------------------------------------------------------------------Com_task_proxy::Create_instance

HRESULT Com_task_proxy::Create_instance( const IID& iid, ptr<IUnknown>* result )
{
    if( iid == object_server::IID_Iproxy )
    {
        ptr<Com_task_proxy> instance = Z_NEW( Com_task_proxy );
        *result = static_cast<IDispatch*>( static_cast<Proxy*>( +instance ) );
        return S_OK;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------Com_task_proxy::Com_task_proxy

Com_task_proxy::Com_task_proxy()
: 
    Proxy_with_local_methods( &class_descriptor ),
    _subprocess_register( Z_NEW( Subprocess_register ) )
{
}

//----------------------------------------------------------------Com_task_proxy::Create_subprocess

STDMETHODIMP Com_task_proxy::Create_subprocess( VARIANT* program_and_parameters, Isubprocess** result )
{
    return _subprocess_register->Create_subprocess( program_and_parameters, result, static_cast<Idispatch_implementation*>( this ) );
}

//---------------------------------------------------------------Com_task_proxy::put_Priority_class

STDMETHODIMP Com_task_proxy::put_Priority_class( VARIANT* priority )
{
    Z_COM_IMPLEMENT( hr = zschimmer::Process( getpid() ).put_Priority_class( priority ) );
}

//---------------------------------------------------------------Com_task_proxy::get_Priority_class

STDMETHODIMP Com_task_proxy::get_Priority_class( BSTR* result )
{
    Z_COM_IMPLEMENT( hr = zschimmer::Process( getpid() ).get_Priority_class( result ) );
}

//------------------------------------------------------------Com_task_proxy::wait_for_subprocesses

void Com_task_proxy::wait_for_subprocesses()
{
    _subprocess_register->wait();
}

//-----------------------------------------------------------------------------Com_thread::_methods
/*
#ifdef Z_COM

const Com_method Com_thread::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_thread::get_Log              , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "script"                    , (Com_method_ptr)&Com_thread::get_Script           , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "include_path"              , (Com_method_ptr)&Com_thread::get_Include_path     , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  4, "name"                      , (Com_method_ptr)&Com_thread::get_Name             , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  5, "java_class_name"           , (Com_method_ptr)&Com_thread::get_Java_class_name  , VT_BSTR },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

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
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_spooler::get_Log             , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "id"                        , (Com_method_ptr)&Com_spooler::get_Id              , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  3, "param"                     , (Com_method_ptr)&Com_spooler::get_Param           , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  4, "script"                    , (Com_method_ptr)&Com_spooler::get_Script          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  5, "job"                       , (Com_method_ptr)&Com_spooler::get_Job             , VT_DISPATCH  , { VT_BSTR } },
    { DISPATCH_METHOD     ,  6, "create_variable_set"       , (Com_method_ptr)&Com_spooler::Create_variable_set , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  7, "include_path"              , (Com_method_ptr)&Com_spooler::get_Include_path    , VT_BSTR      },
    { DISPATCH_PROPERTYGET,  8, "log_dir"                   , (Com_method_ptr)&Com_spooler::get_Log_dir         , VT_BSTR      },
    { DISPATCH_METHOD     ,  9, "let_run_terminate_and_restart", (Com_method_ptr)&Com_spooler::Let_run_terminate_and_restart },
    { DISPATCH_PROPERTYGET, 10, "variables"                 , (Com_method_ptr)&Com_spooler::get_Variables       , VT_DISPATCH  },
    { DISPATCH_PROPERTYPUT, 11, "var"                       , (Com_method_ptr)&Com_spooler::put_Var             , VT_EMPTY     , { VT_BSTR, VT_BYREF|VT_VARIANT } },
    { DISPATCH_PROPERTYGET, 11, "var"                       , (Com_method_ptr)&Com_spooler::get_Var             , VT_VARIANT   , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 12, "db_name"                   , (Com_method_ptr)&Com_spooler::get_Db_name         , VT_BSTR      },
    { DISPATCH_METHOD     , 13, "create_job_chain"          , (Com_method_ptr)&Com_spooler::Create_job_chain    , VT_DISPATCH  },
    { DISPATCH_METHOD     , 14, "add_job_chain"             , (Com_method_ptr)&Com_spooler::Add_job_chain       , VT_EMPTY     , { VT_DISPATCH } },
    { DISPATCH_PROPERTYGET, 15, "job_chain"                 , (Com_method_ptr)&Com_spooler::get_Job_chain       , VT_DISPATCH  , { VT_BSTR } },
    { DISPATCH_METHOD     , 16, "create_order"              , (Com_method_ptr)&Com_spooler::Create_order        , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET, 17, "is_service"                , (Com_method_ptr)&Com_spooler::get_Is_service      , VT_BOOL      },
    { DISPATCH_PROPERTYGET, 18, "java_class_name"           , (Com_method_ptr)&Com_spooler::get_Java_class_name , VT_BSTR      },
    { DISPATCH_PROPERTYGET, 19, "directory"                 , (Com_method_ptr)&Com_spooler::get_Directory       , VT_BSTR      },
    { DISPATCH_METHOD     , 20, "job_chain_exists"          , (Com_method_ptr)&Com_spooler::Job_chain_exists    , VT_BOOL       , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 21, "hostname"                  , (Com_method_ptr)&Com_spooler::get_Hostname        , VT_BSTR      },
    { DISPATCH_METHOD     , 22, "abort_immediately"         , (Com_method_ptr)&Com_spooler::Abort_immediately   , VT_BSTR      },
    { DISPATCH_METHOD     , 23, "abort_immediately_and_restart", (Com_method_ptr)&Com_spooler::Abort_immediately_and_restart, VT_BSTR },
    { DISPATCH_PROPERTYGET, 24, "Db_variables_table_name"   , (Com_method_ptr)&Com_spooler::get_Db_variables_table_name, VT_BSTR      },
    { DISPATCH_PROPERTYGET, 25, "Db_tasks_table_name"       , (Com_method_ptr)&Com_spooler::get_Db_tasks_table_name    , VT_BSTR      },
    { DISPATCH_PROPERTYGET, 26, "Db_orders_table_name"      , (Com_method_ptr)&Com_spooler::get_Db_orders_table_name   , VT_BSTR      },
    { DISPATCH_PROPERTYGET, 27, "Db_history_table_name"     , (Com_method_ptr)&Com_spooler::get_Db_history_table_name  , VT_BSTR      },
    { DISPATCH_PROPERTYGET, 28, "Db_order_history_table_name", (Com_method_ptr)&Com_spooler::get_Db_order_history_table_name, VT_BSTR      },
    { DISPATCH_PROPERTYGET, 29, "Ini_path"                  , (Com_method_ptr)&Com_spooler::get_Ini_path         , VT_BSTR      },
    { DISPATCH_METHOD     , 30, "Execute_xml"               , (Com_method_ptr)&Com_spooler::Execute_xml          , VT_BSTR      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 31, "Tcp_port"                  , (Com_method_ptr)&Com_spooler::get_Tcp_port         , VT_INT       },
    { DISPATCH_PROPERTYGET, 32, "Udp_port"                  , (Com_method_ptr)&Com_spooler::get_Udp_port         , VT_INT       },
    { DISPATCH_METHOD     , 33, "Create_xslt_stylesheet"    , (Com_method_ptr)&Com_spooler::Create_xslt_stylesheet, VT_DISPATCH },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name          , result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//-----------------------------------------------------------------------------Com_spooler::get_Log

STDMETHODIMP Com_spooler::get_Log( Ilog** com_log )
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

STDMETHODIMP Com_spooler::get_Id( BSTR* id_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *id_bstr = SysAllocString_string( _spooler->id() );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------------Com_spooler::param

STDMETHODIMP Com_spooler::get_Param( BSTR* param_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *param_bstr = SysAllocString_string( _spooler->_spooler_param );
    }

    return NOERROR;
}

//---------------------------------------------------------------------------Com_spooler::get_script

STDMETHODIMP Com_spooler::get_Script( IDispatch** script_object )
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

STDMETHODIMP Com_spooler::get_Job( BSTR job_name, Ijob** com_job )
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

STDMETHODIMP Com_spooler::Create_variable_set( Ivariable_set** result )
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

STDMETHODIMP Com_spooler::get_Include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_include_path );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------Com_spooler::get_log_dir

STDMETHODIMP Com_spooler::get_Log_dir( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_log_directory );
    }

    return NOERROR;
}

//-------------------------------------------------------Com_spooler::let_run_terminate_and_restart

STDMETHODIMP Com_spooler::Let_run_terminate_and_restart()
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        _spooler->cmd_let_run_terminate_and_restart();
    }

    return NOERROR;
}

//-----------------------------------------------------------------------Com_spooler::get_variables

STDMETHODIMP Com_spooler::get_Variables( Ivariable_set** result )
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

STDMETHODIMP Com_spooler::put_Var( BSTR name, VARIANT* value )
{
    HRESULT     hr;
    const char* crash_string = "*CRASH SCHEDULER*";
    static int  dummy;


    if( ( name == NULL || SysStringLen(name) == 0 )  &&  string_from_variant(*value) == crash_string )
    {
        _spooler->_log.error( "spooler.var(\"\")=\"" + string(crash_string) + "\"  lässt Scheduler jetzt abbrechen." );
        dummy = *(int*)NULL;
    }

    ptr<Ivariable_set> variables;

    hr = get_Variables( variables.pp() );  if( FAILED(hr) )  return hr;

    return variables->put_Var( name, value );
}

//-----------------------------------------------------------------------------Com_spooler::get_var

STDMETHODIMP Com_spooler::get_Var( BSTR name, VARIANT* value )
{
    HRESULT hr;
    ptr<Ivariable_set> variables;

    hr = get_Variables( variables.pp() );  if( FAILED(hr) )  return hr;

    return variables->get_Var( name, value );
}

//-------------------------------------------------------------------------Com_spooler::get_db_name

STDMETHODIMP Com_spooler::get_Db_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_db_name );
    }

    return NOERROR;
}

//--------------------------------------------------------------------Com_spooler::create_job_chain

STDMETHODIMP Com_spooler::Create_job_chain( spooler_com::Ijob_chain** result )
{
    ptr<Job_chain> job_chain = new Job_chain( _spooler );

    *result = job_chain;
    (*result)->AddRef();

    return S_OK;
}

//-----------------------------------------------------------------------Com_spooler::add_job_chain

STDMETHODIMP Com_spooler::Add_job_chain( spooler_com::Ijob_chain* job_chain )
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

STDMETHODIMP Com_spooler::get_Job_chain( BSTR name, spooler_com::Ijob_chain** result )
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

STDMETHODIMP Com_spooler::Create_order( Iorder** result )
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

STDMETHODIMP Com_spooler::get_Is_service( VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *result = _spooler->is_service();
    }

    return hr;
}

//---------------------------------------------------------------------------spooler::get_directory

STDMETHODIMP Com_spooler::get_Directory( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        hr = String_to_bstr( _spooler->directory(), result );
    }

    return hr;
}

//-----------------------------------------------------------------------Com_spooler::get_hostname

STDMETHODIMP Com_spooler::get_Hostname( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;

        hr = String_to_bstr( _spooler->_hostname, result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.get_hostname" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.get_hostname" ); }

    return hr;
}

//--------------------------------------------------------------------Com_spooler::job_chain_exists

STDMETHODIMP Com_spooler::Job_chain_exists( BSTR name, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;

        *result = _spooler->job_chain_or_null( string_from_bstr(name) ) != NULL;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.job_chain_exists" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.job_chain_exists" ); }

    return hr;
}

//-------------------------------------------------------------------Com_spooler::abort_immediately

STDMETHODIMP Com_spooler::Abort_immediately()
{
    HRESULT hr = S_OK;

    if( !_spooler )  return E_POINTER;

    try
    {
        _spooler->abort_immediately();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------Com_spooler::abort_immediately_and_restart

STDMETHODIMP Com_spooler::Abort_immediately_and_restart()
{
    HRESULT hr = S_OK;

    if( !_spooler )  return E_POINTER;

    try
    {
        _spooler->abort_immediately( true );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------Com_spooler::get_Db_variables_table_name

STDMETHODIMP Com_spooler::get_Db_variables_table_name( BSTR* result )                    
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_variables_tablename, result ); 
}

//-------------------------------------------------------------Com_spooler::get_Db_tasks_table_name

STDMETHODIMP Com_spooler::get_Db_tasks_table_name( BSTR* result )
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_tasks_tablename, result ); 
}

//------------------------------------------------------------Com_spooler::get_Db_orders_table_name

STDMETHODIMP Com_spooler::get_Db_orders_table_name( BSTR* result )                    
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_orders_tablename, result ); 
}

//-----------------------------------------------------------Com_spooler::get_Db_history_table_name

STDMETHODIMP Com_spooler::get_Db_history_table_name( BSTR* result )                    
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_job_history_tablename, result ); 
}

//-----------------------------------------------------Com_spooler::get_Db_order_history_table_name

STDMETHODIMP Com_spooler::get_Db_order_history_table_name( BSTR* result )                    
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_order_history_tablename, result ); 
}

//------------------------------------------------------------------------Com_spooler::get_Ini_path

STDMETHODIMP Com_spooler::get_Ini_path( BSTR* result )                    
{ 
    if( !_spooler )  return E_POINTER;
    return String_to_bstr( _spooler->_factory_ini, result ); 
}

//-------------------------------------------------------------------------Com_spooler::Execute_xml

STDMETHODIMP Com_spooler::Execute_xml( BSTR xml, BSTR* result )                    
{ 
    HRESULT hr = S_OK;

    if( !_spooler )  return E_POINTER;

    try
    {
        Command_processor cp ( _spooler );
        cp.set_host( NULL );
        hr = String_to_bstr( cp.execute( string_from_bstr( xml ), Time::now() ), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//------------------------------------------------------------------------Com_spooler::get_Tcp_port

STDMETHODIMP Com_spooler::get_Tcp_port( int* result )
{
    if( !_spooler )  return E_POINTER;
    *result = _spooler->_tcp_port;

    return S_OK;
}

//------------------------------------------------------------------------Com_spooler::get_Udp_port

STDMETHODIMP Com_spooler::get_Udp_port( int* result )
{
    if( !_spooler )  return E_POINTER;
    *result = _spooler->_udp_port;

    return S_OK;
}

//--------------------------------------------------------------Com_spooler::Create_xslt_stylesheet

HRESULT Com_spooler::Create_xslt_stylesheet( Ixslt_stylesheet** result )
{
    HRESULT               hr         = S_OK;
    ptr<Xslt_stylesheet>  stylesheet = Z_NEW( Xslt_stylesheet );
    
    *result = stylesheet.take();
    return hr;
}

//----------------------------------------------------------------------------Com_context::_methods
#ifdef Z_COM

const Com_method Com_context::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                           , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "log"                       , (Com_method_ptr)&Com_context::get_Log             , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  2, "spooler"                   , (Com_method_ptr)&Com_context::get_Spooler         , VT_DISPATCH  },
  //{ DISPATCH_PROPERTYGET,  3, "thread"                    , (Com_method_ptr)&Com_context::get_Thread          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "job"                       , (Com_method_ptr)&Com_context::get_Job             , VT_DISPATCH  },
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
   // _flags         , dispid, _name                        , _method                                            , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT,  1, "name"                      , (Com_method_ptr)&Com_job_chain::put_Name           , VT_EMPTY      , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  1, "name"                      , (Com_method_ptr)&Com_job_chain::get_Name           , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  2, "order_count"               , (Com_method_ptr)&Com_job_chain::get_Order_count    , VT_INT        },
    { DISPATCH_METHOD     ,  3, "add_job"                   , (Com_method_ptr)&Com_job_chain::Add_job            , VT_EMPTY      , { VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF }, 3 },
    { DISPATCH_METHOD     ,  4, "add_end_state"             , (Com_method_ptr)&Com_job_chain::Add_end_state      , VT_EMPTY      , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_METHOD     ,  5, "add_order"                 , (Com_method_ptr)&Com_job_chain::Add_order          , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  6, "node"                      , (Com_method_ptr)&Com_job_chain::get_Node           , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  7, "order_queue"               , (Com_method_ptr)&Com_job_chain::get_Order_queue    , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  8, "java_class_name"           , (Com_method_ptr)&Com_job_chain::get_Java_class_name, VT_BSTR },
    { DISPATCH_PROPERTYPUT,  9, "store_orders_in_database"  , (Com_method_ptr)&Com_job_chain::put_Store_orders_in_database, VT_EMPTY, { VT_BOOL } },
    { DISPATCH_PROPERTYGET,  9, "store_orders_in_database"  , (Com_method_ptr)&Com_job_chain::get_Store_orders_in_database, VT_BOOL },
    { DISPATCH_METHOD     , 10, "remove_all_pending_orders" , (Com_method_ptr)&Com_job_chain::Remove_all_pending_orders   , VT_INT  },
    { DISPATCH_METHOD     , 11, "Try_add_order"             , (Com_method_ptr)&Com_job_chain::Try_add_order      , VT_BOOL       , { VT_DISPATCH } },
    { DISPATCH_METHOD     , 12, "Add_or_replace_order"      , (Com_method_ptr)&Com_job_chain::Add_or_replace_order, VT_EMPTY     , { VT_DISPATCH } },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

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

STDMETHODIMP Com_job_chain::put_Name( BSTR name_bstr )
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

STDMETHODIMP Com_job_chain::get_Name( BSTR* result )
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

STDMETHODIMP Com_job_chain::get_Order_count( int* result )
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

STDMETHODIMP Com_job_chain::Add_job( VARIANT* job_or_jobname, VARIANT* begin_state, VARIANT* end_state, VARIANT* error_state )
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

STDMETHODIMP Com_job_chain::Add_end_state( VARIANT* state )
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

STDMETHODIMP Com_job_chain::Add_order( VARIANT* order_or_payload, spooler_com::Iorder** result )
{
    HRESULT hr = NOERROR;

    //LOGI( "Job_chain.add_order\n" );

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SCHEDULER-151" );

        ptr<spooler_com::Iorder> iorder = order_from_order_or_payload( _job_chain->_spooler, *order_or_payload );
        if( !iorder )  return E_POINTER;

        Order* order = dynamic_cast<Order*>( &*iorder );
        if( !order )  return E_INVALIDARG;

        // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
        order->add_to_job_chain( dynamic_cast<Job_chain*>( this ) );  

        *result = iorder;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_order" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.add_order" ); }

    //LOG( "Job_chain.add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//--------------------------------------------------------------Com_job_chain::Add_or_replace_order

STDMETHODIMP Com_job_chain::Add_or_replace_order( spooler_com::Iorder* iorder )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SCHEDULER-151" );

        Order* order = dynamic_cast<Order*>( &*iorder );
        if( !order )  return E_INVALIDARG;

        // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
        order->add_to_or_replace_in_job_chain( dynamic_cast<Job_chain*>( this ) );  
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    //LOG( "Job_chain.add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//---------------------------------------------------------------------Com_job_chain::Try_add_order

STDMETHODIMP Com_job_chain::Try_add_order( Iorder* iorder, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    //LOGI( "Job_chain.add_order\n" );

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SCHEDULER-151" );
        if( !iorder )  return E_POINTER;

        Order* order = dynamic_cast<Order*>( &*iorder );
        if( !order )  return E_INVALIDARG;

        // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
        *result = order->try_add_to_job_chain( dynamic_cast<Job_chain*>( this ) )? VARIANT_FALSE : VARIANT_TRUE;  
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    //LOG( "Job_chain.add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//-------------------------------------------------------------------Com_job_chain::get_order_queue

STDMETHODIMP Com_job_chain::get_Order_queue( VARIANT* state, Iorder_queue** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SCHEDULER-151" );

        *result = _job_chain->node_from_state( *state )->_job->order_queue();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_queue" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.order_queue" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_job_chain::node

STDMETHODIMP Com_job_chain::get_Node( VARIANT* state, Ijob_chain_node** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job_chain )  return E_POINTER;
        if( !_job_chain->finished() )  throw_xc( "SCHEDULER-151" );

        *result = _job_chain->node_from_state( *state );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }

    return hr;
}

//------------------------------------------------------Com_job_chain::put_Store_orders_in_database

STDMETHODIMP Com_job_chain::put_Store_orders_in_database( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_job_chain )  return E_POINTER;

        _job_chain->set_store_orders_in_database( b != VARIANT_FALSE );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job_chain.node" ); }

    return hr;
}

//------------------------------------------------------Com_job_chain::get_Store_orders_in_database

STDMETHODIMP Com_job_chain::get_Store_orders_in_database( VARIANT_BOOL* result )
{
    if( !_job_chain )  return E_POINTER;

    *result = _job_chain->_store_orders_in_database? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

//---------------------------------------------------------Com_job_chain::Remove_all_pending_orders

STDMETHODIMP Com_job_chain::Remove_all_pending_orders( int* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_job_chain )  return E_POINTER;

        *result = _job_chain->remove_all_pending_orders();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Com_job_chain_node::_methods
#ifdef Z_COM

const Com_method Com_job_chain_node::_methods[] =
{ 
   // _flags         , dispid, _name                        , _method                                               , _result_type  , _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "state"                     , (Com_method_ptr)&Com_job_chain_node::get_State        , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  2, "next_node"                 , (Com_method_ptr)&Com_job_chain_node::get_Next_node    , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  3, "error_node"                , (Com_method_ptr)&Com_job_chain_node::get_Error_node   , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  4, "job"                       , (Com_method_ptr)&Com_job_chain_node::get_Job          , VT_DISPATCH  },
    { DISPATCH_PROPERTYGET,  5, "next_state"                , (Com_method_ptr)&Com_job_chain_node::get_Next_state   , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  6, "error_state"               , (Com_method_ptr)&Com_job_chain_node::get_Error_state  , VT_VARIANT   },
    { DISPATCH_PROPERTYGET,  7, "java_class_name"           , (Com_method_ptr)&Com_job_chain_node::get_Java_class_name, VT_BSTR },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//--------------------------------------------------------------------Com_job_chain_node::get_state

STDMETHODIMP Com_job_chain_node::get_State( VARIANT* result ) 
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_state );
}

//----------------------------------------------------------------Com_job_chain_node::get_next_node

STDMETHODIMP Com_job_chain_node::get_Next_node( Ijob_chain_node** result )
{ 
    *result = ((Job_chain_node*)(this))->_next_node;
    if( *result )  (*result)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------Com_job_chain_node::get_error_node

STDMETHODIMP Com_job_chain_node::get_Error_node( Ijob_chain_node** result )   
{ 
    *result = ((Job_chain_node*)(this))->_error_node; 
    if( *result )  (*result)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------Com_job_chain_node::get_next_state

STDMETHODIMP Com_job_chain_node::get_Next_state( VARIANT* result )
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_next_state );
}

//--------------------------------------------------------------Com_job_chain_node::get_error_state

STDMETHODIMP Com_job_chain_node::get_Error_state( VARIANT* result )   
{ 
    return VariantCopy( result, &((Job_chain_node*)(this))->_error_state );
}

//----------------------------------------------------------------------Com_job_chain_node::get_job

STDMETHODIMP Com_job_chain_node::get_Job( Ijob** result )              
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
    { DISPATCH_PROPERTYPUT,  1, "id"                        , (Com_method_ptr)&Com_order::put_Id                , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  1, "id"                        , (Com_method_ptr)&Com_order::get_Id                , VT_VARIANT    },
    { DISPATCH_PROPERTYPUT,  2, "title"                     , (Com_method_ptr)&Com_order::put_Title             , VT_EMPTY      , { VT_BSTR     } },
    { DISPATCH_PROPERTYGET,  2, "title"                     , (Com_method_ptr)&Com_order::get_Title             , VT_BSTR       },
    { DISPATCH_PROPERTYPUT,  3, "priority"                  , (Com_method_ptr)&Com_order::put_Priority          , VT_EMPTY      , { VT_INT      } },
    { DISPATCH_PROPERTYGET,  3, "priority"                  , (Com_method_ptr)&Com_order::get_Priority          , VT_INT        },
    { DISPATCH_PROPERTYGET,  4, "job_chain"                 , (Com_method_ptr)&Com_order::get_Job_chain         , VT_DISPATCH   },
    { DISPATCH_PROPERTYGET,  5, "job_chain_node"            , (Com_method_ptr)&Com_order::get_Job_chain_node    , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUTREF,6,"job"                       , (Com_method_ptr)&Com_order::putref_Job            , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYPUT,  6, "job"                       , (Com_method_ptr)&Com_order::put_Job               , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  6, "job"                       , (Com_method_ptr)&Com_order::get_Job               , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUT,  7, "state"                     , (Com_method_ptr)&Com_order::put_State             , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET,  7, "state"                     , (Com_method_ptr)&Com_order::get_State             , VT_VARIANT    },
    { DISPATCH_PROPERTYPUT,  8, "state_text"                , (Com_method_ptr)&Com_order::put_State_text        , VT_EMPTY      , { VT_BSTR     } },
    { DISPATCH_PROPERTYGET,  8, "state_text"                , (Com_method_ptr)&Com_order::get_State_text        , VT_BSTR       },
    { DISPATCH_PROPERTYGET,  9, "error"                     , (Com_method_ptr)&Com_order::get_Error             , VT_DISPATCH   },
    { DISPATCH_PROPERTYPUTREF,10,"payload"                  , (Com_method_ptr)&Com_order::putref_Payload        , VT_EMPTY      , { VT_DISPATCH } },
    { DISPATCH_PROPERTYPUT, 10, "payload"                   , (Com_method_ptr)&Com_order::put_Payload           , VT_EMPTY      , { VT_VARIANT|VT_BYREF  } },
    { DISPATCH_PROPERTYGET, 10, "payload"                   , (Com_method_ptr)&Com_order::get_Payload           , VT_VARIANT    },
    { DISPATCH_METHOD     , 11, "payload_is_type"           , (Com_method_ptr)&Com_order::Payload_is_type       , VT_BOOL       , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 12, "java_class_name"           , (Com_method_ptr)&Com_order::get_Java_class_name   , VT_BSTR },
    { DISPATCH_METHOD     , 13, "setback"                   , (Com_method_ptr)&Com_order::Setback               , VT_EMPTY      },
    { DISPATCH_PROPERTYPUT, 14, "at"                        , (Com_method_ptr)&Com_order::put_At                , VT_EMPTY      , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET, 14, "at"                        , (Com_method_ptr)&Com_order::get_At                , VT_DATE       },
    { DISPATCH_PROPERTYGET, 15, "Run_time"                  , (Com_method_ptr)&Com_order::get_Run_time          , VT_DISPATCH   },
    { DISPATCH_METHOD     , 16, "remove_from_job_chain"     , (Com_method_ptr)&Com_order::Remove_from_job_chain , VT_EMPTY      },
    { DISPATCH_PROPERTYGET, 17, "String_next_start_time"    , (Com_method_ptr)&Com_order::get_String_next_start_time, VT_BSTR   },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//--------------------------------------------------------------------------------Com_order::put_id

STDMETHODIMP Com_order::put_Id( VARIANT* id )
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

STDMETHODIMP Com_order::get_Id( VARIANT* result )
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

STDMETHODIMP Com_order::put_Title( BSTR title_bstr )
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

STDMETHODIMP Com_order::get_Title( BSTR* result )
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

STDMETHODIMP Com_order::put_Priority( int priority )
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

STDMETHODIMP Com_order::get_Priority( int* result )
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

STDMETHODIMP Com_order::get_Job_chain( Ijob_chain** result )
{
    HRESULT hr = NOERROR;

    *result = NULL;

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

STDMETHODIMP Com_order::get_Job_chain_node( Ijob_chain_node** result )
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

STDMETHODIMP Com_order::put_Job( VARIANT* job_or_jobname )
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

STDMETHODIMP Com_order::get_Job( Ijob** result )
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

STDMETHODIMP Com_order::put_State( VARIANT* state )
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

STDMETHODIMP Com_order::get_State( VARIANT* result )
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

STDMETHODIMP Com_order::put_State_text( BSTR state_text_bstr )
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

STDMETHODIMP Com_order::get_State_text( BSTR* result )
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

STDMETHODIMP Com_order::get_Error( Ierror** )
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

STDMETHODIMP Com_order::put_Payload( VARIANT* payload )
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

STDMETHODIMP Com_order::putref_Payload( IUnknown* payload )
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

STDMETHODIMP Com_order::get_Payload( VARIANT* result )
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

STDMETHODIMP Com_order::Payload_is_type( BSTR typname_bstr, VARIANT_BOOL* result )
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

STDMETHODIMP Com_order::Setback()
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

//--------------------------------------------------------------------------------Com_order::put_At

STDMETHODIMP Com_order::put_At( VARIANT* datetime )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;
        if( !datetime )  return E_POINTER;

        Time at;
        at.set_datetime( string_from_variant( *datetime ) );

        _order->set_at( at );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_order::get_At

STDMETHODIMP Com_order::get_At( DATE* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = _order->at().as_local_com_date();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------------Com_order::get_Run_time

STDMETHODIMP Com_order::get_Run_time( Irun_time** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        *result = +_order->run_time();
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------Com_order::Remove_from_job_chain

STDMETHODIMP Com_order::Remove_from_job_chain()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        _order->remove_from_job_chain();  
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//------------------------------------------------------------Com_order::get_String_next_start_time

STDMETHODIMP Com_order::get_String_next_start_time( BSTR* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_order )  return E_POINTER;

        hr = String_to_bstr( _order->at().as_string(), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

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
    { DISPATCH_PROPERTYGET,  1, "length"                    , (Com_method_ptr)&Com_order_queue::get_Length      , VT_INT        },
    { DISPATCH_METHOD     ,  2, "add_order"                 , (Com_method_ptr)&Com_order_queue::Add_order       , VT_DISPATCH   , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET,  3, "java_class_name"           , (Com_method_ptr)&Com_order_queue::get_Java_class_name, VT_BSTR },
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
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//----------------------------------------------------------------------Com_order_queue::get_length

STDMETHODIMP Com_order_queue::get_Length( int* result )
{
    THREAD_LOCK( _lock )
    {
        *result = dynamic_cast<Order_queue*>(this)->order_count();
    }

    return S_OK;
}

//-----------------------------------------------------------------------Com_order_queue::add_order

STDMETHODIMP Com_order_queue::Add_order( VARIANT* order_or_payload, Iorder** result )
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

    //LOG( "Com_order_queue::add_order  hr=" << (void*)hr << "\n" );

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
