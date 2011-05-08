// $Id$

#include "zschimmer.h"
//#include <stdio.h>
#include "com_server.h"
#include "log.h"

#include "z_com_server.h"
#include "z_com_server_register.h"
#include "z_windows.h"

using namespace std;

//-------------------------------------------------------------------------------------------------

namespace zschimmer { 
namespace com { 

//-------------------------------------------------------------------------------------------static

static Mutex                    com_mutex                   ( "com" );
static long32                   locked_server_count         = 0;
static long32                   object_count                = 0;

Module_interface::Class_descriptor   Module_interface::class_descriptor ( NULL, "Module_interface"  );

//-----------------------------------------------------------------------------------can_unload_now

bool com_can_unload_now()
{
    return locked_server_count == 0  &&  object_count == 0;
}

//----------------------------------------------------------------------------------set_com_context

void set_com_context( const Com_context* c )
{
    if( c != &com_context )  static_com_context_ptr = c;
}

//------------------------------------------------Idispatch_base_implementation::QueryInterface
/*
#ifdef SYSTEM_HAS_COM

STDMETHODIMP Idispatch_base_implementation::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( static_cast<ISupportErrorInfo*>( this ), iid, ISupportErrorInfo, result );

    return Object::QueryInterface( iid, result );
}

#endif
*/
//--------------------------------------------------------------------------------Set_excepinfo

HRESULT Set_excepinfo( const exception& x, const string& function )
{
#   ifndef SYSTEM_GNU
        if( typeid( x ) == typeid( ::std::ios_base::failure ) )
        {
            try
            {
                throw_errno( errno, "ios" );
            }
            catch( const Xc& xc )
            {
                return Set_excepinfo( xc, function );
            }
        }
#   endif


/*
#   ifdef DYNAMIC_CAST_CRASHES
        int EINFACHERE_FEHLERBEHANDLUNG_WEGEN_ABSTURZ_IN__DYNAMIC_CAST;	// gcc 3.2.3, bei Hostjava File->open( "-out mail ..." );
#    else
        const Xc* xc = dynamic_cast< const Xc* >( &x );
        if( xc )  return set_excepinfo( *xc, function );
#   endif
*/

/*
    string source = _com_class_descriptor? _com_class_descriptor->name() : "";
    if( function  &&  *function )  
    {
        if( !source.empty() )  source += "::";
        source += function;
    }
*/

    HRESULT hr = Set_excepinfo( x.what(), function );

/*
#   ifndef DYNAMIC_CAST_CRASHES
        const Com_exception* com_xc = dynamic_cast< const Com_exception* >( &x );
        if( com_xc )  hr = com_xc->_hresult;        // Eigener Fehlertext geht verloren.
#   endif
*/

    return hr;
}

//--------------------------------------------------------------------------------Set_excepinfo

HRESULT Set_excepinfo( const _com_error& x, const string& )
{
    return x.Error();
}

//--------------------------------------------------------------------------------Set_excepinfo

HRESULT Set_excepinfo( const char* descr, const string& source )
{
    return Com_set_error( descr, source );

  //  string description = descr? rtrim(descr) : "";

  //  Z_LOG( "Set_excepinfo(\"" << source << "\",\"" << description << ")\n" );

  //  HRESULT                 hr;
  //  ptr<ICreateErrorInfo>   create_error_info;
  //  ptr<IErrorInfo>         error_info;

  //  hr = CreateErrorInfo( create_error_info.pp() );
  //  if( FAILED( hr ) )  return hr;

  ////if( _com_class_descriptor )
  ////create_error_info->SetGUID       ( _com_class_descriptor->iid() );

  //  create_error_info->SetSource     ( Bstr( source      ) );
  //  create_error_info->SetDescription( Bstr( string(description) + ", in " + source ) );

  //  hr = create_error_info->QueryInterface( IID_IErrorInfo, error_info.void_pp() );
  //  if( SUCCEEDED(hr) && error_info )  SetErrorInfo( 0, error_info );

  //  return DISP_E_EXCEPTION;
}

//-------------------------------------------------------------------------Typelib_ref::Typelib_ref
/*
Typelib_ref::Typelib_ref() //, const char* version )
:
    _zero_(this+1)
    //_version_string(version)
{
}
*/
//--------------------------------------------------------------------Typelib_ref::Get_class_object

HRESULT Typelib_ref::Get_class_object( const CLSID& clsid, const IID& iid, void** result )
{
    const Com_class_descriptor* c;

    for( c = _class_descriptor_list; c != NULL; c = c->_next )
    {
        if( clsid == c->clsid() )  break;
    }

    if( !c )
    {
        if( clsid == Module_interface::class_descriptor.clsid() )
        {
            if( iid != IID_Imodule_interface2 )  return E_NOINTERFACE;

            ptr<Module_interface> has_com_context = Z_NEW( Module_interface() );
            *result = has_com_context.take();
            return S_OK;
        }

        string name = string_from_clsid( clsid );
        Z_LOG2( "com", Z_FUNCTION << " CLASS_E_CLASSNOTAVAILABLE  Klasse " << name  << "\n" );
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if( iid == IID_IClassFactory )
    {
        ptr<Com_class_factory> f = Z_NEW( Com_class_factory( c ) );

        *result = static_cast< IClassFactory* >( f.take() );
    }
    else
        return E_NOINTERFACE;

    return S_OK;
}

//------------------------------------------------------------------------Typelib_ref::Load_typelib
#ifdef Z_WINDOWS

HRESULT Typelib_ref::Load_typelib()
{
    HRESULT hr = S_OK;

    if( !_typelib )
    Z_MUTEX( com_mutex )
    if( !_typelib )
    {
        char    typelib_filename[ _MAX_PATH+1 ];

        int len = GetModuleFileName( _hinstance, typelib_filename, sizeof typelib_filename );
        if( !len ) 
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            Z_LOG( "GetModuleFileName(" << (void*)_hinstance << ") => " << string_from_hresult( hr ) << "\n" );
            return hr;
        }

        hr = LoadTypeLib( Bstr( typelib_filename ), _typelib.pp() );
        if( FAILED(hr) )  
        {
            Z_LOG( "LoadTypeLib(" << typelib_filename << ") => " << string_from_hresult( hr ) << "\n" );
            return hr;
        }

        {
            TLIBATTR* libattr;
            hr = _typelib->GetLibAttr( &libattr );
            if( FAILED(hr) )  
            {
                Z_LOG( "GetLibAttr() => " << string_from_hresult( hr ) << "\n" );
                return hr;
            }

            _libattr = *libattr;

            _typelib->ReleaseTLibAttr( libattr );
        }

        _typelib_id = _libattr.guid;
        ostringstream version;
        version << _libattr.wMajorVerNum << '.' << _libattr.wMinorVerNum;
        _version_string = version.str();
    }

    return hr;
}

#endif
//---------------------------------------------------------------------Typelib_ref::Register_server
#ifdef Z_WINDOWS

HRESULT Typelib_ref::Register_server()
{
    HRESULT hr;

    string filename = windows::filename_from_hinstance( _hinstance );

    hr = Load_typelib();
    if( FAILED(hr) )  return hr;

    for( const Com_class_descriptor* c = _class_descriptor_list; c != NULL; c = c->_next )
    {
        if( c->_creatable )
        {
            hr = Com_register_class( filename, _typelib_id, c->clsid(), c->name(), _version_string, c->title() );
            if( FAILED(hr) )  return hr;
        }
    }

    hr = RegisterTypeLib( _typelib, Bstr( filename ), NULL );
    Z_LOG( "RegisterTypeLib(," << filename << ") ==> " << string_from_hresult( hr ) << "\n" );

    return hr;
}

#endif
//-------------------------------------------------------------------Typelib_ref::Unregister_server
#ifdef Z_WINDOWS

HRESULT Typelib_ref::Unregister_server()
{
    HRESULT hr = S_OK;
    HRESULT error;

    string filename = windows::filename_from_hinstance( _hinstance );

    HRESULT hr2 = Load_typelib();       // Setzt _typelib_id und _version_string.
    if( !FAILED(hr) )  hr = hr2;

    for( const Com_class_descriptor* c = _class_descriptor_list; c != NULL; c = c->_next )
    {
        if( c->_creatable )
        {
            error = Com_unregister_class( c->clsid(), c->name(), _version_string );
            if( error )  hr = error;
        }
    }

    if( _typelib )
    {
        error = UnRegisterTypeLib( _typelib_id, _libattr.wMajorVerNum, _libattr.wMinorVerNum, LANG_NEUTRAL, SYS_WIN32 );
        Z_LOG( "UnRegisterTypeLib() ==> " << string_from_hresult( error ) << "\n" );
        if( error )  hr = error;
    }

    return hr;
}

#endif
//-------------------------------------------------------Com_class_descriptor::Com_class_descriptor

Com_class_descriptor::Com_class_descriptor( Typelib_ref* typelib_ref, const CLSID& clsid, const IID& iid, 
                                            const char* class_name, Creatable c, const Com_method* methods )
:
    _zero_(this+1),
    _typelib_ref( typelib_ref ),
    _clsid      ( clsid ),
    _iid        ( iid ),
    _name       ( class_name ),
    _methods    ( methods )
{
    if( _typelib_ref )
    {
        _next = _typelib_ref->_class_descriptor_list;
        _typelib_ref->_class_descriptor_list = this;
    }

    _creatable = c == creatable;
}

//-------------------------------------------------------------Com_class_descriptor::add_to_typelib
/*
void Com_class_descriptor::add_to_typelib( Typelib_ref* typelib_ref )
{
    _next = typelib_ref->_class_descriptor_list;
    typelib_ref->_class_descriptor_list = this;
}
*/
//--------------------------------------------------------------Com_class_descriptor::Load_typeinfo
#ifdef Z_WINDOWS

HRESULT Com_class_descriptor::Load_typeinfo()
{
    HRESULT hr;

    hr = _typelib_ref->Load_typelib();
    if( FAILED(hr) )  return hr;

    if( !_typeinfo )
    Z_MUTEX( com_mutex )
    if( !_typeinfo )
    {
        hr = _typelib_ref->typelib()->GetTypeInfoOfGuid( _iid, _typeinfo.pp() );
    }

    return hr;
}

#endif
//--------------------------------------------------------------Com_class_descriptor::GetIDsOfNames

HRESULT Com_class_descriptor::GetIDsOfNames( const IID& iid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* results )
{
#   ifdef SYSTEM_HAS_COM
        
        HRESULT hr;

        if( iid != IID_NULL )  return DISP_E_UNKNOWNINTERFACE;

        if( !typeinfo() ) 
        {
            hr = Load_typeinfo();
            if( FAILED(hr) )  return hr;
        }

        return DispGetIDsOfNames( typeinfo(), rgszNames, cNames, results );

#    else

        return Com_get_dispid( _methods, iid, rgszNames, cNames, lcid, results );

#   endif
}

//---------------------------------------------------------------------Com_class_descriptor::Invoke

HRESULT Com_class_descriptor::Invoke( IDispatch* idispatch, DISPID dispid, const IID& iid, LCID lcid, unsigned short wFlags, 
                                      DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* error_arg_nr )
{
#   ifdef SYSTEM_HAS_COM

        HRESULT hr;

        if( iid != IID_NULL )  return DISP_E_UNKNOWNINTERFACE;

        if( !typeinfo() ) 
        {
            hr = Load_typeinfo();
            if( FAILED(hr) )  return hr;
        }

        return DispInvoke( idispatch, typeinfo(), dispid, wFlags, dispparams, result, excepinfo, error_arg_nr );

#    else

        return Com_invoke( idispatch, _methods, dispid, iid, lcid, wFlags, dispparams, error_arg_nr, result, excepinfo );

#   endif
}

//------------------------------------------------------------Com_class_descriptor::Create_instance

HRESULT Com_class_descriptor::Create_instance( IUnknown* outer, const IID& iid, void** result ) const
{
    HRESULT hr = S_OK;

    if( outer )  return CLASS_E_NOAGGREGATION;

    if( iid == IID_IUnknown 
     || iid == IID_IDispatch
     || iid == _iid          )
    {
        try
        {
            create( result );
        }
        catch( const Com_exception& x )  { hr = x._hresult; }
        catch( const exception&     x )  { Z_LOG( Z_FUNCTION << ": " << x.what() << "\n" );  hr = CLASS_E_CLASSNOTAVAILABLE; }
        catch( const _com_error&    x )  { hr = x.Error(); }

        if( SUCCEEDED(hr)  &&  *result == NULL )  hr = CLASS_E_CLASSNOTAVAILABLE;
        return hr;
    }
    else
    {
        //return E_NOINTERFACE;
        // Vielleicht liefert QueryInterface() das gewünschte Interface. Sonst wird das Objekt umsonst angelegt und wieder verworfen.

        ptr<IUnknown> iunknown;
        void*         void_ptr = NULL;

        hr = Create_instance( outer, IID_IUnknown, &void_ptr );
        if( FAILED(hr) )  return hr;

        iunknown._ptr = static_cast<IUnknown*>( void_ptr );

        return iunknown->QueryInterface( iid, result );
    }
}

//----------------------------------------------------------------Com_class_factory::CreateInstance

STDMETHODIMP Com_class_factory::CreateInstance( IUnknown* outer, const IID& iid, void** result )
{
    HRESULT hr = _class_descriptor->Create_instance( outer, iid, result );
    if( FAILED(hr) )  Z_LOG( _class_descriptor->name() << " Com_class_factory::CreateInstance( " << iid << ") ==> " << string_from_hresult(hr) << "\n" );

    return hr;
}

//--------------------------------------------------------------------Com_class_factory::LockServer

STDMETHODIMP Com_class_factory::LockServer( BOOL lock )
{ 
    if( lock )  InterlockedIncrement( &locked_server_count );
          else  InterlockedDecrement( &locked_server_count ); 

    return S_OK;
}

//-------------------------------------------------------------Module_interface::putref_Com_context

STDMETHODIMP Module_interface::putref_Com_context( const Com_context* c )
{ 
    set_com_context( c );  
    return S_OK; 
}

//-------------------------------------------------------------Module_interface::put_Log_categories

STDMETHODIMP Module_interface::put_Log_categories( const BSTR c )
{ 
    static_log_categories.set_multiple( string_from_bstr( c ) ); 
    return S_OK; 
}

//----------------------------------------------------Module_interface::Set_stream_and_system_mutex

STDMETHODIMP Module_interface::put_Log_context( Log_context** log_context )
{
    Log_ptr::set_log_context( log_context );
    return S_OK;
}

//----------------------------------------------------Module_interface::Set_stream_and_system_mutex
/*
STDMETHODIMP Module_interface::Set_stream_and_system_mutex( ostream** os, System_mutex* m )
{
    Log_ptr::set_stream_and_system_mutex( os, m );
    return S_OK;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer
